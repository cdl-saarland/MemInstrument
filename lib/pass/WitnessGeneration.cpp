//===----- pass/WitnessGeneration.cpp -- MemSafety Instrumentation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/WitnessGeneration.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FileSystem.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {
cl::opt<bool> PrintWitnessGraphOpt("mi-print-witnessgraph",
                                   cl::desc("Print the WitnessGraph"),
                                   cl::init(false) // default
);

cl::opt<bool>
    NoSimplifyWitnessGraphOpt("mi-no-simplify-witnessgraph",
                              cl::desc("Disable witness graph simplifications"),
                              cl::init(false) // default
    );
}

void generateWitnesses(ITargetVector &Vec, Function &F) {
  const auto &WS = WitnessStrategy::get();
  auto &IM = InstrumentationMechanism::get();

  WitnessGraph WG(F, WS);

  for (auto &Target : Vec) {
    if (Target->isValid()) {
      WG.insertRequiredTarget(Target);
    }
  }

  WG.propagateFlags();

  if (PrintWitnessGraphOpt) {
    WG.dumpDotGraph(("wg." + F.getName() + ".dot").str());
  }

  if (!NoSimplifyWitnessGraphOpt) {
    WS.simplifyWitnessGraph(WG);

    if (PrintWitnessGraphOpt) {
      WG.dumpDotGraph(("wg.simplified." + F.getName() + ".dot").str());
    }
  }

  WG.createWitnesses(IM);

  DEBUG_ALSO_WITH_TYPE("meminstrument-genwitnesses",
                       dbgs() << "targets with the same witnesses:"
                              << "\n";
                       WG.printWitnessClasses(dbgs()););

  for (auto &T : Vec) {
    if (T->isValid() && T->RequiresExplicitBounds) {
      IM.materializeBounds(*T);
    }
  }
}
