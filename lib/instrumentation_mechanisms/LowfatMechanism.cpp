//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/LowfatMechanism.h"

#include "meminstrument/Config.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/pass/Util.h"

STATISTIC(LowfatNumInboundsChecks, "The # of inbounds checks inserted");
STATISTIC(LowfatNumDereferenceChecks, "The # of dereference checks inserted");
STATISTIC(LowfatNumBounds, "The # of bound(pairs) materialized");
STATISTIC(LowfatNumWitnessPhis, "The # of witness phis inserted");
STATISTIC(LowfatNumWitnessSelects, "The # of witness selects inserted");
STATISTIC(LowfatNumWitnessLookups, "The # of witness lookups inserted");

using namespace llvm;
using namespace meminstrument;

llvm::Value *LowfatWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *LowfatWitness::getUpperBound(void) const { return UpperBound; }

LowfatWitness::LowfatWitness(llvm::Value *WitnessValue,
                             llvm::Instruction *Location)
    : Witness(WK_Lowfat), WitnessValue(WitnessValue), Location(Location) {}

llvm::Instruction *LowfatWitness::getInsertionLocation() const {
  auto *Res = Location;
  while (isa<PHINode>(Res)) {
    Res = Res->getNextNode();
  }
  return Res;
}

bool LowfatWitness::hasBoundsMaterialized(void) const {
  return UpperBound != nullptr && LowerBound != nullptr;
}

void LowfatMechanism::insertWitness(ITarget &Target) const {
  auto *CastVal = insertCast(WitnessType, Target.getInstrumentee(),
                             Target.getLocation(), "_witness");
  Target.setBoundWitness(
      std::make_shared<LowfatWitness>(CastVal, Target.getLocation()));
  ++LowfatNumWitnessLookups;
}

void LowfatMechanism::relocCloneWitness(Witness &W, ITarget &Target) const {
  auto *SW = dyn_cast<LowfatWitness>(&W);
  assert(SW != nullptr);

  Target.setBoundWitness(std::shared_ptr<LowfatWitness>(
      new LowfatWitness(SW->WitnessValue, Target.getLocation())));
  ++LowfatNumWitnessLookups;
}

void LowfatMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck() || Target.is(ITarget::Kind::Invariant));

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<LowfatWitness>(Target.getBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

  if (Target.isCheck()) {

    // Determine the access width depending on the target kind
    Value *Size = nullptr;
    if (Target.is(ITarget::Kind::CallCheck)) {
      Size = ConstantInt::get(SizeType, 1);
    }
    if (Target.is(ITarget::Kind::ConstSizeCheck)) {
      Size = ConstantInt::get(SizeType, Target.getAccessSize());
    }
    if (Target.is(ITarget::Kind::VarSizeCheck)) {
      Size = Target.getAccessSizeVal();
    }

    assert(Size);
    insertCall(Builder, CheckDerefFunction,
               std::vector<Value *>{WitnessVal, CastVal, Size});
    ++LowfatNumDereferenceChecks;
  } else {
    assert(Target.is(ITarget::Kind::Invariant));
    insertCall(Builder, CheckOOBFunction,
               std::vector<Value *>{WitnessVal, CastVal});
    ++LowfatNumInboundsChecks;
  }
}

void LowfatMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());

  auto *Witness = cast<LowfatWitness>(Target.getBoundWitness().get());

  if (Witness->hasBoundsMaterialized()) {
    return;
  }

  auto *WitnessVal = Witness->WitnessValue;

  IRBuilder<> Builder(Witness->getInsertionLocation());

  if (Target.hasUpperBoundFlag()) {
    auto Name = Target.getInstrumentee()->getName() + "_upper";
    auto *UpperVal = insertCall(Builder, GetUpperBoundFunction,
                                std::vector<Value *>{WitnessVal}, Name);
    Witness->UpperBound = UpperVal;
  }
  if (Target.hasLowerBoundFlag()) {
    auto Name = Target.getInstrumentee()->getName() + "_lower";
    auto *LowerVal = insertCall(Builder, GetLowerBoundFunction,
                                std::vector<Value *>{WitnessVal}, Name);
    Witness->LowerBound = LowerVal;
  }
  ++LowfatNumBounds;
}

llvm::Value *LowfatMechanism::getFailFunction(void) const {
  return FailFunction;
}

llvm::Value *LowfatMechanism::getVerboseFailFunction(void) const {
  return VerboseFailFunction;
}

void LowfatMechanism::insertFunctionDeclarations(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *VoidTy = Type::getVoidTy(Ctx);

  CheckDerefFunction = insertFunDecl(M, "__lowfat_check_deref", VoidTy,
                                     WitnessType, PtrArgType, SizeType);
  CheckOOBFunction =
      insertFunDecl(M, "__lowfat_check_oob", VoidTy, WitnessType, PtrArgType);
  GetUpperBoundFunction =
      insertFunDecl(M, "__lowfat_get_upper_bound", PtrArgType, WitnessType);
  GetLowerBoundFunction =
      insertFunDecl(M, "__lowfat_get_lower_bound", PtrArgType, WitnessType);

  llvm::AttributeList NoReturnAttr = llvm::AttributeList::get(
      Ctx, llvm::AttributeList::FunctionIndex, llvm::Attribute::NoReturn);
  FailFunction = insertFunDecl(M, "__mi_fail", NoReturnAttr, VoidTy);
  VerboseFailFunction =
      insertFunDecl(M, "__mi_fail_with_msg", NoReturnAttr, VoidTy, PtrArgType);
}

void LowfatMechanism::initTypes(llvm::LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void LowfatMechanism::initialize(llvm::Module &M) {
  initTypes(M.getContext());
  insertFunctionDeclarations(M);
}

std::shared_ptr<Witness>
LowfatMechanism::insertWitnessPhi(ITarget &Target) const {
  assert(Target.isValid());
  auto *Phi = cast<PHINode>(Target.getInstrumentee());

  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  Target.setBoundWitness(std::make_shared<LowfatWitness>(NewPhi, Phi));
  ++LowfatNumWitnessPhis;
  return Target.getBoundWitness();
}

void LowfatMechanism::addIncomingWitnessToPhi(
    std::shared_ptr<Witness> &Phi, std::shared_ptr<Witness> &Incoming,
    llvm::BasicBlock *InBB) const {
  auto *PhiWitness = cast<LowfatWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<LowfatWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> LowfatMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  assert(Target.isValid());
  auto *Sel = cast<SelectInst>(Target.getInstrumentee());

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<LowfatWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<LowfatWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  Target.setBoundWitness(std::make_shared<LowfatWitness>(NewSel, Sel));
  ++LowfatNumWitnessSelects;
  return Target.getBoundWitness();
}
