//===- meminstrument/ITargetGathering.h - Gather ITargets -------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Collect ITargets according to the policy defined in the config.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_ITARGETGATHERING_H
#define MEMINSTRUMENT_PASS_ITARGETGATHERING_H

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"

namespace meminstrument {

void gatherITargets(GlobalConfig &CFG, ITargetVector &Destination,
                    llvm::Function &F);
}

#endif
