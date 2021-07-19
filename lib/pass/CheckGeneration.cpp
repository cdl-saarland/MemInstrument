//===- CheckGeneration.cpp - Generate Checks ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/CheckGeneration.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "llvm/ADT/Statistic.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void meminstrument::generateChecks(GlobalConfig &CFG, ITargetVector &Vec,
                                   Function &F) {
  auto &IM = CFG.getInstrumentationMechanism();

  for (auto &T : Vec) {
    if (T->isValid()) {
      if (T->isCheck() || T->isInvariant()) {
        IM.insertCheck(*T);
      }
    }
  }
}
