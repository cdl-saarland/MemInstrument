//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "lifetimekiller/LifeTimeKillerPass.h"
#include "meminstrument/Definitions.h"
#include "meminstrument/pass/DummyExternalChecksPass.h"
#include "meminstrument/pass/MemInstrumentPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#if MEMINSTRUMENT_USE_PICO
#include "PICO/PICO.h"
#endif

namespace llvm {
ModulePass *createEliminateAvailableExternallyPass();
ModulePass *createStripDeadPrototypesPass();
} // namespace llvm

using namespace meminstrument;
using namespace llvm;

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

static RegisterPass<DummyExternalChecksPass>
    RegisterDummyExternalChecksPass("mi-dummy-external-checks",
                                    "Dummy External Checks",
                                    false, // CFGOnly
                                    true); // isAnalysis

static void registerMeminstrumentPass(const PassManagerBuilder &,
                                      legacy::PassManagerBase &PM) {
  if (UseLifeTimeKillerOpt) {
    PM.add(new lifetimekiller::LifeTimeKillerPass());
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
  PM.add(new MemInstrumentPass());
}

// #if MEMINSTRUMENT_USE_PICO
static RegisterStandardPasses
    RegisterMeminstrumentPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                              registerMeminstrumentPass);
// #else
// static RegisterStandardPasses
//     RegisterMeminstrumentPass(PassManagerBuilder::EP_OptimizerLast,
//                               registerMeminstrumentPass);
// #endif
} // namespace meminstrument
