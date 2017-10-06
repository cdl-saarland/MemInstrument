//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_ITARGETGATHERING_H
#define MEMINSTRUMENT_PASS_ITARGETGATHERING_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"

namespace meminstrument {

void gatherITargets(ITargetVector& Destination, llvm::Function &F);

}

#endif
