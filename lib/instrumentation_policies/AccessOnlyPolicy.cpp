//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void AccessOnlyPolicy::classifyTargets(ITargetVector &Destination, llvm::Instruction *Location) {
  switch (Location->getOpcode()) {
  case Instruction::Load: {
    llvm::LoadInst *I = llvm::cast<llvm::LoadInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(ITarget::createSpatialCheckTarget(PtrOperand, Location, getPointerAccessSize(DL, PtrOperand)));
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *I = llvm::cast<llvm::StoreInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(ITarget::createSpatialCheckTarget(PtrOperand, Location, getPointerAccessSize(DL, PtrOperand)));
    break;
  }

  default:
    break;
  }
}
