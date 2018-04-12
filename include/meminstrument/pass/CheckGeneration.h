//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_CHECKGENERATION_H
#define MEMINSTRUMENT_PASS_CHECKGENERATION_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "meminstrument/pass/ITarget.h"
#include "meminstrument/Config.h"

namespace meminstrument {

void generateChecks(GlobalConfig &CFG, ITargetVector &Vec, llvm::Function &F);
}

#endif
