//===- ExampleMechanism.cpp - Add Wide Bound Checks -----------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/ExampleMechanism.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/pass/Util.h"

STATISTIC(ExampleChecksPlaced, "The number of full bound checks placed");

using namespace llvm;
using namespace meminstrument;

Value *ExampleWitness::getLowerBound(void) const { return nullptr; }

Value *ExampleWitness::getUpperBound(void) const { return nullptr; }

ExampleWitness::ExampleWitness(void) : Witness(WK_Example) {}

void ExampleMechanism::insertWitnesses(ITarget &Target) const {
  assert(Target.isValid());
  // There should be no targets without an instrumentee
  assert(Target.hasInstrumentee());

  auto instrumentee = Target.getInstrumentee();

  if (!instrumentee->getType()->isAggregateType()) {
    Target.setSingleBoundWitness(std::make_shared<ExampleWitness>());
    return;
  }

  if (isa<CallBase>(instrumentee) || isa<LandingPadInst>(instrumentee)) {
    // Find all locations of pointer values in the aggregate type
    auto indices = computePointerIndices(instrumentee->getType());
    for (auto index : indices) {
      Target.setBoundWitness(std::make_shared<ExampleWitness>(), index);
    }
    return;
  }

  // The only aggregates that do not need a source are those that are constant
  assert(isa<Constant>(instrumentee));
  auto con = cast<Constant>(instrumentee);

  auto indexToPtr =
      InstrumentationMechanism::getAggregatePointerIndicesAndValues(con);
  for (auto KV : indexToPtr) {
    Target.setBoundWitness(std::make_shared<ExampleWitness>(), KV.first);
  }
}

WitnessPtr ExampleMechanism::getRelocatedClone(const Witness &,
                                               Instruction *) const {
  return std::make_shared<ExampleWitness>();
}

void ExampleMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());
  IRBuilder<> Builder(Target.getLocation());

  // TODO more efficient checks might be conceivable (e.g. with just one cmp)

  auto *Ptr2Int = Builder.CreatePtrToInt(Target.getInstrumentee(), SizeType);

  auto *Lower = Builder.CreateLoad(LowerBoundLocation, /* isVolatile */ true);
  auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

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
  auto AccessUpper = Builder.CreateAdd(Ptr2Int, Size);

  auto *Upper = Builder.CreateLoad(UpperBoundLocation, /* isVolatile */ true);
  auto *CmpUpper = Builder.CreateICmpUGT(AccessUpper, Upper);

  auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

  auto Unreach = SplitBlockAndInsertIfThen(Or, Target.getLocation(), true);
  Builder.SetInsertPoint(Unreach);
  Builder.CreateStore(ConstantInt::get(SizeType, 1), CheckResultLocation,
                      /* isVolatile */ true);
  Builder.CreateCall(getFailFunction());

  ++ExampleChecksPlaced;
}

void ExampleMechanism::materializeBounds(ITarget &) {
  llvm_unreachable("Explicit bounds are not supported by this mechanism!");
}

FunctionCallee ExampleMechanism::getFailFunction(void) const {
  return failFunction;
}

void ExampleMechanism::initialize(Module &M) {

  // Register common functions
  insertCommonFunctionDeclarations(M);

  auto &Ctx = M.getContext();

  SizeType = Type::getInt64Ty(Ctx);

  size_t numbits = 64;

  APInt lowerVal = APInt::getNullValue(numbits);

  LowerBoundLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, lowerVal), "mi_lower_bound_location");

  APInt upperVal = APInt::getAllOnesValue(numbits);

  UpperBoundLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, upperVal), "mi_upper_bound_location");

  CheckResultLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, 0), "mi_check_result_location");
}

WitnessPtr ExampleMechanism::getWitnessPhi(PHINode *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
}

void ExampleMechanism::addIncomingWitnessToPhi(WitnessPtr &, WitnessPtr &,
                                               BasicBlock *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
}

WitnessPtr ExampleMechanism::getWitnessSelect(SelectInst *, WitnessPtr &,
                                              WitnessPtr &) const {
  llvm_unreachable("Selects are not supported by this mechanism!");
}

bool ExampleMechanism::invariantsAreChecks() const { return false; }
