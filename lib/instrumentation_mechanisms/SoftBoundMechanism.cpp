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

// Number of requests for wide bounds.
STATISTIC(WideBoundsRequested, "Number of wide bounds requested");

// Number of requests for bounds of function pointers.
STATISTIC(FunctionBoundsRequested, "Number of function bounds requested");

// Number of requests for bounds of global arrays whose size is unknown within
// the module.
STATISTIC(ZeroSizedArrayBoundsRequested,
          "Number of bounds requested for arrays declarations of size zero");

// The setup phase detected code in the module which will prevent the
// instrumentation from making the program safe.
STATISTIC(
    SetupError,
    "Number of modules for which the setup phase already detected problems");

// Detailed statistics on which setup error occurred:

STATISTIC(InAllocaArg,
          "[setup error] Number of inalloca arguments encountered");

STATISTIC(
    ExceptionHandlingInst,
    "[setup error] Number of exception handling instructions encountered");
STATISTIC(IntToPtrCast, "[setup error] Number of int to pointer casts");

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

cl::OptionCategory SBCategory("SoftBound options");

enum BadPtrSrc { Disallow, NullBounds, WideBounds };

static cl::opt<BadPtrSrc> IntToPtrHandling(
    cl::desc("Ways to deal with integer to pointer casts:"),
    cl::cat(SBCategory), cl::init(Disallow),
    cl::values(
        clEnumValN(Disallow, "sb-inttoptr-disallow",
                   "Don't even compile the program."),
        clEnumValN(NullBounds, "sb-inttoptr-null-bounds",
                   "Assign nullptr bounds to pointers derived from integers. "
                   "Upon access of the pointer at run time a memory safety "
                   "violation will be reported."),
        clEnumValN(WideBounds, "sb-inttoptr-wide-bounds",
                   "Assign wide bounds to pointers derived from integers. It "
                   "will be possible to access arbitrary memory through a "
                   "pointer derived from an integer.")));

static cl::opt<BadPtrSrc> ZeroSizedArrayHandling(
    cl::desc("Ways to deal with arrays that are defined externally and appear "
             "to have size zero within the currently compiled module:"),
    cl::cat(SBCategory), cl::init(Disallow),
    cl::values(
        clEnumValN(Disallow, "sb-size-zero-disallow",
                   "Don't even compile the program."),
        clEnumValN(
            NullBounds, "sb-size-zero-model-as-such",
            "Use zero as the size of the array. Upon access of the pointer at "
            "run time a memory safety violation will be reported."),
        clEnumValN(
            WideBounds, "sb-size-zero-wide-upper",
            "Assign a wide upper bound to arrays of size zero. Underflows can "
            "still be detected, overflows will go unnoticed.")));

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

  // Store the context and data layout to avoid looking it up all the time
  context = &module.getContext();
  DL = &module.getDataLayout();

  // Replace calls to library functions with calls to the SoftBound wrappers
  replaceWrappedFunction(module);

  // Check if any unimplemented or problematic constructs are contained in this
  // module (e.g. exception handling)
  checkModule(module);

  if (globalConfig.hasErrors()) {
    ++SetupError;
  }

  // Insert the declarations for basic metadata and check functions
  insertFunDecls(module);

  // Rename the main function such that it can be linked against the run-time
  renameMain(module);

  // Insert allocas to store loaded metadata in
  insertMetadataAllocs(module);

  // Generate setup function that inserts metadata stores for global variables
  setUpGlobals(module);

  if (globalConfig.hasErrors()) {
    ++SetupError;
  }
}

