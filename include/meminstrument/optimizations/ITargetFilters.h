//===- meminstrument/ITargetFilters.h - Various ITarget filters -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_ITARGETFILTERS_H
#define MEMINSTRUMENT_OPTIMIZATION_ITARGETFILTERS_H

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

void filterITargets(GlobalConfig &CFG, llvm::Pass *P, ITargetVector &Vec,
                    llvm::Function &F);

void filterITargetsRandomly(
    GlobalConfig &CFG, std::map<llvm::Function *, ITargetVector> TargetMap);

} // namespace meminstrument

#endif
