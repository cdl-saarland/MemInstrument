//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/NonePolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void NonePolicy::classifyTargets(
    std::vector<std::shared_ptr<ITarget>> &,
    llvm::Instruction *) { }

