//===-------- SoftBoundMechanism.cpp - Implementation of SoftBound --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Implementation of Instrumentation Mechanism interface for SoftBound.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/SoftBoundMechanism.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"
#include "meminstrument/instrumentation_mechanisms/softbound/RunTimeHandles.h"
#include "meminstrument/instrumentation_mechanisms/softbound/RunTimePrototypes.h"
#include "meminstrument/instrumentation_mechanisms/softbound/SoftBoundWitness.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "softbound"

// The bounds for in-memory pointer are loaded from a separate data structure,
// keep track of how often this is necessary.
STATISTIC(MetadataLoadInserted, "Number of metadata loads inserted");

// Stores of pointers into memory require that the base and bound are tracked,
// such that upon loading and using the pointer afterwards, they can be looked
// up.
STATISTIC(MetadataStoresInserted, "Number of metadata stores inserted");

// Number of nullptr bounds stored for pointer. This can happen for actual
// nullptr in the code, or pointer which we restrict to access (e.g. when they
// are casted from an integer).
STATISTIC(NullptrBoundsRequested, "Number of nullptr bounds requested");

// Number of requests for bounds of function pointers.
STATISTIC(FunctionBoundsRequested, "Number of function bounds requested");

// The setup phase detected code in the module which will prevent the
// instrumentation from making the program safe.
STATISTIC(
    SetupErrror,
    "Number of modules for which the setup phase already detected problems");

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

cl::OptionCategory SBCategory("SoftBound options");

static cl::opt<bool> NoCTErrorIntToPtr(
    "sb-no-ct-error-inttoptr", cl::cat(SBCategory),
    cl::desc("Compile programs which contain int to pointer casts. Null bounds "
             "will be stored, such that upon access of the pointer (but only "
             "then) the program crashes."));

//===----------------------------------------------------------------------===//
//                   Implementation of SoftBoundMechanism
//===----------------------------------------------------------------------===//

SoftBoundMechanism::SoftBoundMechanism(GlobalConfig &config)
    : InstrumentationMechanism(config) {}

void SoftBoundMechanism::initialize(Module &module) {

  // Currently only spatial safety is implemented, don't do anything if temporal
  // memory safety is requested.
  if (!InternalSoftBoundConfig::ensureOnlySpatial()) {
    globalConfig.noteError();
    return;
  }

  // Store the context to avoid looking it up all the time
  context = &module.getContext();

  // First, check if any unimplemented or problematic constructs are contained
  // in this module (e.g. exception handling)
  checkModule(module);

  // Insert the declarations for basic metadata and check functions
  insertFunDecls(module);

  // Rename the main function such that it can be linked against the run-time
  renameMain(module);

  // Generate setup function that inserts metadata stores for global variables
  setUpGlobals(module);

  if (globalConfig.hasErrors()) {
    ++SetupErrror;
  }
}

void SoftBoundMechanism::insertWitness(ITarget &target) const {

  LLVM_DEBUG(dbgs() << "Insert witness for: " << target << "\n";);

  assert(isa<IntermediateIT>(&target));

  llvm::IRBuilder<> builder(target.getLocation());

  Value *base = nullptr;
  Value *bound = nullptr;
  auto *instrumentee = target.getInstrumentee();
  LLVM_DEBUG(dbgs() << "Instrumentee: " << *instrumentee << "\n";);

  if (isa<Argument>(instrumentee) || isa<CallInst>(instrumentee)) {
    auto locIndex = computeShadowStackLocation(instrumentee);
    std::tie(base, bound) = insertShadowStackLoad(builder, locIndex);
  }

  if (AllocaInst *alloc = dyn_cast<AllocaInst>(instrumentee)) {
    base = builder.CreateConstGEP1_32(alloc, 0);
    bound = builder.CreateConstGEP1_32(alloc, 1);
  }

  if (auto constant = dyn_cast<Constant>(instrumentee)) {
    std::tie(base, bound) = getBoundsForWitness(constant);
  }

  if (auto load = dyn_cast<LoadInst>(instrumentee)) {
    std::tie(base, bound) =
        insertMetadataLoad(builder, load->getPointerOperand());
  }

  if (globalConfig.hasErrors()) {
    return;
  }

  assert(base && bound);
  std::tie(base, bound) = addBitCasts(builder, base, bound);
  LLVM_DEBUG({
    dbgs() << "Created bounds: "
           << "\n\tBase: " << *base << "\n\tBound: " << *bound << "\n";
  });

  target.setBoundWitness(
      std::make_shared<SoftBoundWitness>(base, bound, instrumentee));
}

