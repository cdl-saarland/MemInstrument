//===- OptimizationRunner.cpp - Optimization Runner -----------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "meminstrument/optimizations/OptimizationRunner.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/optimizations/DummyExternalChecksPass.h"

#if PICO_AVAILABLE
#include "PICO/PICO.h"
#include "PMDA/PMDA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#endif

#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace meminstrument;

cl::list<InstrumentationOptimizations> OptimizationList(
    cl::desc("Available Optimizations:"),
    cl::values(clEnumValN(dominance_checkrem, "mi-opt-dominance",
                          "Dominance based filter"),
               clEnumValN(dummy_checkopt, "mi-opt-dummy",
                          "Dummy external checks"),
               clEnumValN(pico_checkopt, "mi-opt-pico", "PICO")));

OptimizationRunner::OptimizationRunner(MemInstrumentPass &mip)
    : mip(mip), selectedOpts(computeSelectedOpts(mip)) {}

void OptimizationRunner::runPreparation(Module &module) const {
  for (auto opt : selectedOpts) {
    opt->prepareModule(mip, module);
  }
}

void OptimizationRunner::updateITargets(ITargetVector &targets,
                                        Function &fun) const {
  for (auto opt : selectedOpts) {
    opt->updateITargetsForFunction(mip, targets, fun);
  }
}

void OptimizationRunner::placeChecks(ITargetVector &targets,
                                     Function &fun) const {
  for (auto opt : selectedOpts) {
    opt->materializeExternalChecksForFunction(mip, targets, fun);
  }
}

void OptimizationRunner::addRequiredAnalyses(AnalysisUsage &AU) {

  for (auto &opt : OptimizationList) {

    switch (opt) {
    case InstrumentationOptimizations::dominance_checkrem:
      /* TODO */
      break;
    case InstrumentationOptimizations::dummy_checkopt:
      AU.addRequired<DummyExternalChecksPass>();
      break;
    case InstrumentationOptimizations::pico_checkopt:
#if !PICO_AVAILABLE
      llvm_unreachable("PICO selected but not available.");
#else
      AU.addRequired<ScalarEvolutionWrapperPass>();
      AU.addRequired<pmda::PMDA>();
      AU.addRequired<pico::PICO>();
#endif
      break;

    default:
      llvm_unreachable("Unhandled optimization option");
      break;
    }
  }
}

OptimizationRunner::~OptimizationRunner() {}

//===---------------------------- private ---------------------------------===//

auto OptimizationRunner::computeSelectedOpts(MemInstrumentPass &mi)
    -> SmallVector<OptimizationInterface *, 2> {

  SmallVector<OptimizationInterface *, 2> opts;

  for (auto &opt : OptimizationList) {

    switch (opt) {
    case InstrumentationOptimizations::dominance_checkrem:
      /* TODO */
      break;
    case InstrumentationOptimizations::dummy_checkopt:
      opts.push_back(&mi.getAnalysis<DummyExternalChecksPass>());
      break;
    case InstrumentationOptimizations::pico_checkopt:
#if !PICO_AVAILABLE
      llvm_unreachable("PICO selected but not available.");
#else
      opts.push_back(&mi.getAnalysis<pico::PICO>());
#endif
      break;

    default:
      llvm_unreachable("Unhandled optimization option");
      break;
    }
  }

  return opts;
}
