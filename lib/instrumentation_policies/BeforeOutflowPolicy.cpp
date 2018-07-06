//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/pass/Util.h"

STATISTIC(NumITargetsGathered,
          "The # of instrumentation targets initially gathered");

STATISTIC(NumIntrinsicsHandled, "The # of intrinsics that could be handled");
STATISTIC(NumIntrinsicsNotHandled,
          "The # of intrinsics that could not be handled");

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

void BeforeOutflowPolicy::classifyTargets(ITargetVector &Dest,
                                          llvm::Instruction *Location) {
  switch (Location->getOpcode()) {
  case Instruction::Ret: {
    llvm::ReturnInst *I = llvm::cast<llvm::ReturnInst>(Location);

    auto *Operand = I->getReturnValue();
    if (!Operand || !Operand->getType()->isPointerTy()) {
      return;
    }

    Dest.push_back(ITarget::createInvariantTarget(Operand, Location));
    ++NumITargetsGathered;
    break;
  }
  case Instruction::Call: {
    llvm::CallInst *I = llvm::cast<llvm::CallInst>(Location);

    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (handleInstrinsicInst(Dest, II)) {
        ++NumIntrinsicsHandled;
        break;
      }
      ++NumIntrinsicsNotHandled;
    }

    auto *Fun = I->getCalledFunction();
    if (!Fun) { // call via function pointer
      if (!validateSize(I->getCalledValue())) {
        return;
      }
      Dest.push_back(
          ITarget::createSpatialCheckTarget(I->getCalledValue(), Location, 1));
      ++NumITargetsGathered;
    }
    if (Fun && Fun->hasName() && Fun->getName().startswith("llvm.dbg.")) {
      // skip debug information pseudo-calls
      break;
    }
    bool FunIsNoVarArg = Fun && !Fun->isVarArg();
    // FIXME If we know the function, we can insert actual dereference checks
    // for byval arguments. Otherwise, we have to hope for the best.
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

      if (FunIsNoVarArg && ArgIt->hasByValAttr()) {
        if (!validateSize(Operand)) {
          return;
        }
        Dest.push_back(ITarget::createSpatialCheckTarget(Operand, Location));
      } else {
        Dest.push_back(ITarget::createInvariantTarget(Operand, Location));
      }

      ++NumITargetsGathered;

      if (FunIsNoVarArg) {
        ++ArgIt;
      }
    }

    break;
  }
  case Instruction::Load: {
    llvm::LoadInst *I = llvm::cast<llvm::LoadInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    if (!validateSize(PtrOperand)) {
      return;
    }
    Dest.push_back(ITarget::createSpatialCheckTarget(PtrOperand, Location));
    ++NumITargetsGathered;
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *I = llvm::cast<llvm::StoreInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    if (!validateSize(PtrOperand)) {
      return;
    }
    Dest.push_back(ITarget::createSpatialCheckTarget(PtrOperand, Location));
    ++NumITargetsGathered;

    auto *StoreOperand = I->getValueOperand();
    if (!StoreOperand->getType()->isPointerTy()) {
      return;
    }

    Dest.push_back(ITarget::createInvariantTarget(StoreOperand, Location));
    ++NumITargetsGathered;

    break;
  }

  case Instruction::Invoke:
  case Instruction::Resume:
  case Instruction::IndirectBr:
  case Instruction::AtomicCmpXchg:
  case Instruction::AtomicRMW:
    // TODO implement
    DEBUG(dbgs() << "skipping unimplemented instruction " << Location->getName()
                 << "\n";);
    break;

  default:
    break;
  }
}
