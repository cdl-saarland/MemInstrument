//===---------------------------------------------------------------------===///
///
/// \file TODO doku
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
STATISTIC(SplayNumFunctions, "The # of functions registered");
STATISTIC(SplayNumAllocas, "The # of allocas registered");
STATISTIC(SplayNumByValArgs, "The # of byval arguments registered");

using namespace llvm;
using namespace meminstrument;

// FIXME currently, all out-of-bounds pointers are marked invalid here,
// including legal one-after-allocation ones.

llvm::Value *SplayWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *SplayWitness::getUpperBound(void) const { return UpperBound; }

SplayWitness::SplayWitness(llvm::Value *WitnessValue)
    : Witness(WK_Splay), WitnessValue(WitnessValue) {}

void SplayMechanism::insertWitness(ITarget &Target) const {
  auto *CastVal =
      insertCast(WitnessType, Target.Instrumentee, Target.Location, "_witness");
  Target.BoundWitness = std::make_shared<SplayWitness>(CastVal);
  ++SplayNumWitnessLookups;
}

void SplayMechanism::insertCheck(ITarget &Target) const {

  Module *M = Target.Location->getModule();
  bool Verbose = GlobalConfig::get(*M).hasInstrumentVerbose();

  IRBuilder<> Builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.Instrumentee, Builder);

  Value *NameVal = nullptr;
  if (Verbose) {
    std::string Name;
    if (DILocation *Loc = Target.Location->getDebugLoc()) {
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      Name = (File + ": " + std::to_string(Line)).str();
    } else {
      Name = "unknown location";
    }

    // auto Name = Target.Location->getFunction()->getName() +
    //             "::" + Target.Instrumentee->getName();
    auto *Str = insertStringLiteral(*M, Name);
    NameVal = insertCast(PtrArgType, Str, Builder);
  }

  if (Target.CheckUpperBoundFlag || Target.CheckLowerBoundFlag) {
    auto *Size = Target.HasConstAccessSize
                     ? ConstantInt::get(SizeType, Target.AccessSize)
                     : Target.AccessSizeVal;
    if (Verbose) {
      insertCall(Builder, CheckDereferenceFunction, WitnessVal, CastVal, Size,
                 NameVal);
    } else {
      insertCall(Builder, CheckDereferenceFunction, WitnessVal, CastVal, Size);
    }
    ++SplayNumDereferenceChecks;
  } else {
    if (Verbose) {
      insertCall(Builder, CheckInboundsFunction, WitnessVal, CastVal, NameVal);
    } else {
      insertCall(Builder, CheckInboundsFunction, WitnessVal, CastVal);
    }
    ++SplayNumInboundsChecks;
  }
}

void SplayMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.RequiresExplicitBounds);
  assert(Target.isValid());

  IRBuilder<> Builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());
  auto *WitnessVal = Witness->WitnessValue;

  if (Target.CheckUpperBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_upper";
    auto *UpperVal =
        insertCall(Builder, GetUpperBoundFunction, Name, WitnessVal);
    Witness->UpperBound = UpperVal;
  }
  if (Target.CheckLowerBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_lower";
    auto *LowerVal =
        insertCall(Builder, GetLowerBoundFunction, Name, WitnessVal);
    Witness->LowerBound = LowerVal;
  }
  ++SplayNumBounds;
}

llvm::Constant *SplayMechanism::getFailFunction(void) const {
  return FailFunction;
}

void SplayMechanism::insertFunctionDeclarations(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *StringTy = Type::getInt8PtrTy(Ctx);

  bool Verbose = GlobalConfig::get(M).hasInstrumentVerbose();

  if (Verbose) {
    CheckInboundsFunction =
        insertFunDecl(M, "__splay_check_inbounds_named", VoidTy, WitnessType,
                      PtrArgType, StringTy);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference_named", VoidTy, WitnessType,
                      PtrArgType, SizeType, StringTy);
  } else {
    CheckInboundsFunction = insertFunDecl(M, "__splay_check_inbounds", VoidTy,
                                          WitnessType, PtrArgType);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference", VoidTy, WitnessType,
                      PtrArgType, SizeType);
  }

  GetLowerBoundFunction =
      insertFunDecl(M, "__splay_get_lower", SizeType, WitnessType);
  GetUpperBoundFunction =
      insertFunDecl(M, "__splay_get_upper", SizeType, WitnessType);

  GlobalAllocFunction =
      insertFunDecl(M, "__splay_alloc_or_merge", VoidTy, PtrArgType, SizeType);
  AllocFunction = insertFunDecl(M, "__splay_alloc_or_replace", VoidTy,
                                PtrArgType, SizeType);
  FailFunction = insertFunDecl(M, "__splay_fail_simple", VoidTy);
}