void SoftBoundMechanism::relocCloneWitness(Witness &toReloc,
                                           ITarget &target) const {
  target.setBoundWitness(std::make_shared<SoftBoundWitness>(
      toReloc.getLowerBound(), toReloc.getUpperBound(), target.getLocation()));
}

auto SoftBoundMechanism::insertWitnessPhi(ITarget &target) const
    -> std::shared_ptr<Witness> {
  auto *phi = cast<PHINode>(target.getInstrumentee());

  IRBuilder<> builder(phi);

  auto *basePhi =
      builder.CreatePHI(handles.baseTy, phi->getNumIncomingValues());
  basePhi->setName("base.phi");
  auto *boundPhi =
      builder.CreatePHI(handles.boundTy, phi->getNumIncomingValues());
  boundPhi->setName("bound.phi");

  target.setBoundWitness(
      std::make_shared<SoftBoundWitness>(basePhi, boundPhi, phi));
  return target.getBoundWitness();
}

void SoftBoundMechanism::addIncomingWitnessToPhi(
    std::shared_ptr<Witness> &phi, std::shared_ptr<Witness> &incoming,
    BasicBlock *inBB) const {

  auto *lowerPhi = cast<PHINode>(phi->getLowerBound());
  auto *upperPhi = cast<PHINode>(phi->getUpperBound());

  lowerPhi->addIncoming(incoming->getLowerBound(), inBB);
  upperPhi->addIncoming(incoming->getUpperBound(), inBB);
}

auto SoftBoundMechanism::insertWitnessSelect(
    ITarget &target, std::shared_ptr<Witness> &trueWitness,
    std::shared_ptr<Witness> &falseWitness) const -> std::shared_ptr<Witness> {

  auto *sel = cast<SelectInst>(target.getInstrumentee());
  auto *cond = sel->getCondition();

  IRBuilder<> builder(sel);
  auto *lowerSel = builder.CreateSelect(cond, trueWitness->getLowerBound(),
                                        falseWitness->getLowerBound());
  lowerSel->setName("base.sel");
  auto *upperSel = builder.CreateSelect(cond, trueWitness->getUpperBound(),
                                        falseWitness->getUpperBound());
  upperSel->setName("bound.sel");

  target.setBoundWitness(
      std::make_shared<SoftBoundWitness>(lowerSel, upperSel, sel));
  return target.getBoundWitness();
}

void SoftBoundMechanism::materializeBounds(ITarget &) {
  // The bounds are always materialized, nothing to do
}

