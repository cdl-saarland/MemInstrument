//===- RegisterPasses.cpp - Register Meminstrument Pipeline ---------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/AnnotationBasedRemovalPass.h"
#include "meminstrument/optimizations/DominanceBasedCheckRemovalPass.h"
#include "meminstrument/optimizations/DummyExternalChecksPass.h"
#include "meminstrument/optimizations/HotnessBasedCheckRemovalPass.h"
#include "meminstrument/pass/MemInstrumentPass.h"

#include "lifetimekiller/LifeTimeKillerPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"

using namespace meminstrument;
using namespace llvm;

namespace llvm {
ModulePass *createEliminateAvailableExternallyPass();
ModulePass *createStripDeadPrototypesPass();
ModulePass *createMemInstrumentPass() { return new MemInstrumentPass(); }
} // namespace llvm

namespace {
cl::opt<bool>
    UseLifeTimeKillerOpt("mi-use-lifetime-killer",
                         cl::desc("Eliminate all llvm.lifetime.* intrinsics "
                                  "before memory safety instrumentation"),
                         cl::init(true));
cl::opt<bool> NoMemInstrumentOpt(
    "mi-no-meminstrument",
    cl::desc(
        "Do not add meminstrument and required passes into the clang pipeline"),
    cl::init(false));
} // namespace

namespace meminstrument {
static RegisterPass<MemInstrumentPass>
    RegisterMemInstrumentPass("meminstrument", "MemInstrument",
                              false,  // CFGOnly
                              false); // isAnalysis

// Registration for the provided optimization passes

static RegisterPass<AnnotationBasedRemovalPass>
    RegisterAnnotationBasedRemovalPass("mi-annotation-based-check-removal",
                                       "Annotation Based Check Removal Pass",
                                       false, // CFGOnly
                                       true); // isAnalysis

static RegisterPass<DominanceBasedCheckRemovalPass>
    RegisterDominanceBasedCheckRemovalPass("mi-dominance-based-check-removal",
                                           "Dominance Based Check Removal Pass",
                                           false, // CFGOnly
                                           true); // isAnalysis

static RegisterPass<HotnessBasedCheckRemovalPass>
    RegisterHotnessBasedCheckRemovalPass("mi-hotness-based-check-removal",
                                         "Hotness Based Check Removal Pass",
                                         false, // CFGOnly
                                         true); // isAnalysis

static RegisterPass<DummyExternalChecksPass>
    RegisterDummyExternalChecksPass("mi-dummy-external-checks",
                                    "Dummy External Checks",
                                    false, // CFGOnly
                                    true); // isAnalysis

static void registerMeminstrumentPass(const PassManagerBuilder &,
                                      legacy::PassManagerBase &PM) {
  if (UseLifeTimeKillerOpt) {
    PM.add(createLifeTimeKillerPass());
  }
  if (NoMemInstrumentOpt) {
    return;
  }

  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createCFGSimplificationPass());
  // This is necessary for instrumenting invoke instructions that occur in C++
  // exception handling:
  // PM.add(createBreakCriticalEdgesPass());
  // TODO this is only a requirement for SoftBound
  // Cut out functions for which no code will be generated (they conflict with
  // the standard library wrappers of the run-time library)
  PM.add(createEliminateAvailableExternallyPass());
  PM.add(createStripDeadPrototypesPass());
  PM.add(createMemInstrumentPass());
}

static RegisterStandardPasses
    RegisterMeminstrumentPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                              registerMeminstrumentPass);
} // namespace meminstrument
