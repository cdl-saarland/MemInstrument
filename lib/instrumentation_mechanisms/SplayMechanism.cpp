//===---------------------------------------------------------------------===///
///
/// \file See corresponding header for more descriptions.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"

#include "meminstrument/Config.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/pass/Util.h"

STATISTIC(SplayNumInboundsChecks, "The # of inbounds checks inserted");
STATISTIC(SplayNumDereferenceChecks, "The # of dereference checks inserted");
STATISTIC(SplayNumBounds, "The # of bound(pairs) materialized");
STATISTIC(SplayNumWitnessPhis, "The # of witness phis inserted");
STATISTIC(SplayNumWitnessSelects, "The # of witness selects inserted");
STATISTIC(SplayNumWitnessLookups, "The # of witness lookups inserted");
STATISTIC(SplayNumGlobals, "The # of globals registered");
STATISTIC(SplayNumNonSizedGlobals,
          "The # of globals non-sized globals ignored");
STATISTIC(SplayNumFunctions, "The # of functions registered");
STATISTIC(SplayNumAllocas, "The # of allocas registered");

using namespace llvm;
using namespace meminstrument;

// FIXME currently, all out-of-bounds pointers are marked invalid here,
// including legal one-after-allocation ones.

Value *SplayWitness::getLowerBound(void) const { return LowerBound; }

Value *SplayWitness::getUpperBound(void) const { return UpperBound; }

SplayWitness::SplayWitness(Value *WitnessValue, Instruction *Location)
    : Witness(WK_Splay), WitnessValue(WitnessValue), Location(Location) {}

Instruction *SplayWitness::getInsertionLocation() const {
  auto *Res = Location;
  while (isa<PHINode>(Res)) {
    Res = Res->getNextNode();
  }
  return Res;
}

bool SplayWitness::hasBoundsMaterialized(void) const {
  return UpperBound != nullptr && LowerBound != nullptr;
}

void SplayMechanism::insertWitnesses(ITarget &Target) const {
  // There should be no targets without an instrumentee for splay
  assert(Target.hasInstrumentee());

  auto instrumentee = Target.getInstrumentee();

  assert(!isa<ExtractValueInst>(instrumentee));

  if (!instrumentee->getType()->isAggregateType()) {
    auto *CastVal = insertCast(WitnessType, Target.getInstrumentee(),
                               Target.getLocation(), "_witness");
    Target.setSingleBoundWitness(
        std::make_shared<SplayWitness>(CastVal, Target.getLocation()));

    ++SplayNumWitnessLookups;
    return;
  }

  // The witnesses for pointers returned from a call/landingpad are the values
  // in the aggregate themselves.
  if (isa<CallBase>(instrumentee) || isa<LandingPadInst>(instrumentee)) {
    IRBuilder<> builder(Target.getLocation());
    // Find all locations of pointer values in the aggregate type
    auto indices = computePointerIndices(instrumentee->getType());
    for (auto index : indices) {
      // Extract the pointer to have the witness at hand.
      auto ptr = builder.CreateExtractValue(instrumentee, index);
      auto *castVal = insertCast(WitnessType, ptr, builder);
      Target.setBoundWitness(
          std::make_shared<SplayWitness>(castVal, Target.getLocation()), index);
    }
    return;
  }

  // The only aggregates that do not need a source are those that are constant
  assert(isa<Constant>(instrumentee));
  auto con = cast<Constant>(instrumentee);

  auto indexToPtr =
      InstrumentationMechanism::getAggregatePointerIndicesAndValues(con);
  for (auto KV : indexToPtr) {
    auto *CastVal =
        insertCast(WitnessType, KV.second, Target.getLocation(), "_witness");
    Target.setBoundWitness(
        std::make_shared<SplayWitness>(CastVal, Target.getLocation()),
        KV.first);

    ++SplayNumWitnessLookups;
  }
}

WitnessPtr SplayMechanism::getRelocatedClone(const Witness &wit,
                                             Instruction *location) const {
  const auto *splayWit = dyn_cast<SplayWitness>(&wit);
  assert(splayWit != nullptr);

  ++SplayNumWitnessLookups;

  return std::make_shared<SplayWitness>(splayWit->WitnessValue, location);
}

void SplayMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck() || Target.isInvariant());

  Module *M = Target.getLocation()->getModule();
  bool Verbose = globalConfig.hasInstrumentVerbose();

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<SplayWitness>(Target.getSingleBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

  Value *NameVal = nullptr;
  if (Verbose) {
    std::string Name;
    raw_string_ostream ss(Name);
    ss << *Target.getLocation() << "(";
    if (DILocation *Loc = Target.getLocation()->getDebugLoc()) {
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      ss << (File + ": " + std::to_string(Line)).str();
    } else {
      ss << "unknown location";
    }
    ss << ", idx:";
    static uint64_t i = 0;
    ss << i++;
    ss << ")";
    ss.str();

    auto *Str = insertStringLiteral(*M, Name);
    NameVal = insertCast(PtrArgType, Str, Builder);
  }

  if (Target.isCheck()) {

    // Determine the access width depending on the target kind
    Value *Size = nullptr;
    if (isa<CallCheckIT>(&Target)) {
      Size = ConstantInt::get(SizeType, 1);
    }
    if (auto constSizeTarget = dyn_cast<ConstSizeCheckIT>(&Target)) {
      Size = ConstantInt::get(SizeType, constSizeTarget->getAccessSize());
    }
    if (auto varSizeTarget = dyn_cast<VarSizeCheckIT>(&Target)) {
      Size = varSizeTarget->getAccessSizeVal();
    }

    assert(Size);

    if (Verbose) {
      insertCall(Builder, CheckDereferenceFunction,
                 std::vector<Value *>{WitnessVal, CastVal, Size, NameVal});
    } else {
      insertCall(Builder, CheckDereferenceFunction,
                 std::vector<Value *>{WitnessVal, CastVal, Size});
    }
    ++SplayNumDereferenceChecks;
  } else {
    assert(Target.isInvariant());
    if (Verbose) {
      insertCall(Builder, CheckInboundsFunction,
                 std::vector<Value *>{WitnessVal, CastVal, NameVal});
    } else {
      insertCall(Builder, CheckInboundsFunction,
                 std::vector<Value *>{WitnessVal, CastVal});
    }
    ++SplayNumInboundsChecks;
  }
}

void SplayMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());

  auto witnesses = Target.getBoundWitnesses();

  for (auto kv : witnesses) {

    auto *Witness = cast<SplayWitness>(kv.second.get());

    if (Witness->hasBoundsMaterialized()) {
      continue;
    }

    auto *WitnessVal = Witness->WitnessValue;

    const auto key = std::tuple<const Instruction *, Value *, unsigned>(
        Target.getLocation(), Target.getInstrumentee(), kv.first);
    const auto &lookup = MaterializedBounds.find(key);
    if (lookup != MaterializedBounds.end()) {
      auto &map_value = lookup->second;
      Value *lower = map_value.first;
      Value *upper = map_value.second;
      Witness->LowerBound = lower;
      Witness->UpperBound = upper;
      continue;
    }

    IRBuilder<> Builder(Witness->getInsertionLocation());

    if (Target.hasUpperBoundFlag()) {
      auto *UpperVal =
          insertCall(Builder, GetUpperBoundFunction, WitnessVal, "upper_bound");
      Witness->UpperBound = UpperVal;
    }
    if (Target.hasLowerBoundFlag()) {
      auto *LowerVal =
          insertCall(Builder, GetLowerBoundFunction, WitnessVal, "lower_bound");
      Witness->LowerBound = LowerVal;
    }

    MaterializedBounds.emplace(
        std::make_tuple(Target.getLocation(), Target.getInstrumentee(),
                        kv.first),
        std::make_pair(Witness->LowerBound, Witness->UpperBound));

    ++SplayNumBounds;
  }
}

Value *SplayMechanism::getFailFunction(void) const { return FailFunction; }

Value *SplayMechanism::getExtCheckCounterFunction(void) const {
  return ExtCheckCounterFunction;
}

Value *SplayMechanism::getVerboseFailFunction(void) const {
  return VerboseFailFunction;
}

