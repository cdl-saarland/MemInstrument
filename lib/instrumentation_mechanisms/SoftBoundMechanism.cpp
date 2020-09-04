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
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "softbound"

// Stores of pointers into memory require that the base and bound are tracked,
// such that upon loading and using the pointer afterwards, they can be looked
// up.
STATISTIC(MetadataStoresInserted, "Number of metadata stores inserted");

STATISTIC(NullptrBoundsRequested, "Number of nullptr bounds requested");
STATISTIC(FunctionBoundsRequested, "Number of function bounds requested");

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

void SoftBoundMechanism::insertWitness(ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::relocCloneWitness(Witness &W, ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::insertCheck(ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::materializeBounds(ITarget &Target) {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getExtCheckCounterFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getVerboseFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::insertWitnessPhi(ITarget &Target) const
    -> std::shared_ptr<Witness> {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::addIncomingWitnessToPhi(
    std::shared_ptr<Witness> &Phi, std::shared_ptr<Witness> &Incoming,
    BasicBlock *InBB) const {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const -> std::shared_ptr<Witness> {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::initialize(Module &module) {

  // Currently only spatial safety is implemented, don't do anything if temporal
  // memory safety is requested.
  if (!InternalSoftBoundConfig::ensureOnlySpatial()) {
    globalConfig.noteError();
    return;
  }

  // Store the context to avoid looking it up all the time
  context = &module.getContext();

  // Insert the declarations for basic metadata and check functions
  insertFunDecls(module);

  // Generate setup function that inserts metadata stores for global variables
  setUpGlobals(module);

  if (globalConfig.hasErrors()) {
    ++SetupErrror;
  }
}

auto SoftBoundMechanism::getName() const -> const char * { return "SoftBound"; }

//===-------------------------- private -----------------------------------===//

/// Insert the declarations for SoftBound metadata propagation functions
void SoftBoundMechanism::insertFunDecls(Module &module) {

  PrototypeInserter protoInserter(module);
  handles = protoInserter.insertRunTimeProtoypes();
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

  MDNode *node = MDNode::get(*context, MDString::get(*context, "Setup"));
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
    std::tie(base, bound) = handleInitializer(glInit, Builder, global, Indices);

    // An error already occurred, stop the initialization.
    if (globalConfig.hasErrors()) {
      break;
    }

    if (glInit->getType()->isPointerTy()) {
      assert(base && bound);
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

  LLVM_DEBUG(dbgs() << "Handle initializer " << *glInit << "\n";);

  if (isa<BlockAddress>(glInit)) {
    std::tie(base, bound) = getBoundsConst(glInit);
    return std::make_pair(base, bound);
  }

  if (isa<GlobalIndirectSymbol>(glInit)) {
    globalConfig.noteError();
    return std::make_pair(base, bound);
  }

  // Return null bounds if a nullptr is explicitly stores somewhere
  if (glInit->isNullValue() || glInit->getType()->isPointerTy()) {
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

auto SoftBoundMechanism::getBoundsConst(Constant *cons) const
    -> std::pair<Value *, Value *> {

  IRBuilder<> Builder(*context);
  Value *base = Builder.CreateConstGEP1_32(cons, 0);
  Value *bound = Builder.CreateConstGEP1_32(cons, 1);

  LLVM_DEBUG(dbgs() << "Create bounds for: " << *cons << "\n\tBase: " << *base
                    << "\n\tBound: " << *bound << "\n";);
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

void SoftBoundMechanism::insertMetadataStore(IRBuilder<> &Builder, Value *ptr,
                                             Value *base, Value *bound) const {

  ++MetadataStoresInserted;

  assert(ptr && base && bound);

  if (handles.voidPtrTy != ptr->getType()) {
    ptr = Builder.CreateBitCast(ptr, handles.voidPtrTy);
  }

  if (handles.baseTy != base->getType()) {
    base = Builder.CreateBitCast(base, handles.baseTy);
  }

  if (handles.boundTy != bound->getType()) {
    bound = Builder.CreateBitCast(bound, handles.boundTy);
  }

  LLVM_DEBUG(dbgs() << "Insert metadata store:\n"
                    << "\tPtr: " << *ptr << "\n\tBase: " << *base
                    << "\n\tBound: " << *bound << "\n";);

  ArrayRef<Value *> args = {ptr, base, bound};
  auto call =
      Builder.CreateCall(FunctionCallee(handles.storeInMemoryPtrInfo), args);

  MDNode *node = MDNode::get(*context, MDString::get(*context, "Metadata"));
  call->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);
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
