//===- AccessOnlyPolicy.cpp - Policy to Check only Accesses ---------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void AccessOnlyPolicy::classifyTargets(ITargetVector &Destination,
                                       Instruction *Location) {

  if (hasVarArgHandling(Location)) {
    return;
  }

  if (isa<LoadInst>(Location) || isa<StoreInst>(Location)) {
    insertCheckTargetsLoadStore(Destination, Location);
    return;
  }

  if (auto *II = dyn_cast<IntrinsicInst>(Location)) {
    insertCheckTargetsForIntrinsic(Destination, II);
  }
}