void SoftBoundMechanism::insertCheck(ITarget &target) const {

  if (auto cInvIT = dyn_cast<CallInvariantIT>(&target)) {
    handleCallInvariant(*cInvIT);
    return;
  }
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::addBitCasts(IRBuilder<> builder, Value *base,
                                     Value *bound) const
    -> std::pair<Value *, Value *> {

  if (base->getType() != handles.baseTy) {
    base = insertCast(handles.baseTy, base, builder);
  }

  if (bound->getType() != handles.boundTy) {
    bound = insertCast(handles.boundTy, bound, builder);
  }

  return std::make_pair(base, bound);
}

auto SoftBoundMechanism::getFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getVerboseFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getExtCheckCounterFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getName() const -> const char * { return "SoftBound"; }

//===-------------------------- private -----------------------------------===//

/// Insert the declarations for SoftBound metadata propagation functions
void SoftBoundMechanism::insertFunDecls(Module &module) {

  PrototypeInserter protoInserter(module);
  handles = protoInserter.insertRunTimeProtoypes();
}

void SoftBoundMechanism::renameMain(Module &module) {

  Function *mainFun = module.getFunction("main");

  // This module does not have a main function, nothing to do here
  if (!mainFun) {
    return;
  }

  mainFun->setName("softboundcets_pseudo_main");
}

void SoftBoundMechanism::setUpGlobals(Module &module) {

  // Look up the run-time init function
  auto initFun = handles.init;

  // Register a static constructor that collects all global allocation size
  // information
  auto listOfFuns = registerCtors(
      module,
      std::make_pair<StringRef, int>("__softboundcets_globals_setup", 0));
  auto globalSetupFun = (*listOfFuns)[0];

  MDNode *node = MDNode::get(
      *context,
      MDString::get(*context, InternalSoftBoundConfig::getSetupInfoStr()));
  globalSetupFun->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);

  auto entryBlock = BasicBlock::Create(*context, "entry", globalSetupFun);

  IRBuilder<> Builder(entryBlock);

  // Call the run-time init functions
  Builder.CreateCall(FunctionCallee(initFun));

  // Take care of globally initialized global variables
  for (GlobalVariable &global : module.globals()) {

    LLVM_DEBUG(dbgs() << "Deal with global: " << global << "\n";);

    // Skip values that are not to be instrumented
    if (hasNoInstrument(&global)) {
      continue;
    }

    // No bound information to collect if there is no initializer
    if (!global.hasInitializer()) {
      continue;
    }

    // Skip llvm internal variables
    if (global.getName().startswith("llvm.")) {
      continue;
    }

    Constant *glInit = global.getInitializer();

    // Null value initialization is unproblematic
    if (glInit->isNullValue()) {
      continue;
    }

    std::vector<Value *> Indices;
    auto idxThis = ConstantInt::get(IntegerType::getInt32Ty(*context), 0);
    Indices.push_back(idxThis);
    Value *base = nullptr;
    Value *bound = nullptr;
    LLVM_DEBUG(dbgs() << "Handle initializer " << *glInit << "\n";);
    std::tie(base, bound) = handleInitializer(glInit, Builder, global, Indices);

    // An error already occurred, stop the initialization.
    if (globalConfig.hasErrors()) {
      break;
    }

    if (glInit->getType()->isPointerTy()) {
      assert(base && bound);
      LLVM_DEBUG({
        dbgs() << "\tBase: " << *base << "\n\tBound: " << *bound << "\n";
      });
      insertMetadataStore(Builder, &global, base, bound);
    }
  }

  Builder.CreateRetVoid();
}