void SplayMechanism::setupGlobals(llvm::Module &M) {
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
    DEBUG(dbgs() << "Creating splay init code for GlobalVariable `" << GV
                 << "'\n");
    auto *PtrArg = insertCast(PtrArgType, &GV, Builder);

    auto *PtrType = cast<PointerType>(GV.getType());
    auto *PointeeType = PtrType->getElementType();
    uint64_t sz = M.getDataLayout().getTypeAllocSize(PointeeType);
    auto *Size = ConstantInt::get(SizeType, sz);

    insertCall(Builder, GlobalAllocFunction, PtrArg, Size);
    ++SplayNumGlobals;
  }

  for (auto &F : M.functions()) {
    // insert functions as having size 1
    if (hasNoInstrument(&F) || F.isIntrinsic()) {
      continue;
    }
    DEBUG(dbgs() << "Creating splay init code for Function `" << F << "'\n");
    auto *PtrArg = insertCast(PtrArgType, &F, Builder);

    auto *Size = ConstantInt::get(SizeType, 1);

    insertCall(Builder, GlobalAllocFunction, PtrArg, Size);
    ++SplayNumFunctions;
  }
  Builder.CreateRetVoid();
}

void SplayMechanism::instrumentAlloca(Module &M, llvm::AllocaInst *AI) {
  IRBuilder<> Builder(AI->getNextNode());
  auto *PtrArg = insertCast(PtrArgType, AI, Builder);

  uint64_t sz = M.getDataLayout().getTypeAllocSize(AI->getAllocatedType());
  Value *Size = ConstantInt::get(SizeType, sz);

  if (AI->isArrayAllocation()) {
    Size = Builder.CreateMul(AI->getArraySize(), Size,
                             AI->getName() + "_byte_size", /*hasNUW*/ true,
                             /*hasNSW*/ false);
  }

  insertCall(Builder, AllocFunction, PtrArg, Size);
  ++SplayNumAllocas;
}

void SplayMechanism::initTypes(llvm::LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

bool SplayMechanism::initialize(llvm::Module &M) {
  initTypes(M.getContext());

  insertFunctionDeclarations(M);

  setupGlobals(M);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;
    IRBuilder<> Builder(&F.front().front());
    for (auto &Arg : F.args()) {
      if (Arg.hasByValAttr()) {
        // byval parameters are implicitly copied
        DEBUG(dbgs() << "Creating splay init code for byval Argument`" << Arg
                     << "'\n");
        auto *PtrArg = insertCast(PtrArgType, &Arg, Builder);

        auto *PtrType = cast<PointerType>(Arg.getType());
        auto *PointeeType = PtrType->getElementType();
        uint64_t sz = M.getDataLayout().getTypeAllocSize(PointeeType);
        auto *Size = ConstantInt::get(SizeType, sz);

        insertCall(Builder, AllocFunction, PtrArg, Size);
        ++SplayNumByValArgs;
      }
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I)) {
          instrumentAlloca(M, AI);
        }
      }
    }
  }

  return true;
}

std::shared_ptr<Witness>
SplayMechanism::insertWitnessPhi(ITarget &Target) const {
  auto *Phi = cast<PHINode>(Target.Instrumentee);

  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  Target.BoundWitness = std::make_shared<SplayWitness>(NewPhi);
  ++SplayNumWitnessPhis;
  return Target.BoundWitness;
}

void SplayMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                             std::shared_ptr<Witness> &Incoming,
                                             llvm::BasicBlock *InBB) const {
  auto *PhiWitness = cast<SplayWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<SplayWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> SplayMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  auto *Sel = cast<SelectInst>(Target.Instrumentee);

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<SplayWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<SplayWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  Target.BoundWitness = std::make_shared<SplayWitness>(NewSel);
  ++SplayNumWitnessSelects;
  return std::make_shared<SplayWitness>(NewSel);
}
