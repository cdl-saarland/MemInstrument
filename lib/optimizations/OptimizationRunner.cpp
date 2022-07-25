//===- OptimizationRunner.cpp - Optimization Runner -----------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "meminstrument/optimizations/OptimizationRunner.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/optimizations/AnnotationBasedRemovalPass.h"
#include "meminstrument/optimizations/DominanceBasedCheckRemovalPass.h"
#include "meminstrument/optimizations/DummyExternalChecksPass.h"
#include "meminstrument/optimizations/HotnessBasedCheckRemovalPass.h"
#include "meminstrument/pass/Util.h"

#if PICO_AVAILABLE
#include "PICO/PICO.h"
#include "PMDA/PMDA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#endif

#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace meminstrument;

cl::list<InstrumentationOptimizations> OptimizationList(
    cl::desc("Available Optimizations:"),
    cl::values(
        clEnumValN(annotation_checkrem, "mi-opt-annotation",
                   "Annotation based filter"),
        clEnumValN(dominance_checkrem, "mi-opt-dominance",
                   "Dominance based filter"),
        clEnumValN(hotness_checkrem, "mi-opt-hotness", "Hotness based filter"),
        clEnumValN(dummy_checkopt, "mi-opt-dummy", "Dummy external checks"),
        clEnumValN(pico_checkopt, "mi-opt-pico", "PICO")));

OptimizationRunner::OptimizationRunner(MemInstrumentPass &mip)
    : mip(mip), selectedOpts(computeSelectedOpts(mip)) {}

void OptimizationRunner::runPreparation(Module &module) const {

  DEBUG_ALSO_WITH_TYPE("meminstrument-opt", {
    dbgs() << "MemInstrumentPass: running preparatory code for "
           << selectedOpts.size() << " optimizations\n";
  });

  for (auto opt : selectedOpts) {
    opt->prepareModule(mip, module);
  }
}

void OptimizationRunner::updateITargets(
    std::map<Function *, ITargetVector> &targetsPerFun) const {

  DEBUG_ALSO_WITH_TYPE("meminstrument-opt", {
    dbgs() << "MemInstrumentPass: updating ITargets with optimizations\n";
  });

  for (auto opt : selectedOpts) {
    opt->updateITargetsForModule(mip, targetsPerFun);
    validate(targetsPerFun);
  }
}

void OptimizationRunner::placeChecks(ITargetVector &targets,
                                     Function &fun) const {

  DEBUG_ALSO_WITH_TYPE("meminstrument-opt", {
    dbgs() << "MemInstrumentPass: generating optimized checks\n";
  });

  for (auto opt : selectedOpts) {
    opt->materializeExternalChecksForFunction(mip, targets, fun);
  }
}

void OptimizationRunner::addRequiredAnalyses(AnalysisUsage &analysisUsage) {

  analysisUsage.addRequired<DominatorTreeWrapperPass>();
  for (auto &opt : OptimizationList) {

    switch (opt) {
    case InstrumentationOptimizations::annotation_checkrem:
      analysisUsage.addRequired<AnnotationBasedRemovalPass>();
      break;
    case InstrumentationOptimizations::dominance_checkrem:
      analysisUsage.addRequired<DominanceBasedCheckRemovalPass>();
      break;
    case InstrumentationOptimizations::hotness_checkrem:
      analysisUsage.addRequired<HotnessBasedCheckRemovalPass>();
      break;
    case InstrumentationOptimizations::dummy_checkopt:
      analysisUsage.addRequired<DummyExternalChecksPass>();
      break;
    case InstrumentationOptimizations::pico_checkopt:
#if !PICO_AVAILABLE
      MemInstrumentError::report("PICO selected but not available.");
#else
      analysisUsage.addRequired<ScalarEvolutionWrapperPass>();
      analysisUsage.addRequired<pmda::PMDA>();
      analysisUsage.addRequired<pico::PICO>();
#endif
      break;

    default:
      llvm_unreachable("Unknown optimization option");
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
    case InstrumentationOptimizations::annotation_checkrem:
      opts.push_back(&mi.getAnalysis<AnnotationBasedRemovalPass>());
      break;
    case InstrumentationOptimizations::dominance_checkrem:
      opts.push_back(&mi.getAnalysis<DominanceBasedCheckRemovalPass>());
      break;
    case InstrumentationOptimizations::hotness_checkrem:
      opts.push_back(&mi.getAnalysis<HotnessBasedCheckRemovalPass>());
      break;
    case InstrumentationOptimizations::dummy_checkopt:
      opts.push_back(&mi.getAnalysis<DummyExternalChecksPass>());
      break;
    case InstrumentationOptimizations::pico_checkopt:
#if !PICO_AVAILABLE
      MemInstrumentError::report("PICO selected but not available.");
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

void OptimizationRunner::validate(
    const std::map<Function *, ITargetVector> &targetsPerFun) const {

  for ([[maybe_unused]] const auto &entry : targetsPerFun) {

    DEBUG_ALSO_WITH_TYPE("meminstrument-opt", {
      dbgs() << "updated instrumentation targets after optimization:\n";
      for (auto &target : entry.second) {
        dbgs() << "  " << *target << "\n";
      }
    });

    assert(ITargetBuilder::validateITargets(
        mip.getAnalysis<DominatorTreeWrapperPass>(*entry.first).getDomTree(),
        entry.second));
  }
}
