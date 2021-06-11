//===--- PointerBoundsPolicy.cpp - Policy for pointer-based approaches ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// This policy collects targets for loads and stores, where
/// stores generate two targets whenever a pointer is stored to memory. In
/// addition, targets for function arguments of pointer type and returned
/// pointers are created. Special calls targets are collected for function
/// calls.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/PointerBoundsPolicy.h"
#include "meminstrument/Config.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "pointerboundspolicy"

using namespace llvm;
using namespace meminstrument;

STATISTIC(NumComplexAggregateTypes, "Number of complex aggregates encountered");

//===----------------------------------------------------------------------===//
//                   Implementation of PointerBoundsPolicy
//===----------------------------------------------------------------------===//

PointerBoundsPolicy::PointerBoundsPolicy(GlobalConfig &config)
    : InstrumentationPolicy(config) {}

auto PointerBoundsPolicy::getName() const -> const char * {
  return "PointerBoundsPolicy";
}

void PointerBoundsPolicy::classifyTargets(ITargetVector &dest,
                                          Instruction *loc) {

  if (hasVarArgHandling(loc)) {
    insertVarArgInvariantTargets(dest, loc);
    return;
  }

  switch (loc->getOpcode()) {
  case Instruction::Call:
    addCallTargets(dest, cast<CallInst>(loc));
    break;
  case Instruction::Ret:
    insertInvariantTargetAggregate(dest, loc);
    insertInvariantTargetReturn(dest, cast<ReturnInst>(loc));
    break;
  case Instruction::Store:
    insertInvariantTargetAggregate(dest, loc);
    insertInvariantTargetStore(dest, cast<StoreInst>(loc));
    // falls through
  case Instruction::Load:
    insertCheckTargetsLoadStore(dest, loc);
    break;
  case Instruction::InsertValue: {
    insertInvariantTargetInsertVal(dest, cast<InsertValueInst>(loc));
    break;
  }
  default:
    break;
  }
}

void PointerBoundsPolicy::addCallTargets(ITargetVector &dest,
                                         CallInst *call) const {

  if (auto *II = dyn_cast<IntrinsicInst>(call)) {
    insertCheckTargetsForIntrinsic(dest, II);
  }

  auto *calledFun = call->getCalledFunction();
  if (!calledFun) {
    // If we cannot identify the called function create a target to check the
    // validity of the pointer value called
    dest.push_back(
        ITargetBuilder::createCallCheckTarget(call->getCalledOperand(), call));
  }

  if (calledFun) {
    // Don't instrument calls of LLVM debug info functions
    if (calledFun->hasName() && calledFun->getName().startswith("llvm.dbg.")) {
      return;
    }
  }

  // In case this intrinsic in some was deals with pointer information we need
  // to update, create an invariant target
  if (auto intrInst = dyn_cast<IntrinsicInst>(call)) {
    if (isRelevantIntrinsic(intrInst)) {
      dest.push_back(ITargetBuilder::createCallInvariantTarget(call));
    }
    // The pointer arguments do not need additional invariant checks (intrinsics
    // won't make use of any additional information).
    return;
  }

  dest.push_back(ITargetBuilder::createCallInvariantTarget(call));

  // Take care of pointer arguments that are handed over
  for (const auto &arg : call->args()) {

    // Skip non-pointer arguments
    if (!arg->getType()->isPointerTy()) {
      continue;
    }

    // Skip metadata arguments
    if (arg->getType()->isMetadataTy()) {
      continue;
    }

    dest.push_back(ITargetBuilder::createArgInvariantTarget(
        arg, call, arg.getOperandNo()));
  }
}

