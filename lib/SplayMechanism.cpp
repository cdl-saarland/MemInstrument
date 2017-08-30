//===--------- SplayMechanism.cpp -- MemSafety Instrumentation ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/SplayMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/ADT/Statistic.h"

#include "meminstrument/Util.h"

#define PACK_ITARGETS 1

STATISTIC(SplayNumChecks, "The # of checks inserted");
STATISTIC(SplayNumBounds, "The # of bound(pairs) materialized");
STATISTIC(SplayNumWitnessPhis, "The # of witness phis inserted");
STATISTIC(SplayNumWitnessSelects, "The # of witness selects inserted");
STATISTIC(SplayNumWitnessLookups, "The # of witness lookups inserted");
STATISTIC(SplayNumGlobals, "The # of globals registered");
STATISTIC(SplayNumAllocas, "The # of allocas registered");

using namespace llvm;
using namespace meminstrument;

// FIXME currently, all out-of-bounds pointers are marked invalid here,
// including legal one-after-allocation ones.

llvm::Value *SplayWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *SplayWitness::getUpperBound(void) const { return UpperBound; }

SplayWitness::SplayWitness(llvm::Value *WitnessValue)
    : Witness(WK_Splay), WitnessValue(WitnessValue) {}

llvm::Type *SplayWitness::getWitnessType(LLVMContext &Ctx) {
  return Type::getInt8PtrTy(Ctx);
}

void SplayMechanism::insertWitness(ITarget &Target) const {
  IRBuilder<> builder(Target.Location);

  auto *VoidPtrTy = Type::getInt8PtrTy(Target.Instrumentee->getContext());

  auto *CastVal =
      builder.CreateBitCast(Target.Instrumentee, VoidPtrTy,
                            Target.Instrumentee->getName() + "_witness");

  Target.BoundWitness = std::make_shared<SplayWitness>(CastVal);
  ++SplayNumWitnessLookups;
}

void SplayMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());

  auto &Ctx = Target.Instrumentee->getContext();
  auto *VoidPtrTy = Type::getInt8PtrTy(Ctx);

  auto *CastVal =
      builder.CreateBitCast(Target.Instrumentee, VoidPtrTy,
                            Target.Instrumentee->getName() + "_casted");

  std::vector<Value *> Args;
  Args.push_back(Witness->WitnessValue);
  Args.push_back(CastVal);
  auto *I64Type = Type::getInt64Ty(Target.Location->getContext());
  Args.push_back(ConstantInt::get(I64Type, Target.AccessSize));

#if PACK_ITARGETS
  Module *M = Target.Location->getModule();
  auto *Val = insertStringLiteral(*M, (Target.Location->getFunction()->getName() + "::" + Target.Instrumentee->getName()).str());
  auto *CastedVal = builder.CreateBitCast(Val, Type::getInt8PtrTy(Ctx));
  Args.push_back(CastedVal);
  builder.CreateCall(CheckAccessFunction, Args);
#else
  builder.CreateCall(CheckAccessFunction, Args);
#endif

  ++SplayNumChecks;
}

void SplayMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.RequiresExplicitBounds);

  IRBuilder<> builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());

  std::vector<Value *> Args;
  Args.push_back(Witness->WitnessValue);

  if (Target.CheckUpperBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_upper";
    auto *UpperVal = builder.CreateCall(GetUpperBoundFunction, Args, Name);
    Witness->UpperBound = UpperVal;
  }
  if (Target.CheckLowerBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_lower";
    auto *LowerVal = builder.CreateCall(GetUpperBoundFunction, Args, Name);
    Witness->LowerBound = LowerVal;
  }
  ++SplayNumBounds;
}

void SplayMechanism::insertFunctionDeclarations(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *WitnessType = Type::getInt8PtrTy(Ctx);
  auto *SizeType = Type::getInt64Ty(Ctx);

  std::vector<Type *> Args;
  FunctionType *FunTy = nullptr;

  Args.push_back(WitnessType);
  Args.push_back(InstrumenteeType);
  Args.push_back(SizeType);

#if PACK_ITARGETS
  Args.push_back(Type::getInt8PtrTy(Ctx));

  FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  CheckAccessFunction = M.getOrInsertFunction("__splay_check_access_named", FunTy);
#else
  FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  CheckAccessFunction = M.getOrInsertFunction("__splay_check_access", FunTy);
#endif

  Args.clear();

  Args.push_back(WitnessType);

  FunTy = FunctionType::get(InstrumenteeType, Args, false);
  GetLowerBoundFunction = M.getOrInsertFunction("__splay_get_lower", FunTy);

  GetUpperBoundFunction = M.getOrInsertFunction("__splay_get_upper", FunTy);

  Args.clear();
  Args.push_back(InstrumenteeType);
  Args.push_back(SizeType);

  FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  GlobalAllocFunction = M.getOrInsertFunction("__splay_alloc_global", FunTy);
  AllocFunction = M.getOrInsertFunction("__splay_alloc_or_replace", FunTy);
}

void SplayMechanism::setupGlobals(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *SizeType = Type::getInt64Ty(Ctx);

  // register a static constructor that inserts all globals into the splay tree
  auto Fun = registerCtors(
      M, std::make_pair<StringRef, int>("__splay_globals_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);
  std::vector<Value *> ArgVals;

  for (auto &GV : M.globals()) {
    if (GV.isDeclaration() ||
        hasNoInstrument(&GV)) { // only insert globals that are defined here
      continue;                 // TODO right?
    }
    DEBUG(dbgs() << "Creating splay init code for GlobalVariable `" << GV
                 << "`\n");
    auto *PtrArg =
        Builder.CreateBitCast(&GV, InstrumenteeType, GV.getName() + "_casted");
    ArgVals.push_back(PtrArg); // Pointer

    auto *PtrType = cast<PointerType>(GV.getType());
    auto *PointeeType = PtrType->getElementType();
    uint64_t sz = M.getDataLayout().getTypeAllocSize(PointeeType);
    ArgVals.push_back(
        Constant::getIntegerValue(SizeType, APInt(64, sz))); // Size

    Builder.CreateCall(GlobalAllocFunction, ArgVals);
    ++SplayNumGlobals;
    ArgVals.clear();
  }
  Builder.CreateRetVoid();
}

void SplayMechanism::instrumentAlloca(Module &M, llvm::AllocaInst *AI) {
  auto &Ctx = AI->getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *SizeType = Type::getInt64Ty(Ctx);
  IRBuilder<> Builder(AI->getNextNode());
  std::vector<Value *> ArgVals;

  auto *PtrArg =
      Builder.CreateBitCast(AI, InstrumenteeType, AI->getName() + "_casted");
  ArgVals.push_back(PtrArg);

  uint64_t sz = M.getDataLayout().getTypeAllocSize(AI->getAllocatedType());

  Value *Size = Constant::getIntegerValue(SizeType, APInt(64, sz));
  if (AI->isArrayAllocation()) {
    Size = Builder.CreateMul(AI->getArraySize(), Size,
                             AI->getName() + "_bytes_size", /*hasNUW*/ true,
                             /*hasNSW*/ false);
  }
  ArgVals.push_back(Size);

  auto *Call = Builder.CreateCall(AllocFunction, ArgVals);
  setNoInstrument(Call);
  ++SplayNumAllocas;
}

bool SplayMechanism::initialize(llvm::Module &M) {
  insertFunctionDeclarations(M);

  setupGlobals(M);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;
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
  auto &Ctx = Phi->getContext();

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi = builder.CreatePHI(SplayWitness::getWitnessType(Ctx),
                                   Phi->getNumIncomingValues(), Name);

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
