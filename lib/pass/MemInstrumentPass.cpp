//===- MemInstrumentPass.cpp - Main Instrumentation Pass ------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/optimizations/OptimizationRunner.h"
#include "meminstrument/pass/CheckGeneration.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/ITargetGathering.h"
#include "meminstrument/pass/Setup.h"
#include "meminstrument/pass/WitnessGeneration.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

char MemInstrumentPass::ID = 0;

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

bool MemInstrumentPass::runOnModule(Module &M) {

  CFG = GlobalConfig::create(M);

  MIMode Mode = CFG->getMIMode();

  if (Mode == MIMode::NOTHING)
    return true;

  auto &IM = CFG->getInstrumentationMechanism();
  if (M.getName().endswith("tools/timeit.c") ||
      M.getName().endswith("fpcmp.c")) {
    // small hack to avoid unnecessary work in the lnt tests
    LLVM_DEBUG(dbgs() << "MemInstrumentPass: skip module `" << M.getName().str()
                      << "`\n";);
    return IM.skipInstrumentation(M);
  }

  LLVM_DEBUG(dbgs() << "Dumped module:\n"; M.dump();
             dbgs() << "\nEnd of dumped module.\n";);

  // Apply transformations required for all instrumentations
  prepareModule(M);

  LLVM_DEBUG(dbgs() << "MemInstrumentPass: processing module `"
                    << M.getName().str() << "`\n";);

  OptimizationRunner optRunner(*this);
  optRunner.runPreparation(M);

  LLVM_DEBUG(
      dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  IM.initialize(M);

  if (Mode == MIMode::SETUP || CFG->hasErrors())
    return true;

  std::map<Function *, ITargetVector> TargetMap;

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];
    LLVM_DEBUG(dbgs() << "MemInstrumentPass: gathering ITargets for function `"
                      << F.getName() << "`\n";);

    gatherITargets(*CFG, Targets, F);
  }

  if (Mode == MIMode::GATHER_ITARGETS || CFG->hasErrors())
    return true;

  optRunner.updateITargets(TargetMap);

  if (Mode == MIMode::FILTER_ITARGETS || CFG->hasErrors())
    return true;

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    LLVM_DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_WITNESSES || CFG->hasErrors())
      continue;

    generateInvariants(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_INVARIANTS || CFG->hasErrors()) {
      continue;
    }

    optRunner.placeChecks(Targets, F);

    if (Mode == MIMode::GENERATE_OPTIMIZATION_CHECKS || CFG->hasErrors())
      continue;

    LLVM_DEBUG(dbgs() << "MemInstrumentPass: generating checks\n";);

    generateChecks(*CFG, Targets, F);
  }

  DEBUG_ALSO_WITH_TYPE("meminstrument-finalmodule", M.dump(););

  return true;
}

void MemInstrumentPass::releaseMemory(void) { CFG.reset(nullptr); }

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  OptimizationRunner::addRequiredAnalyses(AU);
}

void MemInstrumentPass::print(raw_ostream &O, const Module *M) const {
  O << "MemInstrumentPass for module\n" << *M << "\n";
}

GlobalConfig &MemInstrumentPass::getConfig(void) { return *CFG; }
