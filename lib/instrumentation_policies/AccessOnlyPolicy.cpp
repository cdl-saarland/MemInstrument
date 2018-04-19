//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"

#include "llvm/IR/Instructions.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void AccessOnlyPolicy::classifyTargets(ITargetVector &Destination,
                                       llvm::Instruction *Location) {
  llvm::Value *PtrOperand = nullptr;
  switch (Location->getOpcode()) {
  case Instruction::Load: {
    auto *I = llvm::cast<llvm::LoadInst>(Location);
    PtrOperand = I->getPointerOperand();
    break;
  }
  case Instruction::Store: {
    auto *I = llvm::cast<llvm::StoreInst>(Location);
    PtrOperand = I->getPointerOperand();
    break;
  }

  default:
    return;
  }

  if (!validateSize(PtrOperand)) {
    return;
  }

  Destination.push_back(
      ITarget::createSpatialCheckTarget(PtrOperand, Location));
}