void SoftBoundMechanism::insertWitnesses(ITarget &target) const {

  LLVM_DEBUG(dbgs() << "Insert witness for: " << target << "\n";);

  assert(isa<IntermediateIT>(&target));
  IntermediateIT &inTarget = *cast<IntermediateIT>(&target);

  auto *instrumentee = target.getInstrumentee();
  LLVM_DEBUG(dbgs() << "Instrumentee: " << *instrumentee << "\n";);
  if (isa<Instruction>(instrumentee) &&
      hasVarArgHandling(cast<Instruction>(instrumentee))) {
    insertVarArgWitness(inTarget);
    return;
  }
  if (isVarArgMetadataType(instrumentee->getType())) {
    insertVarArgWitness(inTarget);
    return;
  }

  IRBuilder<> builder(target.getLocation());

  Value *base = nullptr;
  Value *bound = nullptr;

  if (auto cb = dyn_cast<CallBase>(instrumentee)) {

    // Compute the locations of pointers with bounds in the witness
    auto locs = computeIndices(cb);
    unsigned shadowStackIndex = 0;
    for (auto locIndex : locs) {
      // Insert the loads of bounds
      std::tie(base, bound) = insertShadowStackLoad(builder, shadowStackIndex);
      LLVM_DEBUG({
        dbgs() << "Created bounds: "
               << "\n\tBase: " << *base << "\n\tBound: " << *bound << "\n";
      });
      // Set the loaded bounds as witness for this pointer
      target.setBoundWitness(
          std::make_shared<SoftBoundWitness>(base, bound, instrumentee),
          locIndex);
      shadowStackIndex++;
    }
    return;
  }

  if (auto arg = dyn_cast<Argument>(instrumentee)) {
    // Look up the function argument shadow stack index
    auto locIndex = computeShadowStackLocation(arg);
    std::tie(base, bound) = insertShadowStackLoad(builder, locIndex);
  }

  if (AllocaInst *alloc = dyn_cast<AllocaInst>(instrumentee)) {
    auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();

    base = builder.CreateConstGEP1_32(alloc, 0);
    if (auto inst = dyn_cast<Instruction>(base)) {
      setMetadata(inst, mdInfo, "sb.base.gep");
    }
    bound = builder.CreateConstGEP1_32(alloc, 1);
    if (auto inst = dyn_cast<Instruction>(bound)) {
      setMetadata(inst, mdInfo, "sb.bound.gep");
    }
  }

  if (auto constant = dyn_cast<Constant>(instrumentee)) {

    // If we have a constant initializer for an aggregate, witnesses for every
    // element have to be generated.
    if (constant->getType()->isAggregateType()) {
      auto indexToPtr =
          InstrumentationMechanism::getAggregatePointerIndicesAndValues(
              constant);
      for (auto KV : indexToPtr) {
        std::tie(base, bound) = getBoundsForWitness(cast<Constant>(KV.second));
        auto wi = std::make_shared<SoftBoundWitness>(base, bound, KV.second);
        target.setBoundWitness(wi, KV.first);
      }
      return;
    }
    std::tie(base, bound) = getBoundsForWitness(constant);
  }

  if (auto load = dyn_cast<LoadInst>(instrumentee)) {
    std::tie(base, bound) =
        insertMetadataLoad(builder, load->getPointerOperand());
  }

  if (isa<IntToPtrInst>(instrumentee)) {
    assert(IntToPtrHandling != BadPtrSrc::Disallow);
    std::tie(base, bound) = getBoundsForIntToPtrCast();
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

  target.setSingleBoundWitness(
      std::make_shared<SoftBoundWitness>(base, bound, instrumentee));
}

auto SoftBoundMechanism::getRelocatedClone(const Witness &w,
                                           Instruction *location) const
    -> WitnessPtr {

  if (auto wProxy = dyn_cast<SoftBoundVarArgWitness>(&w)) {
    return std::make_shared<SoftBoundVarArgWitness>(wProxy->getProxy());
  }

  return std::make_shared<SoftBoundWitness>(w.getLowerBound(),
                                            w.getUpperBound(), location);
}

auto SoftBoundMechanism::getWitnessPhi(PHINode *phi) const -> WitnessPtr {

  IRBuilder<> builder(phi);
  auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

  // Create a special witness when varargs are handled by this phi
  if (hasVarArgHandling(phi)) {
    auto *newPhi = builder.CreatePHI(handles.varArgProxyPtrTy,
                                     phi->getNumIncomingValues());
    setVarArgMetadata(newPhi, mdStr, "sb.vararg.proxy.phi");
    return std::make_shared<SoftBoundVarArgWitness>(newPhi);
  }

  auto *basePhi =
      builder.CreatePHI(handles.baseTy, phi->getNumIncomingValues());
  setMetadata(basePhi, mdStr, "sb.base.phi");
  auto *boundPhi =
      builder.CreatePHI(handles.boundTy, phi->getNumIncomingValues());
  setMetadata(boundPhi, mdStr, "sb.bound.phi");

  return std::make_shared<SoftBoundWitness>(basePhi, boundPhi, phi);
}

void SoftBoundMechanism::addIncomingWitnessToPhi(WitnessPtr &phi,
                                                 WitnessPtr &incoming,
                                                 BasicBlock *inBB) const {

  // Only one phi is required to propagate the vararg proxy
  if (SoftBoundVarArgWitness *phiProxy =
          dyn_cast<SoftBoundVarArgWitness>(phi.get())) {
    auto incomingProxy = cast<SoftBoundVarArgWitness>(incoming.get());

    auto phiSrc = cast<PHINode>(phiProxy->getProxy());
    auto incomingSrc = incomingProxy->getProxy();
    phiSrc->addIncoming(incomingSrc, inBB);
    return;
  }

  // For regular witnesses, propagate base and bound through their own phis
  auto *lowerPhi = cast<PHINode>(phi->getLowerBound());
  auto *upperPhi = cast<PHINode>(phi->getUpperBound());

  lowerPhi->addIncoming(incoming->getLowerBound(), inBB);
  upperPhi->addIncoming(incoming->getUpperBound(), inBB);
}

auto SoftBoundMechanism::getWitnessSelect(SelectInst *sel,
                                          WitnessPtr &trueWitness,
                                          WitnessPtr &falseWitness) const
    -> WitnessPtr {
  auto *cond = sel->getCondition();

  IRBuilder<> builder(sel);
  auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

  // Create a special witness when varargs are handled by this select
  if (auto trueProxy = dyn_cast<SoftBoundVarArgWitness>(trueWitness.get())) {
    auto falseProxy = dyn_cast<SoftBoundVarArgWitness>(falseWitness.get());

    auto *newSel = builder.CreateSelect(cond, trueProxy->getProxy(),
                                        falseProxy->getProxy());
    if (auto newSelInst = dyn_cast<Instruction>(newSel)) {
      setVarArgMetadata(newSelInst, mdStr, "sb.vararg.proxy.sel");
    }
    return std::make_shared<SoftBoundVarArgWitness>(newSel);
  }

  // For regular witnesses, propagate base and bound through their own selects
  auto *lowerSel = builder.CreateSelect(cond, trueWitness->getLowerBound(),
                                        falseWitness->getLowerBound());
  if (auto inst = dyn_cast<Instruction>(lowerSel)) {
    setMetadata(inst, mdStr, "sb.base.sel");
  }
  auto *upperSel = builder.CreateSelect(cond, trueWitness->getUpperBound(),
                                        falseWitness->getUpperBound());
  if (auto inst = dyn_cast<Instruction>(upperSel)) {
    setMetadata(inst, mdStr, "sb.bound.sel");
  }

  return std::make_shared<SoftBoundWitness>(lowerSel, upperSel, sel);
}

void SoftBoundMechanism::materializeBounds(ITarget &) {
  // The bounds are always materialized, nothing to do
}

void SoftBoundMechanism::insertCheck(ITarget &target) const {

  DEBUG_WITH_TYPE("softbound-genchecks",
                  dbgs() << "Insert check for: " << target << "\n";);

  if (target.isCheck()) {

    if (auto callC = dyn_cast<CallCheckIT>(&target)) {
      insertSpatialCallCheck(*callC);
      return;
    }

    insertSpatialDereferenceCheck(target);
    return;
  }

  assert(target.isInvariant());
  auto invIT = cast<InvariantIT>(&target);

  handleInvariant(*invIT);
}

bool SoftBoundMechanism::skipInstrumentation(Module &module) const {

  auto change = module.getFunction("main");

  // Even if the module should not be instrumented, we need to make sure to
  // rename the main function. Linking against the SoftBound run-time
  // causes a crash otherwise (multiple definitions of the main function).
  renameMain(module);
  return change;
}

auto SoftBoundMechanism::getFailFunction() const -> Value * {
  return handles.failFunction;
}

auto SoftBoundMechanism::getExtCheckCounterFunction() const -> Value * {
  if (InternalSoftBoundConfig::hasRunTimeStatsEnabled()) {
    return handles.externalCheckCounter;
  }
  llvm_unreachable("Misconfiguration: The run-time needs to be configured to "
                   "collect run-time statistics (ENABLE_RT_STATS) in order to "
                   "provide the external checks counter function.");
}

auto SoftBoundMechanism::getName() const -> const char * { return "SoftBound"; }

//===---------------------------- private ---------------------------------===//

void SoftBoundMechanism::replaceWrappedFunction(Module &module) const {

  // Rename all declarations of library functions that have a run-time wrapper
  for (auto &fun : module) {
    if (!fun.isDeclaration()) {
      continue;
    }
    assert(fun.hasName());
    if (fun.isIntrinsic()) {
      // Ignore intrinsics
      continue;
    }

    auto oldName = fun.getName();
    auto newName = InternalSoftBoundConfig::getWrappedName(oldName);
    if (oldName != newName) {
      fun.setName(newName);

      auto attrs = fun.getAttributes();
      auto newAttrs = updateNotPreservedAttributes(
          attrs, fun.getFunctionType()->getNumParams());
      fun.setAttributes(newAttrs);
    }
    LLVM_DEBUG(dbgs() << "Renamed function: " << fun.getName() << "\n");
  }

  updateCallAttributesForWrappedFunctions(module);
}

void SoftBoundMechanism::updateCallAttributesForWrappedFunctions(
    Module &module) const {

  for (auto &fun : module) {
    if (fun.isDeclaration()) {
      continue;
    }

    for (auto &block : fun) {
      for (auto &inst : block) {

        if (!isa<CallInst>(inst)) {
          continue;
        }
        auto &callInst = cast<CallInst>(inst);
        auto calledFun = callInst.getCalledFunction();
        if (!calledFun || !calledFun->hasName()) {
          continue;
        }

        if (InternalSoftBoundConfig::isWrappedName(calledFun->getName())) {
          auto attrs = callInst.getAttributes();
          auto newAttrs = updateNotPreservedAttributes(
              attrs, calledFun->getFunctionType()->getNumParams());
          callInst.setAttributes(newAttrs);
        }
      }
    }
  }
}

auto SoftBoundMechanism::updateNotPreservedAttributes(
    const AttributeList &attrs, int numArgs) const -> AttributeList {

  if (attrs.isEmpty()) {
    return attrs;
  }

  // The wrappers do not necessarily preserve those attributes, so we have
  // to drop them.
  // This is conservative, as the wrappers might preserve the attributes. Rely
  // on LTO to properly optimize the functions when the wrapper definitions are
  // available.
  // TODO it might be possible to replace `readnone` with
  // `inaccessiblememonly`, but it is unclear whether it is sound to
  // use this in combination with LTO, where the inaccessible memory becomes
  // available...
  auto attrsToDrop = {Attribute::ReadNone, Attribute::ReadOnly,
                      Attribute::WriteOnly, Attribute::ArgMemOnly};

  AttrBuilder funAttrBuilder(attrs.getFnAttributes());
  for (const auto &attrib : attrsToDrop) {
    funAttrBuilder.removeAttribute(attrib);
  }

  SmallVector<AttributeSet, 5> argAttrs;
  for (int i = 0; i < numArgs; i++) {
    AttrBuilder argAttrBuilder(attrs.getParamAttributes(i));
    for (const auto &attrib : attrsToDrop) {
      argAttrBuilder.removeAttribute(attrib);
    }
    argAttrs.push_back(AttributeSet::get(*context, argAttrBuilder));
  }

  return AttributeList::get(attrs.getContext(),
                            AttributeSet::get(*context, funAttrBuilder),
                            attrs.getRetAttributes(), argAttrs);
}

void SoftBoundMechanism::insertFunDecls(Module &module) {
  PrototypeInserter protoInserter(module);
  handles = protoInserter.insertRunTimeProtoypes();
  handles.highestAddr = determineHighestValidAddress();
}

void SoftBoundMechanism::insertMetadataAllocs(Module &module) {

  for (auto &fun : module) {
    if (fun.isDeclaration()) {
      continue;
    }

    // Use the beginning of the function to insert the allocas
    IRBuilder<> builder(&(*fun.getEntryBlock().getFirstInsertionPt()));
    auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

    auto allocBase = builder.CreateAlloca(handles.baseTy);
    setMetadata(allocBase, mdStr, "sb.base.alloc");
    auto allocBound = builder.CreateAlloca(handles.boundTy);
    setMetadata(allocBound, mdStr, "sb.bound.alloc");

    // Store the generated allocs for reuse
    metadataAllocs[&fun] = std::make_pair(allocBase, allocBound);
  }
}

void SoftBoundMechanism::renameMain(Module &module) const {

  Function *mainFun = module.getFunction("main");

  // This module does not have a main function, nothing to do here
  if (!mainFun) {
    return;
  }

  mainFun->setName("softboundcets_pseudo_main");
}

void SoftBoundMechanism::setUpGlobals(Module &module) const {

  // Look up the run-time init function
  auto initFun = handles.init;

  // Register a static constructor that collects all global allocation size
  // information
  auto listOfFuns = registerCtors(
      module,
      std::make_pair<StringRef, int>("__softboundcets_globals_setup", 0));
  auto globalSetupFun = (*listOfFuns)[0];

  setMetadata(globalSetupFun, InternalSoftBoundConfig::getSetupInfoStr());

  auto entryBlock = BasicBlock::Create(*context, "entry", globalSetupFun);

  IRBuilder<> builder(entryBlock);

  // Call the run-time init functions
  builder.CreateCall(FunctionCallee(initFun));

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
    std::tie(base, bound) = handleInitializer(glInit, builder, global, Indices);

    // An error already occurred, stop the initialization.
    if (globalConfig.hasErrors()) {
      break;
    }

    if (glInit->getType()->isPointerTy()) {
      assert(base && bound);
      LLVM_DEBUG({
        dbgs() << "\tBase: " << *base << "\n\tBound: " << *bound << "\n";
      });
      insertMetadataStore(builder, &global, base, bound);
    }
  }

  builder.CreateRetVoid();
}

