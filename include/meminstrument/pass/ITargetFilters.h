//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_ITARGETFILTERS_H
#define MEMINSTRUMENT_PASS_ITARGETFILTERS_H

#include "meminstrument/pass/ITarget.h"
#include "meminstrument/Config.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

void filterITargets(GlobalConfig &CFG, llvm::Pass *P, ITargetVector &Vec, llvm::Function &F);

void filterITargetsRandomly(GlobalConfig &CFG,
    std::map<llvm::Function *, ITargetVector> TargetMap);

} // namespace meminstrument

#endif
