//===- DominanceBasedCheckRemovalPass.cpp - Remove Dominated Checks -------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/DominanceBasedCheckRemovalPass.h"

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;
using namespace meminstrument;

cl::opt<bool> InvariantIsCheck(
    "mi-opt-dominance-invariant-is-check",
    cl::desc("Assume that invariants are (one byte) checks (holds for splay "
             "and lowfat, but not for softbound)"),
    cl::init(false));

STATISTIC(NumITargetsSubsumed, "The # of instrumentation targets discarded "
                               "because of dominating subsumption");

//===--------------------------- ModulePass -------------------------------===//

char DominanceBasedCheckRemovalPass::ID = 0;

DominanceBasedCheckRemovalPass::DominanceBasedCheckRemovalPass()
    : ModulePass(ID) {}

bool DominanceBasedCheckRemovalPass::runOnModule(Module &) {
  LLVM_DEBUG(dbgs() << "Running Dominance Based Removal Pass\n";);
  return false;
}

void DominanceBasedCheckRemovalPass::getAnalysisUsage(
    AnalysisUsage &analysisUsage) const {
  analysisUsage.setPreservesAll();
}

void DominanceBasedCheckRemovalPass::print(raw_ostream &stream,
                                           const Module *module) const {
  stream << "Running Dominance Based Removal Pass on\n" << *module << "\n";
}

//===--------------------- OptimizationInterface --------------------------===//

void DominanceBasedCheckRemovalPass::updateITargetsForFunction(
    MemInstrumentPass &mip, ITargetVector &targets, Function &fun) {

  const auto &domTree =
      mip.getAnalysis<DominatorTreeWrapperPass>(fun).getDomTree();

  for (auto &target : targets) {
    for (auto &otherTarget : targets) {

      // Don't optimize a target based on the assumption that it itsself is
      // checked
      if (target == otherTarget) {
        continue;
      }

      // Don't compare targets if one is already invalidated
      if (!target->isValid() || !otherTarget->isValid()) {
        continue;
      }

      // Compare if the acccess width of one target is larger than the other one
      // and it also dominates it.
      if (subsumes(*target, *otherTarget) &&
          domTree.dominates(target->getLocation(),
                            otherTarget->getLocation())) {
        otherTarget->invalidate();
        ++NumITargetsSubsumed;
      }
    }
  }
}

//===---------------------------- private ---------------------------------===//

bool DominanceBasedCheckRemovalPass::subsumes(const ITarget &one,
                                              const ITarget &other) const {
  assert(one.isValid());
  assert(other.isValid());

  if (!one.hasInstrumentee() || !other.hasInstrumentee())
    return false;

  // If the instrumentee is not the same, the check cannot be subsumed
  if (one.getInstrumentee() != other.getInstrumentee())
    return false;

  // If one access has a statically larger access width than the other, it
  // subsumes it
  if (auto constSizeTarget = dyn_cast<ConstSizeCheckIT>(&one)) {
    if (auto constSizeTargetOther = dyn_cast<ConstSizeCheckIT>(&other)) {
      return constSizeTarget->getAccessSize() >=
             constSizeTargetOther->getAccessSize();
    }
  }

  // If the accesses have the same (dynamic) width, they subsume each other
  if (auto constSizeTarget = dyn_cast<VarSizeCheckIT>(&one)) {
    if (auto constSizeTargetOther = dyn_cast<VarSizeCheckIT>(&other)) {
      return constSizeTarget->getAccessSizeVal() ==
             constSizeTargetOther->getAccessSizeVal();
    }
  }

  if (InvariantIsCheck) {
    // TODO this only holds for instrumentations that use one-byte "check"
    // invariants. Find a proper solution to report an error if this CL flag is
    // given and an instrumentation that does not support that this is used.
    if (other.isInvariant()) {
      return one.isInvariant() || isa<ConstSizeCheckIT>(&one) ||
             isa<VarSizeCheckIT>(&one);
    }
  }

  return false;
}
