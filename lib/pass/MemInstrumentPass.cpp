//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"

#include "meminstrument/Config.h"
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

  auto& CFG = GlobalConfig::get(M);

  Config::MIMode Mode = CFG.getMIMode();

  DEBUG(dbgs() << "MemInstrumentPass: processing module `" << M.getName().str()
               << "`\n";);

  DEBUG(dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  auto &IM = CFG.getInstrumentationMechanism();
  IM.initialize(M);

  if (Mode == Config::MIMode::SETUP)
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

    if (Mode == Config::MIMode::GATHER_ITARGETS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: filtering ITargets with internal "
                    "filters\n";);

    filterITargets(this, Targets, F);

    auto &ECP = getAnalysis<DummyExternalChecksPass>();
    if (CFG.hasUseExternalChecks()) {
      DEBUG(dbgs() << "MemInstrumentPass: updating ITargets with pass `"
                   << ECP.getPassName() << "'\n";);
      ECP.updateITargetsForFunction(Targets, F);

      DEBUG_ALSO_WITH_TYPE(
          "meminstrument-external",
          dbgs() << "updated instrumentation targets after external filter:\n";
          for (auto &Target
               : Targets) { dbgs() << "  " << *Target << "\n"; });
    }

    if (Mode == Config::MIMode::FILTER_ITARGETS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(Targets, F);

    if (Mode == Config::MIMode::GENERATE_WITNESSES)
      continue;

    if (CFG.hasUseExternalChecks()) {
      DEBUG(
          dbgs() << "MemInstrumentPass: generating external checks with pass `"
                 << ECP.getPassName() << "'\n";);
      ECP.materializeExternalChecksForFunction(Targets, F);
    }

    if (Mode == Config::MIMode::GENERATE_EXTERNAL_CHECKS)
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
