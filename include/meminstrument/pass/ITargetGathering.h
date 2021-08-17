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

/// Gather targets according to the config for the given function and insert
/// them into the target vector.
void gatherITargets(GlobalConfig &, ITargetVector &, llvm::Function &);

/// Collect statistics on the gathered targets
void collectStatistics(ITargetVector &);

} // namespace meminstrument

#endif
