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

namespace {
bool handleInstrinsicInst(ITargetVector &Dest, llvm::IntrinsicInst *II) {
  switch (II->getIntrinsicID()) {
  case Intrinsic::memcpy:
  case Intrinsic::memmove: {
    auto *MT = cast<MemTransferInst>(II);
    auto *Len = MT->getLength();
    auto *Src = MT->getSource();
    auto *Dst = MT->getDest();
    Dest.push_back(ITarget::createSpatialCheckTarget(Src, II, Len));
    Dest.push_back(ITarget::createSpatialCheckTarget(Dst, II, Len));
    return true;
  }
  case Intrinsic::memset: {
    auto *MS = cast<MemSetInst>(II);
    auto *Len = MS->getLength();
    auto *Dst = MS->getDest();
    Dest.push_back(ITarget::createSpatialCheckTarget(Dst, II, Len));
    return true;
  }
  default:
    return false;
  }
}
} // namespace

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
  case Instruction::Call: {
    llvm::CallInst *I = llvm::cast<llvm::CallInst>(Location);
    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      handleInstrinsicInst(Destination, II);
    }
    return;
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
