//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/ITargetGathering.h"
#include "meminstrument/pass/ITargetFilters.h"
#include "meminstrument/pass/WitnessGeneration.h"
#include "meminstrument/pass/CheckGeneration.h"

#include "llvm/IR/Dominators.h"

#include "meminstrument/pass/Util.h"


using namespace meminstrument;
using namespace llvm;

namespace {

enum MIMode {
  MIM_SETUP,
  MIM_GATHER_ITARGETS,
  MIM_FILTER_ITARGETS,
  MIM_GENERATE_WITNESSES,
  MIM_GENERATE_EXTERNAL_CHECKS,
  MIM_GENERATE_CHECKS,
};

cl::opt<MIMode> MIModeOpt(
    "mi-mode", cl::desc("Choose until which stage instrumentation should be performed: (default: genchecks)"),
    cl::values(clEnumValN(MIM_SETUP, "setup", "only until setup is done")),
    cl::values(clEnumValN(MIM_GATHER_ITARGETS, "gatheritargets", "only until ITarget gathering is done")),
    cl::values(clEnumValN(MIM_FILTER_ITARGETS, "filteritargets", "only until ITarget filtering is done")),
    cl::values(clEnumValN(MIM_GENERATE_WITNESSES, "genwitnesses", "only until witness generation is done")),
    cl::values(clEnumValN(MIM_GENERATE_EXTERNAL_CHECKS, "extchecks", "only until external check generation is done")),
    cl::values(clEnumValN(MIM_GENERATE_CHECKS, "genchecks", "the full pipline")),
    cl::init(MIM_GENERATE_CHECKS) // default
);
} // namespace

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

bool MemInstrumentPass::runOnModule(Module &M) {

  DEBUG(dbgs() << "MemInstrumentPass: processing module `"
               << M.getName().str() << "`\n";);

  DEBUG(dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  auto &IM = InstrumentationMechanism::get();
  IM.initialize(M);

  if (MIModeOpt == MIM_SETUP) return true;

  ITargetVector Targets;

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }

    DEBUG(dbgs() << "MemInstrumentPass: processing function `"
                 << F.getName().str() << "`\n";);

    DEBUG(dbgs() << "MemInstrumentPass: gathering ITargets\n";);

    gatherITargets(Targets, F);

    if (MIModeOpt == MIM_GATHER_ITARGETS) continue;

    DEBUG(dbgs() << "MemInstrumentPass: filtering ITargets\n";);

    filterITargets(this, Targets, F);

    if (MIModeOpt == MIM_FILTER_ITARGETS) continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(Targets, F);

    if (MIModeOpt == MIM_GENERATE_WITNESSES) continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating external checks\n";);

    // generateExternalChecks(Targets, F);

    if (MIModeOpt == MIM_GENERATE_EXTERNAL_CHECKS) continue;

    DEBUG(dbgs() << "MemInstrumentPass: generating checks\n";);

    generateChecks(Targets, F);

    Targets.clear();
  }

  return true;
}

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
}

char MemInstrumentPass::ID = 0;
