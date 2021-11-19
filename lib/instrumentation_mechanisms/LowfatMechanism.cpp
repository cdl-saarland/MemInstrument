//===- LowfatMechanism.cpp - Low-Fat Pointer ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
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

Value *LowfatWitness::getLowerBound(void) const { return LowerBound; }

Value *LowfatWitness::getUpperBound(void) const { return UpperBound; }

LowfatWitness::LowfatWitness(Value *WitnessValue, Instruction *Location)
    : Witness(WK_Lowfat), WitnessValue(WitnessValue), Location(Location) {}

Instruction *LowfatWitness::getInsertionLocation() const {
  auto *Res = Location;
  while (isa<PHINode>(Res)) {
    Res = Res->getNextNode();
  }
  return Res;
}

bool LowfatWitness::hasBoundsMaterialized(void) const {
  return UpperBound != nullptr && LowerBound != nullptr;
}

void LowfatMechanism::insertWitnesses(ITarget &Target) const {
  // There should be no targets without an instrumentee for lowfat
  assert(Target.hasInstrumentee());

  auto instrumentee = Target.getInstrumentee();

  if (!instrumentee->getType()->isAggregateType()) {
    auto *CastVal = insertCast(WitnessType, Target.getInstrumentee(),
                               Target.getLocation(), "_witness");
    Target.setSingleBoundWitness(
        std::make_shared<LowfatWitness>(CastVal, Target.getLocation()));

    ++LowfatNumWitnessLookups;
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
          std::make_shared<LowfatWitness>(castVal, Target.getLocation()),
          index);
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
        std::make_shared<LowfatWitness>(CastVal, Target.getLocation()),
        KV.first);

    ++LowfatNumWitnessLookups;
  }
}

WitnessPtr LowfatMechanism::getRelocatedClone(const Witness &wit,
                                              Instruction *location) const {
  const auto *lowfatWit = dyn_cast<LowfatWitness>(&wit);
  assert(lowfatWit != nullptr);

  ++LowfatNumWitnessLookups;

  return std::make_shared<LowfatWitness>(lowfatWit->WitnessValue, location);
}

void LowfatMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck() || Target.isInvariant());

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<LowfatWitness>(Target.getSingleBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

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
    insertCall(Builder, CheckDerefFunction,
               std::vector<Value *>{WitnessVal, CastVal, Size});
    ++LowfatNumDereferenceChecks;
  } else {
    assert(Target.isInvariant());
    insertCall(Builder, CheckOOBFunction,
               std::vector<Value *>{WitnessVal, CastVal});
    ++LowfatNumInboundsChecks;
  }
}

void LowfatMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());
  auto witnesses = Target.getBoundWitnesses();

  for (auto kv : witnesses) {

    auto *Witness = cast<LowfatWitness>(kv.second.get());

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
}

FunctionCallee LowfatMechanism::getFailFunction(void) const {
  return failFunction;
}

FunctionCallee LowfatMechanism::getVerboseFailFunction(void) const {
  return verboseFailFunction;
}

void LowfatMechanism::insertFunctionDeclarations(Module &M) {

  // Register common functions
  insertCommonFunctionDeclarations(M);

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
}

void LowfatMechanism::initTypes(LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void LowfatMechanism::initialize(Module &M) {
  initTypes(M.getContext());
  insertFunctionDeclarations(M);
}

WitnessPtr LowfatMechanism::getWitnessPhi(PHINode *Phi) const {
  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  ++LowfatNumWitnessPhis;
  return std::make_shared<LowfatWitness>(NewPhi, Phi);
}

void LowfatMechanism::addIncomingWitnessToPhi(WitnessPtr &Phi,
                                              WitnessPtr &Incoming,
                                              BasicBlock *InBB) const {
  auto *PhiWitness = cast<LowfatWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<LowfatWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

WitnessPtr LowfatMechanism::getWitnessSelect(SelectInst *Sel,
                                             WitnessPtr &TrueWitness,
                                             WitnessPtr &FalseWitness) const {

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<LowfatWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<LowfatWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  ++LowfatNumWitnessSelects;
  return std::make_shared<LowfatWitness>(NewSel, Sel);
}

bool LowfatMechanism::invariantsAreChecks() const { return true; }
