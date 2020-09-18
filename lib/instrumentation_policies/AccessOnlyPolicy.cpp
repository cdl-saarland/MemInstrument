//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
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

  if (isa<LoadInst>(Location) || isa<StoreInst>(Location)) {
    insertCheckTargetsLoadStore(Destination, Location);
    return;
  }

  if (auto *II = dyn_cast<IntrinsicInst>(Location)) {
    insertCheckTargetsForInstrinsic(Destination, II);
  }
}
