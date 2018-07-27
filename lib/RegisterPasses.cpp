//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Definitions.h"
#include "meminstrument/pass/DummyExternalChecksPass.h"
#include "meminstrument/pass/MemInstrumentPass.h"
#include "meminstrument/lifetimekiller/LifeTimeKillerPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
#endif

using namespace meminstrument;
using namespace llvm;

namespace {
cl::opt<bool>
    UseLifeTimeKillerOpt("mi-use-lifetime-killer",
                  cl::desc("Eliminate all llvm.lifetime.* intrinsics before memory safety instrumentation"),
                  cl::init(true)
                  );
}

namespace meminstrument {
static RegisterPass<MemInstrumentPass>
    RegisterMemInstrumentPass("meminstrument", "MemInstrument",
                              false,  // CFGOnly
                              false); // isAnalysis

static RegisterPass<LifeTimeKillerPass>
    RegisterLifeTimeKillerPass("lifetimekiller", "LifeTimeKiller",
                              false,  // CFGOnly
                              false); // isAnalysis

static RegisterPass<DummyExternalChecksPass>
    RegisterDummyExternalChecksPass("mi-dummy-external-checks",
                                    "Dummy External Checks",
                                    false, // CFGOnly
                                    true); // isAnalysis

static void registerMeminstrumentPass(const llvm::PassManagerBuilder &,
                                      llvm::legacy::PassManagerBase &PM) {
  if (UseLifeTimeKillerOpt) {
    PM.add(new LifeTimeKillerPass());
  }
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createCFGSimplificationPass());
  PM.add(createBreakCriticalEdgesPass());
  PM.add(new MemInstrumentPass());
}

// #if MEMINSTRUMENT_USE_PMDA
static llvm::RegisterStandardPasses
    RegisterMeminstrumentPass(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
                              registerMeminstrumentPass);
// #else
// static llvm::RegisterStandardPasses
//     RegisterMeminstrumentPass(llvm::PassManagerBuilder::EP_OptimizerLast,
//                               registerMeminstrumentPass);
// #endif
} // namespace meminstrument
