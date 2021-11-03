//===- CheckGeneration.cpp - Generate Checks ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/CheckGeneration.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

using namespace meminstrument;
using namespace llvm;

static cl::opt<bool> NoInvariantChecks(
    "mi-no-invariant-checks",
    cl::desc("Don't place checks for invariants (this will horribly break e.g. "
             "SoftBound and gives less guarantees for other instrumentations, "
             "so make sure you know what you do)"),
    cl::init(false));

void meminstrument::generateInvariants(GlobalConfig &CFG, ITargetVector &Vec,
                                       Function &F) {

  if (NoInvariantChecks) {
    return;
  }

  auto &IM = CFG.getInstrumentationMechanism();

  for (auto &T : Vec) {
    if (T->isValid()) {
      if (T->isInvariant()) {
        IM.insertCheck(*T);
      }
    }
  }
}

void meminstrument::generateChecks(GlobalConfig &CFG, ITargetVector &Vec,
                                   Function &F) {
  auto &IM = CFG.getInstrumentationMechanism();

  for (auto &T : Vec) {
    if (T->isValid()) {
      if (T->isCheck()) {
        IM.insertCheck(*T);
      }
    }
  }
}
