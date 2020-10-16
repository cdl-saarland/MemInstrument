//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
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

void DummyMechanism::insertWitness(ITarget &Target) const {
  assert(Target.isValid());
  IRBuilder<> Builder(Target.getLocation());

  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

  auto *WitnessVal =
      insertCall(Builder, CreateWitnessFunction, CastVal, "witness");
  Target.setBoundWitness(std::make_shared<DummyWitness>(WitnessVal));
}

void DummyMechanism::relocCloneWitness(Witness &W, ITarget &Target) const {
  auto *SW = dyn_cast<DummyWitness>(&W);
  assert(SW != nullptr);

  Target.setBoundWitness(
      std::shared_ptr<DummyWitness>(new DummyWitness(SW->WitnessValue)));
}

void DummyMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<DummyWitness>(Target.getBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);
  auto *Size = Target.is(ITarget::Kind::ConstSizeCheck)
                   ? ConstantInt::get(SizeType, Target.getAccessSize())
                   : Target.getAccessSizeVal();

  insertCall(Builder, CheckAccessFunction,
             std::vector<Value *>{CastVal, WitnessVal, Size}, "check");
}

void DummyMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<DummyWitness>(Target.getBoundWitness().get());
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

Value *DummyMechanism::getFailFunction(void) const { return FailFunction; }

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

std::shared_ptr<Witness>
DummyMechanism::insertWitnessPhi(ITarget &Target) const {
  assert(Target.isValid());
  auto *Phi = cast<PHINode>(Target.getInstrumentee());

  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  Target.setBoundWitness(std::make_shared<DummyWitness>(NewPhi));
  return Target.getBoundWitness();
}

void DummyMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                             std::shared_ptr<Witness> &Incoming,
                                             BasicBlock *InBB) const {
  auto *PhiWitness = cast<DummyWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<DummyWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> DummyMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  assert(Target.isValid());
  auto *Sel = cast<SelectInst>(Target.getInstrumentee());

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<DummyWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<DummyWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  Target.setBoundWitness(std::make_shared<DummyWitness>(NewSel));
  return Target.getBoundWitness();
}
