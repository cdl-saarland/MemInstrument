//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Definitions.h"
#include "meminstrument/pass/DummyExternalChecksPass.h"
#include "meminstrument/pass/MemInstrumentPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/BreakCriticalEdges.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#endif

using namespace meminstrument;
using namespace llvm;

namespace meminstrument {
static RegisterPass<MemInstrumentPass>
    RegisterMemInstrumentPass("meminstrument", "MemInstrument",
                              false,  // CFGOnly
                              false); // isAnalysis

#if !MEMINSTRUMENT_USE_PMDA
static RegisterPass<DummyExternalChecksPass>
    RegisterDummyExternalChecksPass("mi-dummy-external-checks",
                                    "Dummy External Checks",
                                    false, // CFGOnly
                                    true); // isAnalysis
#endif

static void registerMeminstrumentPass(const llvm::PassManagerBuilder &,
                                      llvm::legacy::PassManagerBase &PM) {
#if MEMINSTRUMENT_USE_PMDA
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createCFGSimplificationPass());
  PM.add(createUnifyFunctionExitNodesPass());
  PM.add(createBreakCriticalEdgesPass());
  PM.add(new checkoptimizer::CheckOptimizerPass());
#endif
  PM.add(new MemInstrumentPass());
}

static llvm::RegisterStandardPasses RegisterMeminstrumentPass(
    llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, registerMeminstrumentPass);
    // llvm::PassManagerBuilder::EP_OptimizerLast, registerMeminstrumentPass);
} // namespace meminstrument
