//===- DummyMechanism.cpp - Dummy Mechanism -------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/DummyMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/pass/Util.h"

using namespace llvm;
using namespace meminstrument;

Value *DummyWitness::getLowerBound(void) const { return LowerBound; }

Value *DummyWitness::getUpperBound(void) const { return UpperBound; }

DummyWitness::DummyWitness(Value *WitnessValue)
    : Witness(WK_Dummy), WitnessValue(WitnessValue) {}

void DummyMechanism::initTypes(LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void DummyMechanism::insertWitnesses(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.hasInstrumentee());

  IRBuilder<> Builder(Target.getLocation());
  auto instrumentee = Target.getInstrumentee();

  if (!instrumentee->getType()->isAggregateType()) {
    auto *CastVal = insertCast(PtrArgType, instrumentee, Builder);
    auto *WitnessVal =
        insertCall(Builder, CreateWitnessFunction, CastVal, "witness");
    Target.setSingleBoundWitness(std::make_shared<DummyWitness>(WitnessVal));
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
      auto *CastVal = insertCast(WitnessType, ptr, builder);
      auto *WitnessVal =
          insertCall(Builder, CreateWitnessFunction, CastVal, "witness");
      Target.setBoundWitness(std::make_shared<DummyWitness>(WitnessVal), index);
    }
    return;
  }

  // The only aggregates that do not need a source are those that are constant
  assert(isa<Constant>(instrumentee));
  auto con = cast<Constant>(instrumentee);

  auto indexToPtr =
      InstrumentationMechanism::getAggregatePointerIndicesAndValues(con);
  for (auto KV : indexToPtr) {

    auto *CastVal = insertCast(PtrArgType, KV.second, Builder);

    auto *WitnessVal =
        insertCall(Builder, CreateWitnessFunction, CastVal, "witness");
    Target.setBoundWitness(std::make_shared<DummyWitness>(WitnessVal),
                           KV.first);
  }
}

WitnessPtr DummyMechanism::getRelocatedClone(const Witness &wit,
                                             Instruction *) const {
  auto dwit = dyn_cast<DummyWitness>(&wit);
  assert(dwit != nullptr);
  return std::make_shared<DummyWitness>(dwit->WitnessValue);
}

void DummyMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<DummyWitness>(Target.getSingleBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

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

  insertCall(Builder, CheckAccessFunction,
             std::vector<Value *>{CastVal, WitnessVal, Size}, "check");
}

void DummyMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());

  IRBuilder<> Builder(Target.getLocation());
  auto witnesses = Target.getBoundWitnesses();

  for (auto kv : witnesses) {

    auto *Witness = cast<DummyWitness>(kv.second.get());
    auto *WitnessVal = Witness->WitnessValue;

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
  }
}

FunctionCallee DummyMechanism::getFailFunction(void) const { return FailFunction; }

void DummyMechanism::initialize(Module &M) {
  auto &Ctx = M.getContext();
  initTypes(Ctx);
  auto *VoidTy = Type::getVoidTy(Ctx);
  CreateWitnessFunction = insertFunDecl(M, "__memsafe_dummy_create_witness",
                                        WitnessType, PtrArgType);
  CheckAccessFunction = insertFunDecl(M, "__memsafe_dummy_check_access", VoidTy,
                                      PtrArgType, WitnessType, SizeType);
  GetLowerBoundFunction = insertFunDecl(M, "__memsafe_dummy_get_lower_bound",
                                        SizeType, WitnessType);
  GetUpperBoundFunction = insertFunDecl(M, "__memsafe_dummy_get_upper_bound",
                                        SizeType, WitnessType);

  AttributeList NoReturnAttr = AttributeList::get(
      Ctx, AttributeList::FunctionIndex, Attribute::NoReturn);
  FailFunction = insertFunDecl(M, "__memsafe_dummy_fail", NoReturnAttr, VoidTy);
}

WitnessPtr DummyMechanism::getWitnessPhi(PHINode *Phi) const {
  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  return std::make_shared<DummyWitness>(NewPhi);
}

void DummyMechanism::addIncomingWitnessToPhi(WitnessPtr &Phi,
                                             WitnessPtr &Incoming,
                                             BasicBlock *InBB) const {
  auto *PhiWitness = cast<DummyWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<DummyWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

WitnessPtr DummyMechanism::getWitnessSelect(SelectInst *Sel,
                                            WitnessPtr &TrueWitness,
                                            WitnessPtr &FalseWitness) const {

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<DummyWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<DummyWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  return std::make_shared<DummyWitness>(NewSel);
}
