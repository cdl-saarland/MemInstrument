//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/CheckGeneration.h"

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "llvm/ADT/Statistic.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void meminstrument::generateChecks(ITargetVector &Vec, llvm::Function &F) {
  auto &IM = InstrumentationMechanism::get();

  for (auto &T : Vec) {
    if (T->isValid()) {
      IM.insertCheck(*T);
    }
  }
}
