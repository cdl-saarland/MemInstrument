//===- InstrumentationPolicy.cpp - Policy Interface -----------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/CommandLine.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

static cl::opt<bool>
    NoCallChecks("mi-policy-no-call-checks",
                 cl::desc("Don't check function pointers for validity upon "
                          "calls (reduces the safety guarantees)"),
                 cl::init(false));

static cl::opt<bool> IgnoreInlineASM(
    "mi-policy-ignore-inline-asm",
    cl::desc("Ignore inline assembler (reduces the safety guarantees)"),
    cl::init(false));

InstrumentationPolicy::InstrumentationPolicy() {}

void InstrumentationPolicy::validateSize(Value *Ptr) {
  if (!hasPointerAccessSize(Ptr)) {
    MemInstrumentError::report("Found pointer to unsized type: ", Ptr);
  }
}

bool InstrumentationPolicy::insertCheckTargetsForIntrinsic(
    ITargetVector &Dest, IntrinsicInst *II) const {
  switch (II->getIntrinsicID()) {
  case Intrinsic::memcpy:
    // TODO we could add a CallInvariant target here to ensure that src and dest
    // don't overlap. Note however that splay/lowfat cannot yet handle
    // CallInvariants.
  case Intrinsic::memmove: {
    auto *MT = cast<MemTransferInst>(II);
    auto *Len = MT->getLength();
    auto *Src = MT->getSource();
    auto *Dst = MT->getDest();
    Dest.push_back(ITargetBuilder::createSpatialCheckTarget(Src, II, Len));
    Dest.push_back(ITargetBuilder::createSpatialCheckTarget(Dst, II, Len));
    return true;
  }
  case Intrinsic::memset: {
    auto *MS = cast<MemSetInst>(II);
    auto *Len = MS->getLength();
    auto *Dst = MS->getDest();
    Dest.push_back(ITargetBuilder::createSpatialCheckTarget(Dst, II, Len));
    return true;
  }
  default:
    return false;
  }
}

void InstrumentationPolicy::insertCheckTargetsLoadStore(ITargetVector &Dest,
                                                        Instruction *Inst) {

  assert(isa<LoadInst>(Inst) || isa<StoreInst>(Inst));

  auto PtrOp = isa<LoadInst>(Inst) ? cast<LoadInst>(Inst)->getPointerOperand()
                                   : cast<StoreInst>(Inst)->getPointerOperand();

  validateSize(PtrOp);
  Dest.push_back(ITargetBuilder::createSpatialCheckTarget(PtrOp, Inst));
}

void InstrumentationPolicy::insertCheckTargetsForCall(ITargetVector &dest,
                                                      CallBase *call) {

  // Call check targets only make sense if the function cannot be identified
  // statically
  assert(!call->getCalledFunction());

  // Don't insert check if they are disabled by a CL flag
  if (NoCallChecks) {
    return;
  }

  // Decide what to do if we encounter inline ASM
  if (isa<InlineAsm>(call->getCalledOperand())) {
    if (IgnoreInlineASM) {
      return;
    }

    MemInstrumentError::report(
        "Found inline assembler (to ignore inline "
        "assembler, use `-mi-policy-ignore-inline-asm`): ",
        call);
  }

  // Create a target to check the validity of the pointer value called
  dest.push_back(
      ITargetBuilder::createCallCheckTarget(call->getCalledOperand(), call));
}

void InstrumentationPolicy::insertInvariantTargetStore(ITargetVector &Dest,
                                                       StoreInst *Store) const {

  auto *StoreOperand = Store->getValueOperand();
  if (!StoreOperand->getType()->isPointerTy()) {
    return;
  }

  Dest.push_back(ITargetBuilder::createValInvariantTarget(StoreOperand, Store));
}

void InstrumentationPolicy::insertInvariantTargetReturn(ITargetVector &Dest,
                                                        ReturnInst *Ret) const {

  auto *Operand = Ret->getReturnValue();
  if (!Operand || !Operand->getType()->isPointerTy()) {
    return;
  }

  Dest.push_back(ITargetBuilder::createValInvariantTarget(Operand, Ret));
}

void InstrumentationPolicy::insertInvariantTargetInsertVal(
    ITargetVector &dest, InsertValueInst *iVal) const {

  auto *iValOp = iVal->getInsertedValueOperand();

  // If the inserted value is an aggregate, we are operating on a complex
  // aggregate, skip for now
  if (iValOp->getType()->isAggregateType()) {
    MemInstrumentError::report(
        "Sub-aggregate inserts are not supported.\nInsertion found: ", iVal);
  }

  // Non-pointer inserts do not matter
  if (!iValOp->getType()->isPointerTy()) {
    return;
  }

  auto newTar = ITargetBuilder::createValInvariantTarget(iValOp, iVal);
  dest.push_back(newTar);
}