void SplayMechanism::insertFunctionDeclarations(Module &M) {
  auto &Ctx = M.getContext();
  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *StringTy = Type::getInt8PtrTy(Ctx);

  bool Verbose = globalConfig.hasInstrumentVerbose();

  if (Verbose) {
    CheckInboundsFunction =
        insertFunDecl(M, "__splay_check_inbounds_named", VoidTy, WitnessType,
                      PtrArgType, StringTy);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference_named", VoidTy, WitnessType,
                      PtrArgType, SizeType, StringTy);
    GlobalAllocFunction =
        insertFunDecl(M, "__splay_alloc_or_merge_with_msg", VoidTy, PtrArgType,
                      SizeType, PtrArgType);
    AllocFunction = insertFunDecl(M, "__splay_alloc_or_replace_with_msg",
                                  VoidTy, PtrArgType, SizeType, PtrArgType);
  } else {
    CheckInboundsFunction = insertFunDecl(M, "__splay_check_inbounds", VoidTy,
                                          WitnessType, PtrArgType);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference", VoidTy, WitnessType,
                      PtrArgType, SizeType);
    GlobalAllocFunction = insertFunDecl(M, "__splay_alloc_or_merge", VoidTy,
                                        PtrArgType, SizeType);
    AllocFunction = insertFunDecl(M, "__splay_alloc_or_replace", VoidTy,
                                  PtrArgType, SizeType);
  }

  GetLowerBoundFunction =
      insertFunDecl(M, "__splay_get_lower_as_ptr", PtrArgType, WitnessType);
  GetUpperBoundFunction =
      insertFunDecl(M, "__splay_get_upper_as_ptr", PtrArgType, WitnessType);

  AttributeList NoReturnAttr = AttributeList::get(
      Ctx, AttributeList::FunctionIndex, Attribute::NoReturn);
  FailFunction = insertFunDecl(M, "__mi_fail", NoReturnAttr, VoidTy);

  ExtCheckCounterFunction =
      insertFunDecl(M, "__splay_inc_external_counter", VoidTy);

  VerboseFailFunction =
      insertFunDecl(M, "__mi_fail_with_msg", NoReturnAttr, VoidTy, PtrArgType);

  WarningFunction = insertFunDecl(M, "__mi_warning", VoidTy, PtrArgType);

  if (globalConfig.hasUseNoop()) {
    ConfigFunction =
        insertFunDecl(M, "__mi_config", VoidTy, SizeType, SizeType);
  }
}

