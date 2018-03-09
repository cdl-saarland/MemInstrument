//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"
#include "meminstrument/Definitions.h"

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

#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
#define EXTERNAL_PASS checkoptimizer::CheckOptimizerPass
#else
#define EXTERNAL_PASS DummyExternalChecksPass
#endif

using namespace meminstrument;
using namespace llvm;

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

void MemInstrumentPass::releaseMemory(void) { GlobalConfig::release(); }

bool MemInstrumentPass::runOnModule(Module &M) {

  if (M.getName().endswith("tools/timeit.c")) {
    // small hack to avoid unnecessary work in the lnt tests
    DEBUG(dbgs() << "MemInstrumentPass: skip module `" << M.getName().str()
                 << "`\n";);
    return false;
  }

  auto &CFG = GlobalConfig::get(M);

  Config::MIMode Mode = CFG.getMIMode();

  DEBUG(dbgs() << "MemInstrumentPass: processing module `" << M.getName().str()
               << "`\n";);

  DEBUG(dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  auto &IM = CFG.getInstrumentationMechanism();
  IM.initialize(M);

  if (Mode == Config::MIMode::SETUP)
    return true;

  std::map<llvm::Function *, ITargetVector> TargetMap;

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    DEBUG(dbgs() << "MemInstrumentPass: processing function `"
                 << F.getName().str() << "`\n";);

    DEBUG(dbgs() << "MemInstrumentPass: gathering ITargets\n";);

    gatherITargets(Targets, F);

    if (Mode == Config::MIMode::GATHER_ITARGETS)
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: filtering ITargets with internal "
                    "filters\n";);

    filterITargets(this, Targets, F);

    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-itargetfilter",
        dbgs() << "remaining instrumentation targets after filter:"
               << "\n";
        for (auto &Target
             : Targets) { dbgs() << "  " << *Target << "\n"; });
  }

  filterITargetsRandomly(TargetMap);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    auto &ECP = getAnalysis<EXTERNAL_PASS>();
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
  AU.addRequired<EXTERNAL_PASS>();
}


void MemInstrumentPass::print(llvm::raw_ostream &O, const llvm::Module *M) const {
  O << "MemInstrumentPass for module '" << M->getName() << "'\n";
}

char MemInstrumentPass::ID = 0;
