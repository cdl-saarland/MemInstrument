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

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"

#include "meminstrument/pass/Util.h"

#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
#include "PMDA/PMDA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#define EXTERNAL_PASS checkoptimizer::CheckOptimizerPass
#else
#define EXTERNAL_PASS DummyExternalChecksPass
#endif

using namespace meminstrument;
using namespace llvm;

STATISTIC(NumVarArgs, "The # of modules with a function that has varargs");

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

void MemInstrumentPass::releaseMemory(void) {}

GlobalConfig &MemInstrumentPass::getConfig(void) { return *CFG; }

bool MemInstrumentPass::runOnModule(Module &M) {

  if (M.getName().endswith("tools/timeit.c")) {
    // small hack to avoid unnecessary work in the lnt tests
    DEBUG(dbgs() << "MemInstrumentPass: skip module `" << M.getName().str()
                 << "`\n";);
    return false;
  }

  for (auto &F : M) {
    if (!F.empty() && F.isVarArg()) {
      ++NumVarArgs;
      DEBUG(dbgs() << "MemInstrumentPass: skip module `" << M.getName().str()
                   << "` because of varargs\n";);
      return false;
    }
  }

  CFG = GlobalConfig::create(M);

  MIMode Mode = CFG->getMIMode();

  DEBUG(dbgs() << "MemInstrumentPass: processing module `" << M.getName().str()
               << "`\n";);

  DEBUG(
    dbgs() << "Dumped module:\n";
    M.dump();
    dbgs() << "\nEnd of dumped module.\n";
    );

  DEBUG(dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  auto &IM = CFG->getInstrumentationMechanism();
  IM.initialize(M);

  if (Mode == MIMode::SETUP || CFG->hasErrors())
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

    gatherITargets(*CFG, Targets, F);

    if (Mode == MIMode::GATHER_ITARGETS || CFG->hasErrors())
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: filtering ITargets with internal "
                    "filters\n";);

    filterITargets(*CFG, this, Targets, F);

    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-itargetfilter",
        dbgs() << "remaining instrumentation targets after filter:"
               << "\n";
        for (auto &Target
             : Targets) { dbgs() << "  " << *Target << "\n"; });
  }

  filterITargetsRandomly(*CFG, TargetMap);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    auto &ECP = getAnalysis<EXTERNAL_PASS>();
    if (CFG->hasUseExternalChecks()) {
      DEBUG(dbgs() << "MemInstrumentPass: updating ITargets with pass `"
                   << ECP.getPassName() << "'\n";);
      ECP.updateITargetsForFunction(*this, Targets, F);

      DEBUG_ALSO_WITH_TYPE(
          "meminstrument-external",
          dbgs() << "updated instrumentation targets after external filter:\n";
          for (auto &Target
               : Targets) { dbgs() << "  " << *Target << "\n"; });
    }

    if (Mode == MIMode::FILTER_ITARGETS || CFG->hasErrors())
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_WITNESSES || CFG->hasErrors())
      continue;

    if (CFG->hasUseExternalChecks()) {
      DEBUG(
          dbgs() << "MemInstrumentPass: generating external checks with pass `"
                 << ECP.getPassName() << "'\n";);
      ECP.materializeExternalChecksForFunction(*this, Targets, F);
    }

    if (Mode == MIMode::GENERATE_EXTERNAL_CHECKS || CFG->hasErrors())
      continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating checks\n";);

    generateChecks(*CFG, Targets, F);
  }

  return true;
}

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
#if MEMINSTRUMENT_USE_PMDA
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<pmda::PMDA>();
#endif
  AU.addRequired<EXTERNAL_PASS>();
}

void MemInstrumentPass::print(llvm::raw_ostream &O,
                              const llvm::Module *M) const {
  O << "MemInstrumentPass for module '" << M->getName() << "'\n";
}

char MemInstrumentPass::ID = 0;