void PointerBoundsPolicy::insertVarArgInvariantTargets(ITargetVector &vec,
                                                       Instruction *inst) {

  if (auto cb = dyn_cast<CallBase>(inst)) {

    // Nothing apart from vararg intrinsics should end up here
    assert(isa<IntrinsicInst>(cb));

    auto intrInst = cast<IntrinsicInst>(cb);
    switch (intrInst->getIntrinsicID()) {
    case Intrinsic::vastart:
    case Intrinsic::vaend: {
      std::map<unsigned, llvm::Value *> requiredArgs;
      requiredArgs[0] = cb->getArgOperand(0);
      vec.push_back(
          ITargetBuilder::createCallInvariantTarget(cb, requiredArgs));
      return;
    }
    case Intrinsic::vacopy: {
      std::map<unsigned, llvm::Value *> requiredArgs;
      requiredArgs[0] = cb->getArgOperand(0);
      requiredArgs[1] = cb->getArgOperand(1);
      vec.push_back(
          ITargetBuilder::createCallInvariantTarget(cb, requiredArgs));
      return;
    }
    default:
      llvm_unreachable("Unexpected intrinsic instruction encountered.");
    }
  }

  if (auto load = dyn_cast<LoadInst>(inst)) {
    // Insert a target that makes sure that metadata is propagated up to this
    // load, such that it has bounds for the next vararg available
    if (hasVarArgLoadArg(load) && load->getType()->isPointerTy()) {
      vec.push_back(ITargetBuilder::createValInvariantTarget(
          load->getPointerOperand(), load));
    }
  }
}

void PointerBoundsPolicy::insertInvariantTargetAggregate(ITargetVector &vec,
                                                         Instruction *inst) {
  assert(isa<ReturnInst>(inst) || isa<StoreInst>(inst));
  // Return and store (note error upon store)
  Value *agg = nullptr;
  if (auto *ret = dyn_cast<ReturnInst>(inst)) {
    agg = ret->getReturnValue();
    // Nothing is returned, so no invariant is required
    if (!agg) {
      return;
    }
  }

  if (auto *store = dyn_cast<StoreInst>(inst)) {
    agg = store->getValueOperand();
  }
  assert(agg);

  // Nothing to do if this isn't an aggregate
  if (!agg->getType()->isAggregateType()) {
    return;
  }

  // No handling for nested aggregates implemented yet
  if (isNested(agg->getType())) {
    globalConfig.noteError();
    return;
  }

  // An invariant target is only required if the aggregate contains at least
  // one pointer
  if (!containsPointer(agg->getType())) {
    return;
  }
  vec.push_back(ITargetBuilder::createValInvariantTarget(agg, inst));
}

bool PointerBoundsPolicy::isRelevantIntrinsic(
    const IntrinsicInst *intrInst) const {

  switch (intrInst->getIntrinsicID()) {
  case Intrinsic::memcpy:
  case Intrinsic::memmove:
  case Intrinsic::memset:
    return true;
  default:
    break;
  }

  return false;
}

bool PointerBoundsPolicy::isNested(const Type *Aggregate) const {
  assert(Aggregate->isAggregateType());

  if (const auto *arTy = dyn_cast<ArrayType>(Aggregate)) {
    if (arTy->getElementType()->isPointerTy()) {
      ++NumComplexAggregateTypes;
      return true;
    }
    return false;
  }

  if (const auto *strTy = dyn_cast<StructType>(Aggregate)) {
    if (std::any_of(strTy->element_begin(), strTy->element_end(),
                    [](const Type *t) { return t->isAggregateType(); })) {
      ++NumComplexAggregateTypes;
      return true;
    }
    return false;
  }

  llvm_unreachable(
      "Aggregate type that is neither array nor struct encountered");
}

bool PointerBoundsPolicy::containsPointer(const Type *Aggregate) const {
  assert(Aggregate->isAggregateType());
  // It seems that nested aggregates are converted to function arguments early
  // on and do not show up as return types. Bail on complex aggregates for
  // now.
  assert(!isNested(Aggregate));

  // Check if the aggregate contains pointer values
  bool containsPtr = false;
  if (const auto *strTy = dyn_cast<StructType>(Aggregate)) {
    containsPtr = std::any_of(strTy->element_begin(), strTy->element_end(),
                              [](Type *t) { return t->isPointerTy(); });
  }
  if (const auto *arTy = dyn_cast<ArrayType>(Aggregate)) {
    containsPtr = arTy->getElementType()->isPointerTy();
  }

  return containsPtr;
}
