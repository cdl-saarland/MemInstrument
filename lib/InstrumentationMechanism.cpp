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

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace llvm;
using namespace meminstrument;

void DummyMechanism::insertWitness(ITarget &Target) {
  IRBuilder<> builder(Target.Location);

  auto *WitnessVal = builder.CreateCall(CreateWitnessFunction,
    ArrayRef<Value*>(Target.Instrumentee),
    Target.Instrumentee->getName() + "_witness");
  Target.BoundWitness = std::make_shared<DummyWitness>(WitnessVal);
}

void DummyMechanism::insertCheck(ITarget &Target) {
  IRBuilder<> builder(Target.Location);

  auto* Witness = dyn_cast<DummyWitness>(Target.BoundWitness.get());

  std::vector<Value*> Args;
  Args.push_back(Target.Instrumentee);
  Args.push_back(Witness->WitnessValue);
  auto* I64Type = Type::getInt64Ty(Target.Location->getContext());
  Args.push_back(ConstantInt::get(I64Type, Target.AccessSize));

  builder.CreateCall(CheckAccessFunction, Args);
}

void DummyMechanism::materializeBounds(ITarget &Target) {
  IRBuilder<> builder(Target.Location);

  assert(Target.RequiresExplicitBounds);

  auto* Witness = dyn_cast<DummyWitness>(Target.BoundWitness.get());

  std::vector<Value*> Args;
  Args.push_back(Witness->WitnessValue);

  if (Target.CheckUpperBoundFlag) {
    auto *UpperVal = builder.CreateCall(GetUpperBoundFunction, Args, Target.Instrumentee->getName() + "_upper");
    Witness->UpperBound = UpperVal;
  }
  if (Target.CheckLowerBoundFlag) {
    auto *LowerVal = builder.CreateCall(GetUpperBoundFunction, Args, Target.Instrumentee->getName() + "_lower");
    Witness->LowerBound = LowerVal;
  }
}


bool DummyMechanism::insertFunctionDefinitions(llvm::Module &M) {
  auto& Ctx = M.getContext();
  auto *InstrumenteeType = Type::getInt8PtrTy(Ctx);
  auto *WitnessType = Type::getInt8PtrTy(Ctx);

  std::vector<Type*> Args;

  Args.push_back(InstrumenteeType);
  auto *FunTy = FunctionType::get(WitnessType, Args, false);
  CreateWitnessFunction = M.getOrInsertFunction("__memsafe_dummy_create_witness", FunTy/*, TODO*/);

  Args.clear();

  Args.push_back(InstrumenteeType);
  Args.push_back(WitnessType);
  Args.push_back(Type::getInt64Ty(Ctx));

  FunTy = FunctionType::get(Type::getVoidTy(Ctx), Args, false);
  CheckAccessFunction = M.getOrInsertFunction("__memsafe_dummy_check_access", FunTy/*, TODO*/);

  Args.clear();

  Args.push_back(WitnessType);

  FunTy = FunctionType::get(InstrumenteeType, Args, false);
  GetLowerBoundFunction = M.getOrInsertFunction("__memsafe_dummy_get_lower_bound", FunTy/*, TODO*/);

  GetUpperBoundFunction = M.getOrInsertFunction("__memsafe_dummy_get_upper_bound", FunTy/*, TODO*/);

  return true;
}
