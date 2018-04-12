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

llvm::Value *DummyWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *DummyWitness::getUpperBound(void) const { return UpperBound; }

DummyWitness::DummyWitness(llvm::Value *WitnessValue)
    : Witness(WK_Dummy), WitnessValue(WitnessValue) {}

void DummyMechanism::initTypes(llvm::LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void DummyMechanism::insertWitness(ITarget &Target) const {
  IRBuilder<> Builder(Target.Location);

  auto *CastVal = insertCast(PtrArgType, Target.Instrumentee, Builder);

  auto Name = Target.Instrumentee->getName() + "_witness";
  auto *WitnessVal = insertCall(Builder, CreateWitnessFunction, Name, CastVal);
  Target.BoundWitness = std::make_shared<DummyWitness>(WitnessVal);
}

void DummyMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> Builder(Target.Location);

  auto *Witness = cast<DummyWitness>(Target.BoundWitness.get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.Instrumentee, Builder);
  auto *Size = Target.HasConstAccessSize
                   ? ConstantInt::get(SizeType, Target.AccessSize)
                   : Target.AccessSizeVal;

  insertCall(Builder, CheckAccessFunction, CastVal, WitnessVal, Size);
}

void DummyMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.RequiresExplicitBounds);

  IRBuilder<> Builder(Target.Location);

  auto *Witness = cast<DummyWitness>(Target.BoundWitness.get());
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
}

llvm::Constant *DummyMechanism::getFailFunction(void) const {
  return FailFunction;
}

bool DummyMechanism::initialize(llvm::Module &M) {
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

  llvm::AttributeList NoReturnAttr = llvm::AttributeList::get(
      Ctx, llvm::AttributeList::FunctionIndex, llvm::Attribute::NoReturn);
  FailFunction = insertFunDecl(M, "__memsafe_dummy_fail", NoReturnAttr, VoidTy);

  return true;
}

std::shared_ptr<Witness>
DummyMechanism::insertWitnessPhi(ITarget &Target) const {
  auto *Phi = cast<PHINode>(Target.Instrumentee);

  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  Target.BoundWitness = std::make_shared<DummyWitness>(NewPhi);
  return Target.BoundWitness;
}

void DummyMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                             std::shared_ptr<Witness> &Incoming,
                                             llvm::BasicBlock *InBB) const {
  auto *PhiWitness = cast<DummyWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<DummyWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> DummyMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  auto *Sel = cast<SelectInst>(Target.Instrumentee);

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<DummyWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<DummyWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  Target.BoundWitness = std::make_shared<DummyWitness>(NewSel);
  return std::make_shared<DummyWitness>(NewSel);
}
