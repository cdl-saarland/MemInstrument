//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/NoopMechanism.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/Config.h"

#include "meminstrument/pass/Util.h"

STATISTIC(NoopMechanismAnnotated, "The number of full bound checks placed");

using namespace llvm;
using namespace meminstrument;

llvm::Value *NoopWitness::getLowerBound(void) const { return nullptr; }

llvm::Value *NoopWitness::getUpperBound(void) const { return nullptr; }

NoopWitness::NoopWitness(void) : Witness(WK_Noop) {}

void NoopMechanism::insertWitness(ITarget &Target) const {
  assert(Target.isValid());
  Target.setBoundWitness(std::make_shared<NoopWitness>());
}

void NoopMechanism::relocCloneWitness(Witness &, ITarget &Target) const {
  Target.setBoundWitness(std::shared_ptr<NoopWitness>(new NoopWitness()));
}

void NoopMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());
  IRBuilder<> Builder(Target.getLocation());

  // TODO more efficient checks might be conceivable (e.g. with just one cmp)

  auto *Ptr2Int = Builder.CreatePtrToInt(Target.getInstrumentee(), SizeType);

  auto *Lower = Builder.CreateLoad(LowerBoundLocation, /* isVolatile */ true);
  auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

  Value *AccessUpper = nullptr;
  if (Target.is(ITarget::Kind::ConstSizeCheck)) {
    AccessUpper = Builder.CreateAdd(
        Ptr2Int, ConstantInt::get(SizeType, Target.getAccessSize()));
  } else {
    AccessUpper = Builder.CreateAdd(Ptr2Int, Target.getAccessSizeVal());
  }

  auto *Upper = Builder.CreateLoad(UpperBoundLocation, /* isVolatile */ true);
  auto *CmpUpper = Builder.CreateICmpUGT(AccessUpper, Upper);

  auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

  auto Unreach = SplitBlockAndInsertIfThen(Or, Target.getLocation(), true);
  Builder.SetInsertPoint(Unreach);
  Builder.CreateStore(ConstantInt::get(SizeType, 1), CheckResultLocation,
                      /* isVolatile */ true);
  Builder.CreateCall(getFailFunction());

  ++NoopMechanismAnnotated;
}

void NoopMechanism::materializeBounds(ITarget &Target) {
  llvm_unreachable("Explicit bounds are not supported by this mechanism!");
}

llvm::Value *NoopMechanism::getFailFunction(void) const {
  return FailFunction;
}

bool NoopMechanism::initialize(llvm::Module &M) {
  auto &Ctx = M.getContext();

  SizeType = Type::getInt64Ty(Ctx);

  size_t numbits = 64;

  llvm::APInt lowerVal = APInt::getNullValue(numbits);

  LowerBoundLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, lowerVal), "mi_lower_bound_location");

  llvm::APInt upperVal = APInt::getAllOnesValue(numbits);

  UpperBoundLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, upperVal), "mi_upper_bound_location");

  CheckResultLocation = new GlobalVariable(
      M, SizeType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantInt::get(SizeType, 0), "mi_check_result_location");

  llvm::AttributeList NoReturnAttr = llvm::AttributeList::get(
      Ctx, llvm::AttributeList::FunctionIndex, llvm::Attribute::NoReturn);
  FailFunction =
      M.getOrInsertFunction("abort", NoReturnAttr, Type::getVoidTy(Ctx)).getCallee();

  return true;
}

std::shared_ptr<Witness> NoopMechanism::insertWitnessPhi(ITarget &) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
  return std::shared_ptr<Witness>(nullptr);
}

void NoopMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &,
                                            std::shared_ptr<Witness> &,
                                            llvm::BasicBlock *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
}

std::shared_ptr<Witness>
NoopMechanism::insertWitnessSelect(ITarget &, std::shared_ptr<Witness> &,
                                   std::shared_ptr<Witness> &) const {
  llvm_unreachable("Selects are not supported by this mechanism!");
  return std::shared_ptr<Witness>(nullptr);
}
