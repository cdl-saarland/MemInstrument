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
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace llvm;
using namespace meminstrument;

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
}

void SplayMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());

  auto *VoidPtrTy = Type::getInt8PtrTy(Target.Instrumentee->getContext());

  auto *CastVal =
      builder.CreateBitCast(Target.Instrumentee, VoidPtrTy,
                            Target.Instrumentee->getName() + "_casted");

  std::vector<Value *> Args;
  Args.push_back(Witness->WitnessValue);
  Args.push_back(CastVal);
  auto *I64Type = Type::getInt64Ty(Target.Location->getContext());
  Args.push_back(ConstantInt::get(I64Type, Target.AccessSize));

  builder.CreateCall(CheckAccessFunction, Args);
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
}

bool SplayMechanism::insertFunctionDefinitions(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *WitnessType = Type::getInt8PtrTy(Ctx);

  std::vector<Type *> Args;

  Args.push_back(WitnessType);
  Args.push_back(InstrumenteeType);
  Args.push_back(Type::getInt64Ty(Ctx));

  auto *FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  CheckAccessFunction =
      M.getOrInsertFunction("__splay_check_access", FunTy);

  Args.clear();

  Args.push_back(WitnessType);

  FunTy = FunctionType::get(InstrumenteeType, Args, false);
  GetLowerBoundFunction = M.getOrInsertFunction(
      "__splay_get_lower", FunTy);

  GetUpperBoundFunction = M.getOrInsertFunction(
      "__splay_get_upper", FunTy);

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
  return std::make_shared<SplayWitness>(NewSel);
}