void SplayMechanism::setupGlobals(Module &M) {
  auto &Ctx = M.getContext();

  // register a static constructor that inserts all globals into the splay tree
  auto Fun = registerCtors(
      M, std::make_pair<StringRef, int>("__splay_globals_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);

  for (auto &GV : M.globals()) {
    if (hasNoInstrument(&GV) || GV.getName().startswith("llvm.")) {
      continue;
    }
    LLVM_DEBUG(dbgs() << "Creating splay init code for GlobalVariable `" << GV
                      << "'\n");
    auto *PtrArg = insertCast(PtrArgType, &GV, Builder);

    auto *PtrType = cast<PointerType>(GV.getType());
    auto *PointeeType = PtrType->getElementType();
    if (!PointeeType->isSized()) {
      LLVM_DEBUG(dbgs() << "Found unsized global variable: " << GV << "\n";);
      ++SplayNumNonSizedGlobals;
      continue;
    }
    uint64_t sz = M.getDataLayout().getTypeAllocSize(PointeeType);
    auto *Size = ConstantInt::get(SizeType, sz);

    if (globalConfig.hasInstrumentVerbose()) {
      std::string insn = "";
      raw_string_ostream ss(insn);
      ss << GV;
      auto *Arr = insertStringLiteral(M, ss.str());
      auto *Str = insertCast(PtrArgType, Arr, Builder);
      insertCall(Builder, GlobalAllocFunction,
                 std::vector<Value *>{PtrArg, Size, Str});
    } else {
      insertCall(Builder, GlobalAllocFunction,
                 std::vector<Value *>{PtrArg, Size});
    }
    ++SplayNumGlobals;
  }

  for (auto &F : M.functions()) {
    // insert functions as having size 1
    if (hasNoInstrument(&F) || F.isIntrinsic()) {
      continue;
    }
    LLVM_DEBUG(dbgs() << "Creating splay init code for Function `" << F
                      << "'\n");
    auto *PtrArg = insertCast(PtrArgType, &F, Builder);

    auto *Size = ConstantInt::get(SizeType, 1);

    if (globalConfig.hasInstrumentVerbose()) {
      std::string insn = "";
      raw_string_ostream ss(insn);
      ss << "Function " << F.getName();
      auto *Arr = insertStringLiteral(M, ss.str());
      auto *Str = insertCast(PtrArgType, Arr, Builder);
      insertCall(Builder, GlobalAllocFunction,
                 std::vector<Value *>{PtrArg, Size, Str});
    } else {
      insertCall(Builder, GlobalAllocFunction,
                 std::vector<Value *>{PtrArg, Size});
    }
    ++SplayNumFunctions;
  }
  Builder.CreateRetVoid();
}

void SplayMechanism::instrumentAlloca(Module &M, AllocaInst *AI) {
  if (hasNoInstrument(AI) || hasVarArgHandling(AI)) {
    return;
  }

  IRBuilder<> Builder(AI->getNextNode());
  auto *PtrArg = insertCast(PtrArgType, AI, Builder);

  uint64_t sz = M.getDataLayout().getTypeAllocSize(AI->getAllocatedType());
  LLVM_DEBUG(dbgs() << "Registering alloca `"; AI->dump();
             dbgs() << "` with size " << sz << "\n";);
  Value *Size = ConstantInt::get(SizeType, sz);

  if (AI->isArrayAllocation()) {
    Size = Builder.CreateMul(AI->getArraySize(), Size, "", /*hasNUW*/ true,
                             /*hasNSW*/ false);
  }

  if (globalConfig.hasInstrumentVerbose()) {
    std::string insn = "";
    raw_string_ostream ss(insn);
    ss << *AI;
    auto *Arr = insertStringLiteral(M, ss.str());
    auto *Str = insertCast(PtrArgType, Arr, Builder);
    insertCall(Builder, AllocFunction, std::vector<Value *>{PtrArg, Size, Str});
  } else {
    insertCall(Builder, AllocFunction, std::vector<Value *>{PtrArg, Size});
  }
  ++SplayNumAllocas;
}

void SplayMechanism::initTypes(LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void SplayMechanism::setupInitCall(Module &M) {
  auto &Ctx = M.getContext();
  auto FunTy = FunctionType::get(Type::getVoidTy(Ctx), false);
  auto *Fun = Function::Create(FunTy, GlobalValue::WeakAnyLinkage,
                               "__mi_init_callback__", &M);
  setNoInstrument(Fun);

  auto *BB = BasicBlock::Create(Ctx, "bb", Fun, 0);
  IRBuilder<> Builder(BB);

  Constant *TimeVal = nullptr;
  Constant *IndexVal = nullptr;

#define ADD_TIME_VAL(i, x)                                                     \
  IndexVal = ConstantInt::get(SizeType, i);                                    \
  TimeVal = ConstantInt::get(SizeType, globalConfig.getNoop##x##Time());       \
  insertCall(Builder, ConfigFunction, std::vector<Value *>{IndexVal, TimeVal});

  ADD_TIME_VAL(0, DerefCheck)
  ADD_TIME_VAL(1, InvarCheck)
  ADD_TIME_VAL(2, GenBounds)
  ADD_TIME_VAL(3, StackAlloc)
  ADD_TIME_VAL(4, HeapAlloc)
  ADD_TIME_VAL(5, GlobalAlloc)
  ADD_TIME_VAL(6, HeapFree)
  Builder.CreateRetVoid();
}

void SplayMechanism::initialize(Module &M) {
  initTypes(M.getContext());

  insertFunctionDeclarations(M);

  setupGlobals(M);

  if (globalConfig.hasUseNoop()) {
    setupInitCall(M);
  }

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F))
      continue;

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I)) {
          instrumentAlloca(M, AI);
        }
      }
    }
  }
}

WitnessPtr SplayMechanism::getWitnessPhi(PHINode *Phi) const {

  IRBuilder<> builder(Phi);
  auto *NewPhi = builder.CreatePHI(WitnessType, Phi->getNumIncomingValues());

  ++SplayNumWitnessPhis;
  return std::make_shared<SplayWitness>(NewPhi, Phi);
}

void SplayMechanism::addIncomingWitnessToPhi(WitnessPtr &Phi,
                                             WitnessPtr &Incoming,
                                             BasicBlock *InBB) const {
  auto *PhiWitness = cast<SplayWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<SplayWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

WitnessPtr SplayMechanism::getWitnessSelect(SelectInst *Sel,
                                            WitnessPtr &TrueWitness,
                                            WitnessPtr &FalseWitness) const {
  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<SplayWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<SplayWitness>(FalseWitness.get())->WitnessValue;

  // auto Name = Sel->getName() + "_witness";
  auto *NewSel = builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal);

  ++SplayNumWitnessSelects;
  return std::make_shared<SplayWitness>(NewSel, Sel);
}