auto SoftBoundMechanism::handleInitializer(Constant *glInit,
                                           IRBuilder<> &builder,
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
        if (IntToPtrHandling == BadPtrSrc::Disallow) {
          LLVM_DEBUG(
              dbgs() << "Integer to pointer cast found, report an error.\n";);
          globalConfig.noteError();
          return std::make_pair(base, bound);
        }
        return getBoundsForIntToPtrCast();
      }
      case Instruction::GetElementPtr:
        // Fall through
      case Instruction::BitCast: {
        // Recursively go through geps/bitcasts to find the base ptr
        auto *ptrOp = constExpr->getOperand(0);

        std::tie(base, bound) =
            handleInitializer(ptrOp, builder, topLevel, indices);
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
          handleInitializer(elem, builder, topLevel, tmpIndices);

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

        Value *gepToElem = builder.CreateInBoundsGEP(&topLevel, tmpIndices);
        insertMetadataStore(builder, gepToElem, subBase, subBound);
      }
    }

    return std::make_pair(base, bound);
  }

  llvm_unreachable("Unimplemented constant global initializer.");
}

void SoftBoundMechanism::insertVarArgWitness(IntermediateIT &target) const {
  auto instrumentee = target.getInstrumentee();
  auto location = target.getLocation();
  IRBuilder<> builder(location);
  auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

  if (auto load = dyn_cast<LoadInst>(instrumentee)) {
    assert(hasVarArgLoadArg(load));

    // Load from the allocas that hold base and bound
    AllocaInst *baseAlloc;
    AllocaInst *boundAlloc;
    std::tie(baseAlloc, boundAlloc) = metadataAllocs.at(load->getFunction());

    // Load base and bound
    auto base = builder.CreateLoad(baseAlloc);
    setVarArgMetadata(base, mdStr, "sb.vararg.base");

    auto bound = builder.CreateLoad(boundAlloc);
    setVarArgMetadata(bound, mdStr, "sb.vararg.bound");

    target.setSingleBoundWitness(
        std::make_shared<SoftBoundWitness>(base, bound, load));
    return;
  }

  LLVM_DEBUG(dbgs() << "Insert proxy witness for: " << *instrumentee << "\n";);

  Value *proxyPtr = nullptr;
  if (auto alloc = dyn_cast<AllocaInst>(instrumentee)) {

    // Create an allocation for the proxy object alongside the va_list
    builder.SetInsertPoint(alloc->getNextNode());
    auto proxyAlloc = builder.CreateAlloca(handles.varArgProxyTy);
    setVarArgMetadata(proxyAlloc, mdStr, "sb.vararg.proxy.alloc");
    proxyPtr = proxyAlloc;
  }

  if (Argument *arg = dyn_cast<Argument>(instrumentee)) {
    // Load the proxy object for this argument in the entry block, otherwise we
    // might conflict with calls later on in the function that build up new
    // layers on the shadow stack
    builder.SetInsertPoint(
        &(*arg->getParent()->getEntryBlock().getFirstInsertionPt()));

    // Load the proxy object from the shadow stack
    auto index = computeShadowStackLocation(arg);
    auto load = insertVarArgShadowStackLoad(builder, index);

    // Store the proxy to a local variable.
    // This ensures both va_list arguments to a function and regular vararg
    // functions both store the proxy in their own local variable, and can be
    // used uniformly by the metadata propagation.
    auto alloc = builder.CreateAlloca(handles.varArgProxyTy);
    setVarArgMetadata(alloc, mdStr, "sb.valist.proxy.alloc");

    // The store to the allocation can be as late as the target wants it to be
    builder.SetInsertPoint(location);
    auto store = builder.CreateStore(load, alloc);
    setVarArgMetadata(store, mdStr);

    proxyPtr = alloc;
  }

  if (isa<ConstantPointerNull>(instrumentee)) {
    // In case the va_list is null, propagate null metadata
    builder.SetInsertPoint(
        &(*location->getFunction()->getEntryBlock().getFirstInsertionPt()));

    auto alloc = builder.CreateAlloca(handles.varArgProxyTy);
    setVarArgMetadata(alloc, mdStr, "sb.valist.proxy.alloc");

    builder.SetInsertPoint(location->getNextNode());
    auto toStore = ConstantPointerNull::get(handles.varArgProxyTy);

    auto store = builder.CreateStore(toStore, alloc);
    setVarArgMetadata(store, mdStr);

    proxyPtr = alloc;
  }

  if (isa<GlobalVariable>(instrumentee)) {
    llvm_unreachable("Implementation for va_lists stored in globals missing");
  }

  if (!proxyPtr) {
    LLVM_DEBUG(dbgs() << "No proxy created for: " << *instrumentee << "\n"
                      << target << "\n";);
  }

  assert(proxyPtr);
  target.setSingleBoundWitness(
      std::make_shared<SoftBoundVarArgWitness>(proxyPtr));
}

