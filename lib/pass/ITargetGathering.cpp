//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetGathering.h"

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/Util.h"

#include "llvm/IR/Module.h"

using namespace meminstrument;
using namespace llvm;

void meminstrument::gatherITargets(ITargetVector &Destination, Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();
  auto &IP = InstrumentationPolicy::get(DL);

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (hasNoInstrument(&I)) {
        continue;
      }
      IP.classifyTargets(Destination, &I);
    }
  }

  DEBUG_ALSO_WITH_TYPE(
      "meminstrument-itargetprovider",
      dbgs() << "identified instrumentation targets:"
             << "\n";
      for (auto &Target
           : Destination) { dbgs() << "  " << *Target << "\n"; });
}