auto SoftBoundMechanism::handleInitializer(Constant *glInit,
                                           IRBuilder<> &Builder,
                                           Value &topLevel,
                                           std::vector<Value *> indices) const
    -> std::pair<Value *, Value *> {

  Value *base = nullptr;
  Value *bound = nullptr;

  if (globalConfig.hasErrors()) {
    return std::make_pair(base, bound);
  }

  if (isa<BlockAddress>(glInit)) {
    std::tie(base, bound) = getBoundsConst(glInit);
    return std::make_pair(base, bound);
  }

  if (isa<GlobalIndirectSymbol>(glInit)) {
    globalConfig.noteError();
    return std::make_pair(base, bound);
  }

  // Return null bounds if a nullptr is explicitly stored somewhere
  if (glInit->isNullValue() && glInit->getType()->isPointerTy()) {
    return getNullPtrBounds();
  }

  // Constant data are simple types such as int, float, vector of int/float,
  // array of int/float, etc. or zero initializer. No pointers hidden.
  if (isa<ConstantData>(glInit)) {
    LLVM_DEBUG(dbgs() << "Constant data\n";);
    return std::make_pair(base, bound);
  }

  const Type *initType = glInit->getType();

  if (initType->isFloatingPointTy() || initType->isIntegerTy()) {
    LLVM_DEBUG(dbgs() << "Int or float type\n";);
    return std::make_pair(base, bound);
  }

  if (ConstantExpr *constExpr = dyn_cast<ConstantExpr>(glInit)) {

    if (constExpr->getType()->isPointerTy()) {

      switch (constExpr->getOpcode()) {
      case Instruction::IntToPtr: {
        if (!NoCTErrorIntToPtr) {
          LLVM_DEBUG(
              dbgs() << "Integer to pointer cast found, report an error.\n";);
          globalConfig.noteError();
          return std::make_pair(base, bound);
        }
        // Store null pointer bounds for pointers derived from integers
        std::tie(base, bound) = getNullPtrBounds();
        return std::make_pair(base, bound);
      }
      case Instruction::GetElementPtr:
        // Fall through
      case Instruction::BitCast: {
        // Recursively go through geps/bitcasts to find the base ptr
        auto *ptrOp = constExpr->getOperand(0);

        std::tie(base, bound) =
            handleInitializer(ptrOp, Builder, topLevel, indices);
        return std::make_pair(base, bound);
      }
      default:
        llvm_unreachable("Unimplemented constant expression with pointer type");
      }
    }

    llvm_unreachable(
        "Unimplemented constant expression with non-pointer/-integer/-float "
        "type that is not constant data");
  }

  if (isa<GlobalObject>(glInit)) {

    if (isa<GlobalVariable>(glInit)) {
      return getBoundsConst(glInit);
    }

    if (isa<Function>(glInit)) {
      return getBoundsForFun(glInit);
    }

    llvm_unreachable(
        "Global object was neither a function nor a global variable.");
  }

  if (ConstantAggregate *ag = dyn_cast<ConstantAggregate>(glInit)) {
    if (const VectorType *vecTy = dyn_cast<VectorType>(initType)) {

      // Only allow simple vector types that do not contain pointer values
      if (!isSimpleVectorTy(vecTy)) {
        globalConfig.noteError();
        return std::make_pair(base, bound);
      }
    }

    // Recursively handle all elements of an array or struct
    for (unsigned i = 0; i < ag->getNumOperands(); i++) {
      Constant *elem = ag->getAggregateElement(i);
      Value *subBase = nullptr;
      Value *subBound = nullptr;
      auto tmpIndices = indices;
      auto idxOne = ConstantInt::get(IntegerType::getInt32Ty(*context), i);
      tmpIndices.push_back(idxOne);

      // Collect the base and bound of the underlying element...
      std::tie(subBase, subBound) =
          handleInitializer(elem, Builder, topLevel, tmpIndices);

      // ...and store them in case of this data structures contains a pointer
      if (elem->getType()->isPointerTy()) {
        assert(subBase && subBound);

        LLVM_DEBUG({
          dbgs() << "Create gep for " << topLevel << "\nIndices: ";
          for (const auto &idx : tmpIndices) {
            dbgs() << " " << *idx;
          }
          dbgs() << "\n";
        });

        Value *gepToElem = Builder.CreateInBoundsGEP(&topLevel, tmpIndices);
        insertMetadataStore(Builder, gepToElem, subBase, subBound);
      }
    }

    return std::make_pair(base, bound);
  }

  llvm_unreachable("Unimplemented constant global initializer.");
}

void SoftBoundMechanism::handleCallInvariant(CallInvariantIT &) const {

  // TODO Insert shadow stack allocation + deallocation

  // TODO rename wrapped functions

  // TODO intrinsics might need additional calls for metadata copying
}

auto SoftBoundMechanism::getBoundsConst(Constant *cons) const
    -> std::pair<Value *, Value *> {

  IRBuilder<> builder(*context);
  auto *base = builder.CreateConstGEP1_32(cons, 0);
  auto *bound = builder.CreateConstGEP1_32(cons, 1);

  std::tie(base, bound) = addBitCasts(builder, base, bound);

  return std::make_pair(base, bound);
}

auto SoftBoundMechanism::getBoundsForFun(Value *cons) const
    -> std::pair<Value *, Value *> {

  ++FunctionBoundsRequested;

  // Function pointer should not be accessed as if they were regular pointer to
  // an object of any size, so we fix there base and bound to be the address of
  // the function. A call check ensures at run time that ptr=base=bound holds
  // when calling a function pointer.
  return std::make_pair(cons, cons);
}

auto SoftBoundMechanism::getNullPtrBounds() const
    -> std::pair<Value *, Value *> {

  ++NullptrBoundsRequested;

  auto base = ConstantPointerNull::get(handles.baseTy);
  auto bound = ConstantPointerNull::get(handles.boundTy);

  return std::make_pair(base, bound);
}

