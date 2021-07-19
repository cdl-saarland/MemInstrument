//===- BeforeOutflowPolicy.cpp - Check pointer on Escape ------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/pass/Util.h"

STATISTIC(NumIntrinsicsHandled, "The # of intrinsics that could be handled");
STATISTIC(NumIntrinsicsNotHandled,
          "The # of intrinsics that could not be handled");

using namespace meminstrument;
using namespace llvm;

void BeforeOutflowPolicy::classifyTargets(ITargetVector &Dest,
                                          Instruction *Location) {

  // Ignore instructions that are metadata propagation for varargs
  if (hasVarArgHandling(Location)) {
    return;
  }

  switch (Location->getOpcode()) {
  case Instruction::Ret:
    insertInvariantTargetReturn(Dest, cast<ReturnInst>(Location));
    break;
  case Instruction::Call: {
    CallInst *I = cast<CallInst>(Location);

    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (insertCheckTargetsForIntrinsic(Dest, II)) {
        ++NumIntrinsicsHandled;
        break;
      }
      ++NumIntrinsicsNotHandled;
    }

    auto *Fun = I->getCalledFunction();
    if (!Fun) { // call via function pointer
      if (!validateSize(I->getCalledOperand())) {
        return;
      }
      Dest.push_back(
          ITargetBuilder::createCallCheckTarget(I->getCalledOperand(), I));
    }
    if (Fun && Fun->hasName() && Fun->getName().startswith("llvm.dbg.")) {
      // skip debug information pseudo-calls
      break;
    }
    bool FunIsNoVarArg = Fun && !Fun->isVarArg();
    Function::arg_iterator ArgIt;
    if (FunIsNoVarArg) {
      ArgIt = Fun->arg_begin();
    }
    for (auto &Operand : I->arg_operands()) {
      if (!Operand->getType()->isPointerTy()) {
        if (FunIsNoVarArg) {
          ++ArgIt;
        }
        continue;
      }
      if (Operand->getType()->isMetadataTy()) {
        // skip metadata arguments
        if (FunIsNoVarArg) {
          ++ArgIt;
        }
        continue;
      }

      Dest.push_back(ITargetBuilder::createArgInvariantTarget(
          Operand, I, Operand.getOperandNo()));

      if (FunIsNoVarArg) {
        ++ArgIt;
      }
    }

    break;
  }
  case Instruction::InsertValue:
    insertInvariantTargetInsertVal(Dest, cast<InsertValueInst>(Location));
    break;
  case Instruction::Store:
    insertInvariantTargetStore(Dest, cast<StoreInst>(Location));
    // falls through
  case Instruction::Load:
    insertCheckTargetsLoadStore(Dest, Location);
    break;

  case Instruction::Invoke:
  case Instruction::Resume:
  case Instruction::IndirectBr:
  case Instruction::AtomicCmpXchg:
  case Instruction::AtomicRMW:
    // TODO implement
    LLVM_DEBUG(dbgs() << "skipping unimplemented instruction "
                      << Location->getName() << "\n";);
    break;

  default:
    break;
  }
}