void SoftBoundMechanism::handleInvariant(const InvariantIT &target) const {

  if (auto cInvIT = dyn_cast<CallInvariantIT>(&target)) {
    handleCallInvariant(*cInvIT);
    return;
  }

  assert(!isa<ArgInvariantIT>(target));

  Value *instrumentee = target.getInstrumentee();
  Instruction *loc = target.getLocation();

  IRBuilder<> builder(loc);

  // Stores of pointer values to memory require a metadata store.
  if (auto store = dyn_cast<StoreInst>(loc)) {
    assert(instrumentee->getType()->isPointerTy());
    // TODO This triggers if an aggregate is stored to memory
    // This is not too tricky to implement, but we had no use case for this so
    // far.
    builder.SetInsertPoint(loc->getNextNode());
    assert(target.getBoundWitnesses().size() == 1);
    const auto bw = target.getSingleBoundWitness();
    if (instrumentee->getType() != handles.voidPtrTy) {
      instrumentee = insertCast(handles.voidPtrTy, instrumentee, builder);
    }

    insertMetadataStore(builder, store->getPointerOperand(),
                        bw->getLowerBound(), bw->getUpperBound());

    DEBUG_WITH_TYPE("softbound-genchecks",
                    dbgs() << "Metadata for pointer store to memory saved.\n";);
    return;
  }

  if (isa<InsertValueInst>(loc)) {
    DEBUG_WITH_TYPE("softbound-genchecks", dbgs() << "Nothing to do.\n";);
    return;
  }

  // If a pointer is returned, its bounds need to be stored to the shadow stack.
  if (auto retInst = dyn_cast<ReturnInst>(loc)) {
    auto locs = computeIndices(retInst);
    unsigned shadowStackIndex = 0;
    auto bWitnesses = target.getBoundWitnesses();
    for (auto locIndex : locs) {
      auto bw = bWitnesses.at(locIndex);
      insertShadowStackStore(builder, bw->getLowerBound(), bw->getUpperBound(),
                             shadowStackIndex);
      shadowStackIndex++;
    }
    DEBUG_WITH_TYPE(
        "softbound-genchecks",
        dbgs() << "Returned pointer information stored to shadow stack.\n";);
    return;
  }

  if (isa<ValInvariantIT>(&target)) {
    auto varArgW =
        dyn_cast<SoftBoundVarArgWitness>(target.getSingleBoundWitness().get());
    assert(varArgW);

    // Look up which proxy to use
    auto proxyPtr = varArgW->getProxy();

    auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

    // Load the proxy
    auto load = builder.CreateLoad(proxyPtr);
    setVarArgMetadata(load, mdStr, "sb.vararg.proxy.load");

    AllocaInst *baseAlloc;
    AllocaInst *boundAlloc;
    std::tie(baseAlloc, boundAlloc) = metadataAllocs.at(loc->getFunction());

    // Load base and bound into the local variables
    SmallVector<Value *, 3> args = {load, baseAlloc, boundAlloc};
    auto loadCall = builder.CreateCall(
        FunctionCallee(handles.loadNextInfoVarArgProxy), args);
    setVarArgMetadata(loadCall, mdStr);
    return;
  }

  LLVM_DEBUG(dbgs() << target << "\n";);
  llvm_unreachable("Unexpected invariant target.");
}

void SoftBoundMechanism::handleCallInvariant(
    const CallInvariantIT &target) const {

  auto call = target.getCall();

  // Intrinsics might need additional calls for metadata copying, insert them
  if (isa<IntrinsicInst>(call)) {
    handleIntrinsicInvariant(target);
    return;
  }

  // The allocation call should be right before the call
  IRBuilder<> builder(call);

  // Take care of shadow stack allocation and deallocation
  handleShadowStackAllocation(call, builder);

  builder.SetInsertPoint(call);
  for (auto elem : target.getRequiredArgs()) {
    auto argNum = elem.first;
    auto *arg = elem.second;

    // Store the bounds to the shadow stack before the call
    auto locIndex = computeShadowStackLocation(call, argNum);

    // Look up the bound witness for this argument
    const auto bw = target.getBoundWitness(argNum);
    if (isVarArgMetadataType(arg->getType())) {
      auto mdStr = InternalSoftBoundConfig::getMetadataInfoStr();

      auto varArgWit = cast<SoftBoundVarArgWitness>(bw.get());
      auto loadedVal = builder.CreateLoad(varArgWit->getProxy());
      setVarArgMetadata(loadedVal, mdStr, "sb.vararg.proxy.load");

      insertVarArgShadowStackStore(builder, loadedVal, locIndex);
      continue;
    }

    insertShadowStackStore(builder, bw->getLowerBound(), bw->getUpperBound(),
                           locIndex);
    DEBUG_WITH_TYPE("softbound-genchecks",
                    dbgs() << "Passed pointer information for arg " << argNum
                           << " stored to shadow stack slot " << locIndex
                           << ".\n";);
  }
}