auto SoftBoundMechanism::getBoundsForWitness(Constant *constant) const
    -> std::pair<Value *, Value *> {

  // Construct simple bounds with the width of the global variable, block
  // address or constant aggregate of some sort
  if (isa<GlobalVariable>(constant) || isa<ConstantAggregate>(constant) ||
      isa<ConstantDataSequential>(constant) ||
      isa<ConstantAggregateZero>(constant) || isa<BlockAddress>(constant)) {
    return getBoundsConst(constant);
  }

  // Use special bounds for functions
  if (isa<Function>(constant)) {
    return getBoundsForFun(constant);
  }

  // Nullptr are not accessible, therefore the bounds are nullptr as well
  if (isa<ConstantPointerNull>(constant)) {
    return getNullPtrBounds();
  }

  // Disallow accessing undef values
  if (isa<UndefValue>(constant)) {
    return getNullPtrBounds();
  }

  // The bounds for an integer casted to a pointer are requested, we won't allow
  // the access to it, therefore nullptr bounds are stored
  if (isa<ConstantInt>(constant)) {
    assert(NoCTErrorIntToPtr);
    return getNullPtrBounds();
  }

  if (isa<ConstantFP>(constant)) {
    llvm_unreachable("Pointer source is a floating point value...");
  }

  if (isa<GlobalIndirectSymbol>(constant)) {
    llvm_unreachable("Global indirect symbols are not handled.");
  }

  if (ConstantExpr *constExpr = dyn_cast<ConstantExpr>(constant)) {

    if (constExpr->getType()->isPointerTy()) {

      switch (constExpr->getOpcode()) {
      case Instruction::IntToPtr: {
        // Store null pointer bounds for pointers derived from integers
        return getNullPtrBounds();
      }
      case Instruction::GetElementPtr:
        // Fall through
      case Instruction::BitCast: {
        // Recursively go through geps/bitcasts to find the base ptr
        auto *ptrOp = cast<Constant>(constExpr->getOperand(0));
        return getBoundsForWitness(ptrOp);
      }
      default:
        llvm_unreachable("Unimplemented constant expression with pointer type");
      }
    }
  }

  llvm_unreachable("Unhandled case when constructing bound witnesses");
}

auto SoftBoundMechanism::insertMetadataLoad(IRBuilder<> &builder,
                                            Value *ptr) const
    -> std::pair<Value *, Value *> {

  ++MetadataLoadInserted;

  assert(ptr);

  if (handles.voidPtrTy != ptr->getType()) {
    ptr = insertCast(handles.voidPtrTy, ptr, builder);
  }

  // Allocate space for base and bound for the call stores the information in
  auto allocBase = builder.CreateAlloca(handles.baseTy);
  allocBase->setName("base.alloc");
  auto allocBound = builder.CreateAlloca(handles.boundTy);
  allocBound->setName("bound.alloc");

  LLVM_DEBUG(dbgs() << "Insert metadata load:\n"
                    << "\tPtr: " << *ptr << "\n\tBaseAlloc: " << *allocBase
                    << "\n\tBoundAlloc: " << *allocBound << "\n";);

  ArrayRef<Value *> args = {ptr, allocBase, allocBound};

  auto call =
      builder.CreateCall(FunctionCallee(handles.loadInMemoryPtrInfo), args);

  auto base = builder.CreateLoad(allocBase);
  auto bound = builder.CreateLoad(allocBound);

  MDNode *node = MDNode::get(
      *context,
      MDString::get(*context, InternalSoftBoundConfig::getMetadataInfoStr()));
  call->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);

  return std::make_pair(base, bound);
}

void SoftBoundMechanism::insertMetadataStore(IRBuilder<> &builder, Value *ptr,
                                             Value *base, Value *bound) const {

  ++MetadataStoresInserted;

  assert(ptr && base && bound);

  // Make sure the types are correct
  if (handles.voidPtrTy != ptr->getType()) {
    ptr = insertCast(handles.voidPtrTy, ptr, builder);
  }
  std::tie(base, bound) = addBitCasts(builder, base, bound);

  LLVM_DEBUG(dbgs() << "Insert metadata store:\n"
                    << "\tPtr: " << *ptr << "\n\tBase: " << *base
                    << "\n\tBound: " << *bound << "\n";);

  ArrayRef<Value *> args = {ptr, base, bound};
  auto call =
      builder.CreateCall(FunctionCallee(handles.storeInMemoryPtrInfo), args);

  MDNode *node = MDNode::get(
      *context,
      MDString::get(*context, InternalSoftBoundConfig::getMetadataInfoStr()));
  call->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);
}

