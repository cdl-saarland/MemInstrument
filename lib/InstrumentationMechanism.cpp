//===---- InstrumentationMechanism.cpp -- MemSafety Instrumentation -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/InstrumentationMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace llvm;
using namespace meminstrument;

namespace {

enum InstrumentationMechanismKind {
  IM_dummy,
};

cl::opt<InstrumentationMechanismKind> InstrumentationMechanismOpt(
    "memsafety-imechanism",
    cl::desc("Choose InstructionMechanism: (default: dummy)"),
    cl::values(clEnumValN(IM_dummy, "dummy",
                          "only insert dummy calls for instrumentation")),
    cl::init(IM_dummy) // default
    );

std::unique_ptr<InstrumentationMechanism> GlobalIM(nullptr);
}

InstrumentationMechanism &InstrumentationMechanism::get(void) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationMechanismOpt) {
    case IM_dummy:
      GlobalIM.reset(new DummyMechanism());
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}

llvm::Value *DummyWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *DummyWitness::getUpperBound(void) const { return UpperBound; }

DummyWitness::DummyWitness(llvm::Value *WitnessValue)
    : Witness(WK_Dummy), WitnessValue(WitnessValue) {}

llvm::Type *DummyWitness::getWitnessType(LLVMContext &Ctx) {
  return Type::getInt8PtrTy(Ctx);
}

void DummyMechanism::insertWitness(ITarget &Target) const {
  IRBuilder<> builder(Target.Location);

  auto Name = Target.Instrumentee->getName() + "_witness";
  auto *WitnessVal = builder.CreateCall(
      CreateWitnessFunction, ArrayRef<Value *>(Target.Instrumentee), Name);
  Target.BoundWitness = std::make_shared<DummyWitness>(WitnessVal);
}

void DummyMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> builder(Target.Location);

  auto *Witness = dyn_cast<DummyWitness>(Target.BoundWitness.get());

  std::vector<Value *> Args;
  Args.push_back(Target.Instrumentee);
  Args.push_back(Witness->WitnessValue);
  auto *I64Type = Type::getInt64Ty(Target.Location->getContext());
  Args.push_back(ConstantInt::get(I64Type, Target.AccessSize));

  builder.CreateCall(CheckAccessFunction, Args);
}

void DummyMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.RequiresExplicitBounds);

  IRBuilder<> builder(Target.Location);

  auto *Witness = dyn_cast<DummyWitness>(Target.BoundWitness.get());

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

bool DummyMechanism::insertFunctionDefinitions(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *WitnessType = Type::getInt8PtrTy(Ctx);

  std::vector<Type *> Args;

  Args.push_back(InstrumenteeType);
  auto *FunTy = FunctionType::get(WitnessType, Args, false);
  CreateWitnessFunction =
      M.getOrInsertFunction("__memsafe_dummy_create_witness", FunTy /*, TODO*/);

  Args.clear();

  Args.push_back(InstrumenteeType);
  Args.push_back(WitnessType);
  Args.push_back(Type::getInt64Ty(Ctx));

  FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  CheckAccessFunction =
      M.getOrInsertFunction("__memsafe_dummy_check_access", FunTy /*, TODO*/);

  Args.clear();

  Args.push_back(WitnessType);

  FunTy = FunctionType::get(InstrumenteeType, Args, false);
  GetLowerBoundFunction = M.getOrInsertFunction(
      "__memsafe_dummy_get_lower_bound", FunTy /*, TODO*/);

  GetUpperBoundFunction = M.getOrInsertFunction(
      "__memsafe_dummy_get_upper_bound", FunTy /*, TODO*/);

  // TODO mark these functions somehow so that they are not instrumented
  // TODO mark these functions to be always inlined
  return true;
}

std::shared_ptr<Witness>
DummyMechanism::insertWitnessPhi(ITarget &Target) const {
  auto *Phi = dyn_cast<PHINode>(Target.Instrumentee);

  IRBuilder<> builder(Phi);
  auto &Ctx = Phi->getContext();

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi = builder.CreatePHI(DummyWitness::getWitnessType(Ctx),
                                   Phi->getNumIncomingValues(), Name);

  return std::make_shared<DummyWitness>(NewPhi);
}

void DummyMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                             std::shared_ptr<Witness> &Incoming,
                                             llvm::BasicBlock *InBB) const {
  auto *PhiWitness = dyn_cast<DummyWitness>(Phi.get());
  auto *PhiVal = dyn_cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = dyn_cast<DummyWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> DummyMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  auto *Sel = dyn_cast<SelectInst>(Target.Instrumentee);

  IRBuilder<> builder(Sel);

  auto *TrueVal = dyn_cast<DummyWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = dyn_cast<DummyWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  return std::make_shared<DummyWitness>(NewSel);
}
