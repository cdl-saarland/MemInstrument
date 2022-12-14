//===- WitnessGeneration.cpp -- Generate Witnesses ------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///

#include "meminstrument/pass/WitnessGeneration.h"

#include "meminstrument/Config.h"
#include "meminstrument/Definitions.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FileSystem.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void meminstrument::generateWitnesses(GlobalConfig &CFG, ITargetVector &Vec,
                                      Function &F) {
  const auto &WS = CFG.getWitnessStrategy();
  auto &IM = CFG.getInstrumentationMechanism();

  WitnessGraph WG(F, WS);

  for (auto &Target : Vec) {
    if (Target->isValid()) {
      WG.insertRequiredTarget(Target);
    }
  }

  WG.propagateFlags();

  if (CFG.hasPrintWitnessGraph()) {
    WG.dumpDotGraph(("wg." + F.getName() + ".dot").str());
  }

  WS.simplifyWitnessGraph(IM, WG);

  if (CFG.hasPrintWitnessGraph()) {
    WG.dumpDotGraph(("wg.simplified." + F.getName() + ".dot").str());
  }

  WG.createWitnesses(IM);

  DEBUG_ALSO_WITH_TYPE("meminstrument-genwitnesses",
                       dbgs() << "targets with the same witnesses:"
                              << "\n";
                       WG.printWitnessClasses(dbgs()););

  for (auto &T : Vec) {
    if (!T->isValid()) {
      continue;
    }
    assert(T->hasWitnessesIfNeeded());
    if (T->requiresExplicitBounds()) {
      LLVM_DEBUG(dbgs() << "Materializing explicit bounds for " << *T << "\n";);
      IM.materializeBounds(*T);
      LLVM_DEBUG({
        for (const auto &KV : T->getBoundWitnesses()) {
          dbgs() << "  resulting explicit bounds for " << *T << ":\n";
          dbgs() << "    upper: " << *KV.second->getUpperBound() << "\n";
          dbgs() << "    lower: " << *KV.second->getLowerBound() << "\n";
        }
      });
    }
  }
}