auto SoftBoundMechanism::insertShadowStackLoad(llvm::IRBuilder<> &builder,
                                               int locIndex) const
    -> std::pair<Value *, Value *> {

  auto locIndexVal = ConstantInt::get(handles.intTy, locIndex, true);

  ArrayRef<Value *> args = {locIndexVal};
  auto callLoadBase =
      builder.CreateCall(FunctionCallee(handles.loadBaseStack), args);
  auto callLoadBound =
      builder.CreateCall(FunctionCallee(handles.loadBoundStack), args);

  MDNode *node = MDNode::get(
      *context,
      MDString::get(*context,
                    InternalSoftBoundConfig::getShadowStackInfoStr()));
  callLoadBase->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);
  callLoadBound->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);

  return std::make_pair(callLoadBase, callLoadBound);
}

void SoftBoundMechanism::insertShadowStackStore(IRBuilder<> &builder,
                                                ITarget &) const {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::computeShadowStackLocation(const Value *val) const
    -> int {

  assert(val->getType()->isPointerTy());

  int shadowStackLoc = 0;
  if (isa<CallInst>(val)) {
    return shadowStackLoc;
  }

  const auto arg = dyn_cast<Argument>(val);
  assert(arg);
  auto fun = arg->getParent();

  // The returned pointer will have location 0, skip it in case something else
  // is requested
  if (fun->getReturnType()->isPointerTy()) {
    shadowStackLoc++;
  }

  for (const auto &funArg : fun->args()) {
    if (!funArg.getType()->isPointerTy()) {
      // Non-pointer arguments are not accounted for in the location calculation
      continue;
    }

    if (arg == &funArg) {
      return shadowStackLoc;
    }
    shadowStackLoc++;
  }

  llvm_unreachable(
      "Argument not found, shadow stack location cannot be determined");
}

bool SoftBoundMechanism::isSimpleVectorTy(const VectorType *vecTy) const {

  const Type *elemType = vecTy->getElementType();

  if (elemType->isPointerTy()) {
    return false;
  }

  // Make sure to bail out if new unexpected vector types are introduced
  if (!(elemType->isIntegerTy() || elemType->isFloatingPointTy())) {
    llvm_unreachable("Unexpected vector element type discovered.");
  }

  return true;
}

bool SoftBoundMechanism::containsUnsupportedOp(const Constant *cons) const {
  if (!isa<ConstantExpr>(cons)) {
    return false;
  }

  auto constExpr = cast<ConstantExpr>(cons);
  if (isUnsupportedInstruction(constExpr->getOpcode())) {
    return true;
  }

  for (const auto &op : constExpr->operands()) {
    if (containsUnsupportedOp(cast<Constant>(op))) {
      return true;
    }
  }
  return false;
}

bool SoftBoundMechanism::isUnsupportedInstruction(unsigned opC) const {

  // Bail if exception handling is involved
  if (Instruction::isExceptionalTerminator(opC)) {
    return true;
  }

  switch (opC) {
  case Instruction::CatchPad:
  case Instruction::CleanupPad:
  case Instruction::LandingPad:
    return true;
  case Instruction::IntToPtr: {
    if (!NoCTErrorIntToPtr) {
      return true;
    }
    break;
  }
  default:
    break;
  }
  return false;
}

void SoftBoundMechanism::checkModule(Module &module) {
  for (const auto &fun : module) {
    for (const auto &bb : fun) {
      for (const Instruction &inst : bb) {

        if (isUnsupportedInstruction(inst.getOpcode())) {
          globalConfig.noteError();
          return;
        }

        for (const auto &arg : inst.operands()) {
          if (const auto &cons = dyn_cast<Constant>(arg)) {
            if (containsUnsupportedOp(cons)) {
              globalConfig.noteError();
              return;
            }
          }
        }
      }
    }
  }
}