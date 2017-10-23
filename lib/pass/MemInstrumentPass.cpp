//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/CheckGeneration.h"
#include "meminstrument/pass/DummyExternalChecksPass.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/ITargetFilters.h"
#include "meminstrument/pass/ITargetGathering.h"
#include "meminstrument/pass/WitnessGeneration.h"

#include "llvm/IR/Dominators.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

bool MemInstrumentPass::runOnModule(Module &M) {

  DEBUG(dbgs() << "MemInstrumentPass: processing module `" << M.getName().str()
               << "`\n";);

  DEBUG(dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  auto &IM = InstrumentationMechanism::get();
  IM.initialize(M);

  if (MIModeOpt == MIM_SETUP)
    return true;

  ITargetVector Targets;

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }

    Targets.clear();

    DEBUG(dbgs() << "MemInstrumentPass: processing function `"
                 << F.getName().str() << "`\n";);

    DEBUG(dbgs() << "MemInstrumentPass: gathering ITargets\n";);

    gatherITargets(Targets, F);

    if (MIModeOpt == MIM_GATHER_ITARGETS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: filtering ITargets with internal "
                    "filters\n";);

    filterITargets(this, Targets, F);

    auto &ECP = getAnalysis<DummyExternalChecksPass>();
    if (MIUseExternalChecksOpt) {
      DEBUG(dbgs() << "MemInstrumentPass: updating ITargets with pass `"
                   << ECP.getPassName() << "'\n";);
      ECP.updateITargetsForFunction(Targets, F);

      DEBUG_ALSO_WITH_TYPE(
          "meminstrument-external",
          dbgs() << "updated instrumentation targets after external filter:\n";
          for (auto &Target
               : Targets) { dbgs() << "  " << *Target << "\n"; });
    }

    if (MIModeOpt == MIM_FILTER_ITARGETS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(Targets, F);

    if (MIModeOpt == MIM_GENERATE_WITNESSES)
      continue;

    if (MIUseExternalChecksOpt) {
      DEBUG(
          dbgs() << "MemInstrumentPass: generating external checks with pass `"
                 << ECP.getPassName() << "'\n";);
      ECP.materializeExternalChecksForFunction(Targets, F);
    }

    if (MIModeOpt == MIM_GENERATE_EXTERNAL_CHECKS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating checks\n";);

    generateChecks(Targets, F);
  }

  return true;
}

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<DummyExternalChecksPass>();
}

char MemInstrumentPass::ID = 0;
