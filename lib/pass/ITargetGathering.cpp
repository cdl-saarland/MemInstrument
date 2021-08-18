//===- ITargetGathering.cpp - Gather ITargets -----------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetGathering.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"

STATISTIC(NumITargetsGathered,
          "The # of instrumentation targets initially gathered");
STATISTIC(NumInvariantTargets,
          "The # of invariant instrumentation targets initially gathered");
STATISTIC(AccessCheckTargets,
          "The # of access check instrumentation targets initially gathered");
STATISTIC(CallCheckTargets,
          "The # of call check instrumentation targets initially gathered");
STATISTIC(BoundsTargets,
          "The # of bounds instrumentation targets initially gathered");
STATISTIC(IntermediateTargets,
          "The # of intermediate instrumentation targets initially gathered");

using namespace meminstrument;
using namespace llvm;

void meminstrument::gatherITargets(GlobalConfig &globalConfig,
                                   ITargetVector &destination, Function &fun) {
  auto &IP = globalConfig.getInstrumentationPolicy();

  for (auto &bb : fun) {
    for (auto &inst : bb) {
      if (hasNoInstrument(&inst)) {
        continue;
      }
      IP.classifyTargets(destination, &inst);
    }
  }

  collectStatistics(destination);

  DEBUG_ALSO_WITH_TYPE("meminstrument-itargetprovider", {
    dbgs() << "identified instrumentation targets:\n";
    for (const auto &target : destination) {
      dbgs() << "  " << *target << "\n";
    }
  });
}

void meminstrument::collectStatistics(ITargetVector &targets) {
  NumITargetsGathered += ITargetBuilder::getNumValidITargets(targets);

  for (const auto &target : targets) {

    // Skip already invalidated targets
    if (!target->isValid()) {
      continue;
    }

    if (isa<InvariantIT>(target)) {
      ++NumInvariantTargets;
    }
    if (isa<ConstSizeCheckIT>(target) || isa<VarSizeCheckIT>(target)) {
      ++AccessCheckTargets;
    }
    if (isa<CallCheckIT>(target)) {
      ++CallCheckTargets;
    }
    if (isa<BoundsIT>(target)) {
      ++BoundsTargets;
    }
    if (isa<IntermediateIT>(target)) {
      ++IntermediateTargets;
    }
  }
}
