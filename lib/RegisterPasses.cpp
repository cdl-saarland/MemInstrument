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
#include "meminstrument/optimizations/ExampleExternalChecksPass.h"
#include "meminstrument/optimizations/HotnessBasedCheckRemovalPass.h"
#include "meminstrument/pass/MemInstrumentPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;
using namespace meminstrument;

namespace llvm {
ModulePass *createEliminateAvailableExternallyPass();
ModulePass *createStripDeadPrototypesPass();
ModulePass *createMemInstrumentPass() { return new MemInstrumentPass(); }
} // namespace llvm

cl::opt<bool> NoMemInstrumentOpt(
    "mi-no-meminstrument",
    cl::desc(
        "Do not add meminstrument and required passes into the clang pipeline"),
    cl::init(false));

cl::opt<bool> HandleInvoke(
    "mi-handle-invoke",
    cl::desc("For benchmarks that contain invoke instructions, MemInstrument "
             "needs to break critical edges first. Hand this flag over in case "
             "your benchmarks contains invokes. Note that not every mechanism "
             "properly supports C++, where invokes often occur."),
    cl::init(false));

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

static RegisterPass<ExampleExternalChecksPass>
    RegisterExampleExternalChecksPass("mi-example-external-checks",
                                      "Example External Checks",
                                      false, // CFGOnly
                                      true); // isAnalysis

static void registerMeminstrumentPass(const PassManagerBuilder &,
                                      legacy::PassManagerBase &PM) {
  if (NoMemInstrumentOpt) {
    return;
  }

  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createCFGSimplificationPass());
  if (HandleInvoke) {
    // This is necessary for instrumenting invoke instructions that occur in C++
    // exception handling
    PM.add(createBreakCriticalEdgesPass());
  }
  // Cut out functions for which no code will be generated (they conflict with
  // the standard library wrappers of the run-time library).
  // TODO `Available externally` is only an issue for some mechanisms, so it
  // might be reasonable to make this pass optional for others. However, this
  // might have an impact on the performance comparability.
  PM.add(createEliminateAvailableExternallyPass());
  PM.add(createStripDeadPrototypesPass());
  PM.add(createMemInstrumentPass());
}

static RegisterStandardPasses
    RegisterMeminstrumentPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                              registerMeminstrumentPass);
} // namespace meminstrument