void SoftBoundMechanism::handleShadowStackAllocation(
    CallBase *call, IRBuilder<> &builder) const {

  // Compute how many arguments the shadow stack needs be capable to store
  auto size = computeSizeShadowStack(call);

  // Don't allocate anything if no metadata needs to be propagated
  if (size == 0) {
    return;
  }

  // Construct the metadata for the call
  auto shadowStackInfo = InternalSoftBoundConfig::getShadowStackInfoStr();

  // Allocate the shadow stack
  auto sizeVal = ConstantInt::get(handles.intTy, size, true);
  SmallVector<Value *, 1> args = {sizeVal};
  auto allocCall =
      builder.CreateCall(FunctionCallee(handles.allocateShadowStack), args);
  setMetadata(allocCall, shadowStackInfo);

  // Deallocate the shadow stack space after the call

  // Determine the location for the deallocation
  auto locAfter = getLastMDLocation(
      call, InternalSoftBoundConfig::getShadowStackLoadStr(), true);
  // Insertion always happens before the given instruction, so skip to the next
  // one. There always needs to be a next node, as our metadata annotated
  // instructions cannot be terminators.
  builder.SetInsertPoint(locAfter->getNextNode());

  // Deallocate the shadow stack
  auto deallocCall =
      builder.CreateCall(FunctionCallee(handles.deallocateShadowStack));
  setMetadata(deallocCall, shadowStackInfo);

  DEBUG_WITH_TYPE("softbound-genchecks",
                  dbgs() << "Allocate shadow stack: " << *allocCall
                         << "\nDeallocate it: " << *deallocCall << "\n";);
}

void SoftBoundMechanism::handleIntrinsicInvariant(
    const CallInvariantIT &target) const {

  IntrinsicInst *intrInst = cast<IntrinsicInst>(target.getCall());

  IRBuilder<> builder(intrInst);

  // Copy pointer metadata upon memcpy and memmove
  switch (intrInst->getIntrinsicID()) {
  case Intrinsic::memcpy:
  case Intrinsic::memmove: {
    // Set the call to copy metadata after the intrinsic to handle the intrinsic
    // the same way the memcpy run-time wrapper does.
    builder.SetInsertPoint(intrInst->getNextNode());
    SmallVector<Value *, 3> args;
    args.push_back(intrInst->getArgOperand(0));
    args.push_back(intrInst->getArgOperand(1));
    args.push_back(intrInst->getArgOperand(2));
    auto cpyCall =
        builder.CreateCall(FunctionCallee(handles.copyInMemoryMetadata), args);
    setMetadata(cpyCall, InternalSoftBoundConfig::getMetadataInfoStr());

    DEBUG_WITH_TYPE("softbound-genchecks", {
      dbgs() << "Inserted metadata copy: " << *cpyCall << "\n";
    });
    return;
  }
  case Intrinsic::memset: {
    // TODO FIXME what if an in-memory pointer is overwritten here?
    return;
  }
  case Intrinsic::vastart: {
    // Call the function to initialize the proxy object
    initializeVaListProxy(builder, target);
    return;
  }
  case Intrinsic::vacopy: {
    // Call the function to copy the proxy object
    copyVaListProxy(builder, target);
    return;
  }
  case Intrinsic::vaend: {
    // Call the function to free the proxy object
    freeVaListProxy(builder, target);
    return;
  }
  default:
    break;
  }

  llvm_unreachable("Invariant for unsupported intrinsic requested.");
}

void SoftBoundMechanism::initializeVaListProxy(
    IRBuilder<> &builder, const CallInvariantIT &callIT) const {

  auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();

  auto size = determineShadowStackLocationFirstVarArg(
      callIT.getLocation()->getFunction());
  auto sizeVal = ConstantInt::get(handles.intTy, size, true);
  SmallVector<Value *, 1> args = {sizeVal};
  auto allocProxy =
      builder.CreateCall(FunctionCallee(handles.allocateVarArgProxy), args);
  setVarArgMetadata(allocProxy, mdInfo, "sb.vararg.proxy.start");

  // Store to alloc prepared for this.
  auto varArgW =
      cast<SoftBoundVarArgWitness>(callIT.getBoundWitness(0).get())->getProxy();
  auto store = builder.CreateStore(allocProxy, varArgW);
  setVarArgMetadata(store, mdInfo);
  LLVM_DEBUG(dbgs() << "Module after insert of initializing alloc:\n"
                    << *store->getModule() << "\n";);
}

void SoftBoundMechanism::copyVaListProxy(IRBuilder<> &builder,
                                         const CallInvariantIT &callIT) const {

  auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();

  // Load the source proxy pointer
  SoftBoundVarArgWitness *srcWit =
      cast<SoftBoundVarArgWitness>(&(*callIT.getBoundWitness(1)));
  LLVM_DEBUG(dbgs() << "Witness value for cpy src: " << *srcWit->getProxy()
                    << "\n";);
  auto proxyPtr = builder.CreateLoad(srcWit->getProxy());
  setVarArgMetadata(proxyPtr, mdInfo, "sb.vararg.proxy.load");

  // Create a copy of the proxy object
  SmallVector<Value *, 1> args = {proxyPtr};
  auto copyCall =
      builder.CreateCall(FunctionCallee(handles.copyVarArgProxy), args);
  setVarArgMetadata(copyCall, mdInfo, "sb.vararg.proxy.copy");

  SoftBoundVarArgWitness *trgWit =
      cast<SoftBoundVarArgWitness>(&(*callIT.getBoundWitness(0)));

  // Store the copied proxy
  auto store = builder.CreateStore(copyCall, trgWit->getProxy());
  setVarArgMetadata(store, mdInfo);
  LLVM_DEBUG(dbgs() << "Witness value for cpy trg: " << *trgWit->getProxy()
                    << "\n";);
}

void SoftBoundMechanism::freeVaListProxy(IRBuilder<> &builder,
                                         const CallInvariantIT &callIT) const {

  auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();

  // Load the proxy pointer
  SoftBoundVarArgWitness *proxyWit =
      cast<SoftBoundVarArgWitness>(&(*callIT.getBoundWitness(0)));
  auto proxyPtr = builder.CreateLoad(proxyWit->getProxy());
  setVarArgMetadata(proxyPtr, mdInfo, "sb.vararg.proxy.load");

  // Free it
  SmallVector<Value *, 1> args = {proxyPtr};
  auto freeCall =
      builder.CreateCall(FunctionCallee(handles.freeVarArgProxy), args);
  setVarArgMetadata(freeCall, mdInfo);
}

