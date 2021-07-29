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

using namespace meminstrument;
using namespace llvm;

void meminstrument::gatherITargets(GlobalConfig &CFG,
                                   ITargetVector &Destination, Function &F) {
  auto &IP = CFG.getInstrumentationPolicy();

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (hasNoInstrument(&I)) {
        continue;
      }
      IP.classifyTargets(Destination, &I);
    }
  }

  NumITargetsGathered += Destination.size();

  DEBUG_ALSO_WITH_TYPE("meminstrument-itargetprovider", {
    dbgs() << "identified instrumentation targets:\n";
    for (auto &Target : Destination) {
      dbgs() << "  " << *Target << "\n";
    }
  });
}
