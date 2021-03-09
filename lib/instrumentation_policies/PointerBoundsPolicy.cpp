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

#include "llvm/ADT/Statistic.h"

#ifdef DEBUG_TYPE
#undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "pointerboundspolicy"

using namespace llvm;
using namespace meminstrument;

//===----------------------------------------------------------------------===//
//                   Implementation of PointerBoundsPolicy
//===----------------------------------------------------------------------===//

PointerBoundsPolicy::PointerBoundsPolicy(GlobalConfig &config)
    : InstrumentationPolicy(config) {}

auto PointerBoundsPolicy::getName() const -> const char * {
  return "PointerBoundsPolicy";
}

void PointerBoundsPolicy::classifyTargets(std::vector<ITargetPtr> &dest,
                                          Instruction *loc) {

  switch (loc->getOpcode()) {
  case Instruction::Call:
    addCallTargets(dest, cast<CallInst>(loc));
    break;
  case Instruction::Ret:
    insertInvariantTargetReturn(dest, cast<ReturnInst>(loc));
    break;
  case Instruction::Store:
    insertInvariantTargetStore(dest, cast<StoreInst>(loc));
    // falls through
  case Instruction::Load:
    insertCheckTargetsLoadStore(dest, loc);
    break;
  default:
    break;
  }
}

void PointerBoundsPolicy::addCallTargets(std::vector<ITargetPtr> &dest,
                                         CallInst *call) {

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

  dest.push_back(ITargetBuilder::createCallInvariantTarget(call));

  // In case of an intrinsic, the pointer arguments do not need additional
  // invariant checks (these won't make use of any additional information).
  if (isa<IntrinsicInst>(call)) {
    return;
  }

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
