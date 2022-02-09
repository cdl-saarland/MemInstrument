//===- PointerBoundsPolicy.cpp - Policy for pointer-based approaches ------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This policy collects targets for loads and stores, where stores generate two
/// targets whenever a pointer is stored to memory. In addition, targets for
/// function arguments of pointer type and returned pointers are created.
/// Special calls targets are collected for function calls.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/PointerBoundsPolicy.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

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
  case Instruction::Invoke:
    addCallTargets(dest, cast<CallBase>(loc));
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

void PointerBoundsPolicy::addCallTargets(ITargetVector &dest, CallBase *call) {

  if (auto *II = dyn_cast<IntrinsicInst>(call)) {
    insertCheckTargetsForIntrinsic(dest, II);
  }

  auto *calledFun = call->getCalledFunction();
  if (!calledFun) {
    insertCheckTargetsForCall(dest, call);
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

  std::map<unsigned, Value *> requiredArgs;
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

    requiredArgs[arg.getOperandNo()] = arg;
  }
  dest.push_back(ITargetBuilder::createCallInvariantTarget(call, requiredArgs));
}

void PointerBoundsPolicy::insertVarArgInvariantTargets(ITargetVector &vec,
                                                       Instruction *inst) {

  if (auto cb = dyn_cast<CallBase>(inst)) {

    // The call simply returns a va_list, make sure to add the regular targets
    if (!isa<IntrinsicInst>(cb)) {
      addCallTargets(vec, cb);
      return;
    }

    auto intrInst = cast<IntrinsicInst>(cb);
    switch (intrInst->getIntrinsicID()) {
    case Intrinsic::vastart:
    case Intrinsic::vaend: {
      std::map<unsigned, Value *> requiredArgs;
      requiredArgs[0] = cb->getArgOperand(0);
      vec.push_back(
          ITargetBuilder::createCallInvariantTarget(cb, requiredArgs));
      return;
    }
    case Intrinsic::vacopy: {
      std::map<unsigned, Value *> requiredArgs;
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

  if (auto store = dyn_cast<StoreInst>(inst)) {
    // Insert a target that makes sure that metadata for the stored va_list is
    // propagated
    if (isVarArgMetadataType(store->getValueOperand()->getType())) {
      vec.push_back(ITargetBuilder::createValInvariantTarget(
          store->getValueOperand(), store));
    }
  }

  if (ReturnInst *ret = dyn_cast<ReturnInst>(inst)) {
    // Insert a target that makes sure that returned va_list pointer is stored
    // to the shadow stack
    assert(isVarArgMetadataType(ret->getReturnValue()->getType()));
    insertInvariantTargetReturn(vec, ret);
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
    MemInstrumentError::report("Found nested aggregate: ", agg);
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