auto SoftBoundMechanism::addBitCasts(IRBuilder<> builder, Value *base,
                                     Value *bound) const
    -> std::pair<Value *, Value *> {

  // Derive the metadata information for the bitcast from the incoming base
  // (the bound should have the same information).
  MDNode *mdNode = nullptr;
  if (auto inst = dyn_cast<Instruction>(base)) {
    mdNode = inst->getMetadata(InternalSoftBoundConfig::getMetadataKind());
  }

  if (base->getType() != handles.baseTy) {
    base = insertCast(handles.baseTy, base, builder);
    if (auto inst = dyn_cast<Instruction>(base)) {
      setMetadata(inst, mdNode);
    }
  }

  if (bound->getType() != handles.boundTy) {
    bound = insertCast(handles.boundTy, bound, builder);
    if (auto inst = dyn_cast<Instruction>(bound)) {
      setMetadata(inst, mdNode);
    }
  }

  return std::make_pair(base, bound);
}

auto SoftBoundMechanism::getBoundsConst(Constant *cons) const
    -> std::pair<Value *, Value *> {

  // Treat zero sized global arrays in a special way.
  if (auto globalVar = dyn_cast<GlobalVariable>(cons)) {
    PointerType *pt = cast<PointerType>(globalVar->getType());
    auto elemType = pt->getElementType();
    if (globalVar->isDeclaration()) {
      if (!elemType->isSized() || DL->getTypeSizeInBits(elemType) == 0) {
        return getBoundsForZeroSizedArray(globalVar);
      }
    }
  }

  IRBuilder<> builder(*context);
  auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();

  auto *base = builder.CreateConstGEP1_32(cons, 0);
  if (auto baseInst = dyn_cast<Instruction>(base)) {
    setMetadata(baseInst, mdInfo, "sb.base.gep");
  }
  auto *bound = builder.CreateConstGEP1_32(cons, 1);
  if (auto boundInst = dyn_cast<Instruction>(base)) {
    setMetadata(boundInst, mdInfo, "sb.bound.gep");
  }

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

auto SoftBoundMechanism::getBoundsForIntToPtrCast() const
    -> std::pair<Value *, Value *> {
  if (IntToPtrHandling == BadPtrSrc::Disallow) {
    return std::make_pair(nullptr, nullptr);
  }
  if (IntToPtrHandling == BadPtrSrc::NullBounds) {
    return getNullPtrBounds();
  }
  if (IntToPtrHandling == BadPtrSrc::WideBounds) {
    return getWideBounds();
  }
  llvm_unreachable("Unknown option set for IntToPtr handling.");
}

auto SoftBoundMechanism::getBoundsForZeroSizedArray(GlobalVariable *gv) const
    -> std::pair<Value *, Value *> {

  ++ZeroSizedArrayBoundsRequested;

  LLVM_DEBUG(dbgs() << "Deal with zero sized array: " << *gv << "\n";);

  if (ZeroSizedArrayHandling == BadPtrSrc::Disallow) {
    return std::make_pair(nullptr, nullptr);
  }

  IRBuilder<> builder(*context);
  Value *base = gv;

  if (ZeroSizedArrayHandling == BadPtrSrc::NullBounds) {
    Value *bound = gv;
    std::tie(base, bound) = addBitCasts(builder, base, bound);
    return std::make_pair(base, bound);
  }

  if (ZeroSizedArrayHandling == BadPtrSrc::WideBounds) {
    Value *wide_ub;
    std::tie(std::ignore, wide_ub) = getWideBounds();
    std::tie(base, wide_ub) = addBitCasts(builder, base, wide_ub);
    return std::make_pair(base, wide_ub);
  }
  llvm_unreachable("Unknown option set for IntToPtr handling.");
}

auto SoftBoundMechanism::getNullPtrBounds() const
    -> std::pair<Value *, Value *> {
  ++NullptrBoundsRequested;

  auto base = ConstantPointerNull::get(handles.baseTy);
  auto bound = ConstantPointerNull::get(handles.boundTy);

  return std::make_pair(base, bound);
}

auto SoftBoundMechanism::getWideBounds() const -> std::pair<Value *, Value *> {
  ++WideBoundsRequested;

  auto base = ConstantPointerNull::get(handles.baseTy);

  // The bound value is a pointer with the highest valid address.
  auto suitableIntType = DL->getIntPtrType(handles.boundTy);
  auto maxAddressInt =
      ConstantInt::get(suitableIntType, handles.highestAddr, false);
  auto bound = ConstantExpr::getIntToPtr(maxAddressInt, handles.boundTy);
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

  // The bounds for an integer casted to a pointer are requested, no bounds are
  // available. Decide based on the given policy what to do.
  if (isa<ConstantInt>(constant)) {
    assert(IntToPtrHandling != BadPtrSrc::Disallow);
    return getBoundsForIntToPtrCast();
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

  // Get the allocations for the metadata load call to store base and bound in
  AllocaInst *allocBase = nullptr;
  AllocaInst *allocBound = nullptr;
  std::tie(allocBase, allocBound) =
      metadataAllocs.at(builder.GetInsertPoint()->getFunction());

  auto mdInfo = InternalSoftBoundConfig::getMetadataInfoStr();
  LLVM_DEBUG(dbgs() << "Insert metadata load:\n"
                    << "\tPtr: " << *ptr << "\n\tBaseAlloc: " << *allocBase
                    << "\n\tBoundAlloc: " << *allocBound << "\n";);

  SmallVector<Value *, 3> args = {ptr, allocBase, allocBound};

  auto call =
      builder.CreateCall(FunctionCallee(handles.loadInMemoryPtrInfo), args);
  setMetadata(call, mdInfo);

  auto base = builder.CreateLoad(allocBase);
  setMetadata(base, mdInfo, "sb.base.load");
  auto bound = builder.CreateLoad(allocBound);
  setMetadata(bound, mdInfo, "sb.bound.load");

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

  SmallVector<Value *, 3> args = {ptr, base, bound};
  auto call =
      builder.CreateCall(FunctionCallee(handles.storeInMemoryPtrInfo), args);
  setMetadata(call, InternalSoftBoundConfig::getMetadataInfoStr());
}

auto SoftBoundMechanism::insertShadowStackLoad(IRBuilder<> &builder,
                                               int locIndex) const
    -> std::pair<Value *, Value *> {

  auto locIndexVal = ConstantInt::get(handles.intTy, locIndex, true);

  auto mdStr = InternalSoftBoundConfig::getShadowStackLoadStr();
  SmallVector<Value *, 1> args = {locIndexVal};
  auto callLoadBase =
      builder.CreateCall(FunctionCallee(handles.loadBaseStack), args);
  setMetadata(callLoadBase, mdStr, "sb.base.load");
  auto callLoadBound =
      builder.CreateCall(FunctionCallee(handles.loadBoundStack), args);
  setMetadata(callLoadBound, mdStr, "sb.bound.load");

  return std::make_pair(callLoadBase, callLoadBound);
}

void SoftBoundMechanism::insertShadowStackStore(IRBuilder<> &builder,
                                                Value *lowerBound,
                                                Value *upperBound,
                                                int locIndex) const {

  auto mdStr = InternalSoftBoundConfig::getShadowStackStoreStr();

  auto locIndexVal = ConstantInt::get(handles.intTy, locIndex, true);

  SmallVector<Value *, 2> argsBase = {lowerBound, locIndexVal};
  SmallVector<Value *, 2> argsBound = {upperBound, locIndexVal};
  auto callStoreBase =
      builder.CreateCall(FunctionCallee(handles.storeBaseStack), argsBase);
  setMetadata(callStoreBase, mdStr);
  auto callStoreBound =
      builder.CreateCall(FunctionCallee(handles.storeBoundStack), argsBound);
  setMetadata(callStoreBound, mdStr);

  LLVM_DEBUG(dbgs() << "\tBase store: " << *callStoreBase
                    << "\n\tBound store: " << *callStoreBound << "\n";);
}

auto SoftBoundMechanism::insertVarArgShadowStackLoad(IRBuilder<> &builder,
                                                     int index) const
    -> Instruction * {

  auto mdStr = InternalSoftBoundConfig::getShadowStackLoadStr();

  // Insert calls to the C run-time that look up the proxy pointer
  auto indexVal = ConstantInt::get(handles.intTy, index, true);
  SmallVector<Value *, 1> args = {indexVal};
  auto proxyPtr =
      builder.CreateCall(FunctionCallee(handles.loadVarArgProxyStack), args);
  setVarArgMetadata(proxyPtr, mdStr, "sb.vararg.proxy.load");

  return proxyPtr;
}

void SoftBoundMechanism::insertVarArgShadowStackStore(IRBuilder<> &builder,
                                                      Value *proxyPtr,
                                                      int index) const {

  auto mdStr = InternalSoftBoundConfig::getShadowStackStoreStr();

  // Insert calls to the C run-time that look up the proxy pointer
  auto indexVal = ConstantInt::get(handles.intTy, index, true);
  SmallVector<Value *, 2> args = {proxyPtr, indexVal};
  auto storeCall =
      builder.CreateCall(FunctionCallee(handles.storeVarArgProxyStack), args);
  setVarArgMetadata(storeCall, mdStr);
}

auto SoftBoundMechanism::computeShadowStackLocation(const Argument *arg) const
    -> unsigned {
  assert(arg->getType()->isPointerTy());

  unsigned shadowStackLoc = 0;

  // Skip shadow stack locations of returned values
  auto fun = arg->getParent();
  shadowStackLoc += determineNumberOfPointers(fun->getReturnType());

  for (const auto &funArg : fun->args()) {

    if (!funArg.getType()->isPointerTy()) {
      // Non-pointer arguments are not accounted for in the location
      // calculation
      continue;
    }

    // Skip metadata arguments
    if (funArg.getType()->isMetadataTy()) {
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

auto SoftBoundMechanism::computeShadowStackLocation(const CallBase *call,
                                                    unsigned argNum) const
    -> unsigned {
  unsigned shadowStackLoc = 0;

  // Skip shadow stack locations of returned values
  shadowStackLoc += determineNumberOfPointers(call->getType());

  for (const auto &use : call->args()) {

    if (!use->getType()->isPointerTy()) {
      // Non-pointer arguments are not accounted for in the location
      // calculation
      continue;
    }

    // Skip metadata arguments
    if (use->getType()->isMetadataTy()) {
      continue;
    }

    if (argNum == use.getOperandNo()) {
      return shadowStackLoc;
    }
    shadowStackLoc++;
  }

  llvm_unreachable(
      "Argument not found, shadow stack location cannot be determined");
}

auto SoftBoundMechanism::determineNumberOfPointers(const Type *ty) const
    -> unsigned {

  if (ty->isPointerTy()) {
    return 1;
  }

  if (ty->isAggregateType()) {
    unsigned numPtrs = 0;

    if (const auto *sTy = dyn_cast<StructType>(ty)) {
      for (const auto &elem : sTy->elements()) {
        if (elem->isPointerTy()) {
          numPtrs++;
        }
      }
    }

    if (const auto *aTy = dyn_cast<ArrayType>(ty)) {
      numPtrs += aTy->getNumElements();
    }

    return numPtrs;
  }

  return 0;
}

/// Determine the shadow stack location at which the va args start
auto SoftBoundMechanism::determineShadowStackLocationFirstVarArg(
    const Function *fun) const -> unsigned {
  assert(fun->isVarArg());

  unsigned shadowStackLoc = 0;

  // Skip shadow stack locations of returned values
  shadowStackLoc += determineNumberOfPointers(fun->getReturnType());

  for (const auto &use : fun->args()) {

    if (!use.getType()->isPointerTy()) {
      // Non-pointer arguments are not accounted for in the location
      // calculation
      continue;
    }

    // Skip metadata arguments
    if (use.getType()->isMetadataTy()) {
      continue;
    }

    shadowStackLoc++;
  }

  return shadowStackLoc;
}

auto SoftBoundMechanism::computeIndices(const Instruction *inst) const
    -> SmallVector<unsigned, 1> {
  assert(isa<CallBase>(inst) || isa<ReturnInst>(inst));

  const Type *ty = nullptr;
  if (auto cb = dyn_cast<CallBase>(inst)) {
    ty = cb->getType();
  }

  if (auto ri = dyn_cast<ReturnInst>(inst)) {
    ty = ri->getReturnValue()->getType();
  }

  SmallVector<unsigned, 1> indices;
  if (!ty->isAggregateType()) {
    assert(ty->isPointerTy());
    indices.push_back(0);
    return indices;
  }

  unsigned index = 0;
  if (const auto *sTy = dyn_cast<StructType>(ty)) {
    for (const auto &elem : sTy->elements()) {
      if (elem->isPointerTy()) {
        indices.push_back(index);
      }
      index++;
    }
  }

  if (const auto *aTy = dyn_cast<ArrayType>(ty)) {
    assert(aTy->getElementType()->isPointerTy());
    int num = aTy->getNumElements();
    for (int i = 0; i < num; i++) {
      indices.push_back(i);
    }
  }

  assert(indices.size() == determineNumberOfPointers(ty));
  assert(indices.size() > 0);
  return indices;
}

auto SoftBoundMechanism::computeSizeShadowStack(const CallBase *call) const
    -> int {

  int size = 0;

  // Determine the number of returned pointer values
  size += determineNumberOfPointers(call->getType());

  // Count every real pointer argument
  for (const auto &use : call->args()) {

    if (!use->getType()->isPointerTy()) {
      // Non-pointer arguments are not accounted for in the location
      // calculation
      continue;
    }

    // Skip metadata arguments
    if (use->getType()->isMetadataTy()) {
      continue;
    }

    size++;
  }

  return size;
}

void SoftBoundMechanism::insertSpatialDereferenceCheck(
    const ITarget &target) const {
  assert(isa<ConstSizeCheckIT>(&target) || isa<VarSizeCheckIT>(&target));

  auto *instrumentee = target.getInstrumentee();
  IRBuilder<> builder(target.getLocation());

  Value *size = nullptr;
  if (auto constSIT = dyn_cast<ConstSizeCheckIT>(&target)) {
    size = ConstantInt::get(handles.sizeTTy, constSIT->getAccessSize());
  }

  if (auto varSIT = dyn_cast<VarSizeCheckIT>(&target)) {
    size = varSIT->getAccessSizeVal();
    if (size->getType() != handles.sizeTTy) {
      size = insertCast(handles.sizeTTy, size, builder);
    }
  }

  if (instrumentee->getType() != handles.voidPtrTy) {
    instrumentee = insertCast(handles.voidPtrTy, instrumentee, builder);
  }

  assert(size);

  const auto bw = target.getSingleBoundWitness();
  SmallVector<Value *, 4> args = {bw->getLowerBound(), bw->getUpperBound(),
                                  instrumentee, size};

  DEBUG_WITH_TYPE("softbound-genchecks",
                  dbgs() << "\tLB: " << *bw->getLowerBound()
                         << "\n\tUB: " << *bw->getUpperBound() << "\n\tinstr: "
                         << *instrumentee << "\n\tsize: " << *size << "\n";);
  auto call = builder.CreateCall(FunctionCallee(handles.spatialCheck), args);
  setMetadata(call, InternalSoftBoundConfig::getCheckInfoStr());

  DEBUG_WITH_TYPE("softbound-genchecks",
                  dbgs() << "Generated check: " << *call << "\n";);
}

void SoftBoundMechanism::insertSpatialCallCheck(
    const CallCheckIT &target) const {

  IRBuilder<> builder(target.getLocation());

  auto *instrumentee = target.getInstrumentee();

  if (instrumentee->getType() != handles.voidPtrTy) {
    instrumentee = insertCast(handles.voidPtrTy, instrumentee, builder);
  }

  const auto bw = target.getSingleBoundWitness();
  SmallVector<Value *, 3> args = {bw->getLowerBound(), bw->getUpperBound(),
                                  instrumentee};

  auto call =
      builder.CreateCall(FunctionCallee(handles.spatialCallCheck), args);
  setMetadata(call, InternalSoftBoundConfig::getCheckInfoStr());

  DEBUG_WITH_TYPE("softbound-genchecks",
                  dbgs() << "Generated check: " << *call << "\n";);
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
    ++ExceptionHandlingInst;
    return true;
  }

  switch (opC) {
  case Instruction::CatchPad:
  case Instruction::CleanupPad:
  case Instruction::LandingPad:
    ++ExceptionHandlingInst;
    return true;
  case Instruction::IntToPtr: {
    if (IntToPtrHandling == BadPtrSrc::Disallow) {
      ++IntToPtrCast;
      return true;
    }
    break;
  }
  default:
    break;
  }
  return false;
}

auto SoftBoundMechanism::determineHighestValidAddress() const -> uintptr_t {

  // Compute the highest valid address.
  auto ptrSize = DL->getPointerSizeInBits();

  // TODO It would be nice if there was some TT function that could provide
  // this information...

  // In case the pointer width is 64bit, the actual highest address is only
  // 2^48 (-1) (at least on x86_64), adapt the calculation accordingly.
  if (ptrSize == 64) {
    ptrSize = 48;
  }
  return pow(2, ptrSize) - 1;
}

void SoftBoundMechanism::checkModule(Module &module) {
  for (const Function &fun : module) {

    if (fun.hasAvailableExternallyLinkage()) {
      // Function which are `available_externally` conflict with our goal to
      // wrap standard library functions. These functions may include kind of
      // inlined standard library functions, but no code for them will be
      // emitted, so we cannot instrument them instead of calling the wrapper
      // (as there is - to the best of my knowledge - no guarantee that the
      // function will be inlined).
      LLVM_DEBUG({
        dbgs() << "Function " << fun.getName()
               << " with `available_externally` linkage found, this should be "
                  "prevented by the `EliminateAvailableExternallyPass` pass.\n";
      });
      globalConfig.noteError();
    }

    for (const auto &arg : fun.args()) {
      if (arg.hasInAllocaAttr()) {
        ++InAllocaArg;
        globalConfig.noteError();
      }
    }

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

void SoftBoundMechanism::setMetadata(Instruction *inst, MDNode *mdNode,
                                     const StringRef name) const {
  if (mdNode) {
    inst->setMetadata(InternalSoftBoundConfig::getMetadataKind(), mdNode);
  }
  setNoInstrument(inst);
  if (!name.empty()) {
    inst->setName(name);
  }
}

void SoftBoundMechanism::setMetadata(GlobalObject *glObj,
                                     const StringRef mdtext,
                                     const StringRef name) const {
  InternalSoftBoundConfig::setSoftBoundMetadata(glObj, mdtext);
  setNoInstrument(glObj);
  if (!name.empty()) {
    glObj->setName(name);
  }
}

void SoftBoundMechanism::setMetadata(Instruction *inst, const StringRef mdtext,
                                     const StringRef name) const {
  InternalSoftBoundConfig::setSoftBoundMetadata(inst, mdtext);
  setNoInstrument(inst);
  if (!name.empty()) {
    inst->setName(name);
  }
}

void SoftBoundMechanism::setVarArgMetadata(Instruction *inst,
                                           const StringRef mdtext,
                                           const StringRef name) const {
  setMetadata(inst, mdtext, name);
  setVarArgHandling(inst);
}

auto SoftBoundMechanism::getLastMDLocation(Instruction *start,
                                           StringRef nodeString,
                                           bool forward) const
    -> Instruction * {

  auto loc = start;
  while (loc) {
    Instruction *nextLoc;
    if (forward) {
      nextLoc = loc->getNextNode();
    } else {
      nextLoc = loc->getPrevNode();
    }

    if (!nextLoc) {
      break;
    }
    auto md = nextLoc->getMetadata(InternalSoftBoundConfig::getMetadataKind());

    // If this is not a shadow stack store, we found our insert location
    if (!md) {
      break;
    }

    auto nodeStringFound = false;
    for (unsigned i = 0; i < md->getNumOperands(); i++) {
      if (const auto *opStr = dyn_cast<MDString>(md->getOperand(i))) {
        if (opStr->getString().equals(nodeString)) {
          nodeStringFound = true;
          break;
        }
      }
    }

    if (!nodeStringFound) {
      break;
    }

    loc = nextLoc;
  }
  return loc;
}
