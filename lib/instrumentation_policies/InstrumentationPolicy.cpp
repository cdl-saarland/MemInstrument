//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"
#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {
STATISTIC(NumUnsizedTypes, "modules discarded because of unsized types");
}

InstrumentationPolicy::InstrumentationPolicy(GlobalConfig &cfg) : _CFG(cfg) {}

void InstrumentationPolicy::classifyTargetsArg(
    std::vector<std::shared_ptr<ITarget>> &, Argument *) {}

bool InstrumentationPolicy::validateSize(Value *Ptr) {
  if (!hasPointerAccessSize(Ptr)) {
    ++NumUnsizedTypes;
    LLVM_DEBUG(dbgs() << "Found pointer to unsized type: `" << *Ptr << "'!\n";);
    _CFG.noteError();
    return false;
  }
  return true;
}

bool InstrumentationPolicy::insertCheckTargetsForInstrinsic(ITargetVector &Dest,
                                                            IntrinsicInst *II) {
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

void InstrumentationPolicy::insertCheckTargetsLoadStore(ITargetVector &Dest,
                                                        Instruction *Inst) {

  assert(isa<LoadInst>(Inst) || isa<StoreInst>(Inst));

  auto PtrOp = isa<LoadInst>(Inst) ? cast<LoadInst>(Inst)->getPointerOperand()
                                   : cast<StoreInst>(Inst)->getPointerOperand();

  if (!validateSize(PtrOp)) {
    return;
  }

  Dest.push_back(ITarget::createSpatialCheckTarget(PtrOp, Inst));
}

void InstrumentationPolicy::insertInvariantTargetStore(ITargetVector &Dest,
                                                       StoreInst *Store) {

  auto *StoreOperand = Store->getValueOperand();
  if (!StoreOperand->getType()->isPointerTy()) {
    return;
  }

  Dest.push_back(ITarget::createInvariantTarget(StoreOperand, Store));
}

void InstrumentationPolicy::insertInvariantTargetLoad(ITargetVector &Dest,
                                                      LoadInst *Load) {
  if (!Load->getType()->isPointerTy()) {
    return;
  }

  Dest.push_back(ITarget::createInvariantTarget(Load, Load));
}

void InstrumentationPolicy::insertInvariantTargetReturn(ITargetVector &Dest,
                                                        llvm::ReturnInst *Ret) {

  auto *Operand = Ret->getReturnValue();
  if (!Operand || !Operand->getType()->isPointerTy()) {
    return;
  }

  Dest.push_back(ITarget::createInvariantTarget(Operand, Ret));
}
