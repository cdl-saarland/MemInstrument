//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/CheckGeneration.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "llvm/ADT/Statistic.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void meminstrument::generateChecks(GlobalConfig &CFG, ITargetVector &Vec,
                                   llvm::Function &F) {
  auto &IM = CFG.getInstrumentationMechanism();

  for (auto &T : Vec) {
    if (T->isValid()) {
      if (T->isCheck() || T->is(ITarget::Kind::Invariant)) {
        IM.insertCheck(*T);
      }
    }
  }
}
