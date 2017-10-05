//===--- RegisterPasses.cpp - Add the required passes to default passes  --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Definitions.h"
#include "meminstrument/FancyChecksPass.h"
#include "meminstrument/GenerateChecksPass.h"
#include "meminstrument/GenerateWitnessesPass.h"
#include "meminstrument/ITargetFilterPass.h"
#include "meminstrument/ITargetProviderPass.h"
#include "meminstrument/MemInstrumentSetupPass.h"

#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
#endif

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace meminstrument;
using namespace llvm;

namespace meminstrument {
static RegisterPass<GenerateChecksPass>
    RegisterGenerateChecksPass("mi-genchecks", "GenerateChecks",
                               false,  // CFGOnly
                               false); // isAnalysis

static RegisterPass<FancyChecksPass>
    RegisterFancyChecksPass("mi-fancychecks", "FancyChecks",
                            false,  // CFGOnly
                            false); // isAnalysis

static RegisterPass<GenerateWitnessesPass>
    RegisterGenerateWitnessesPass("mi-genwitnesses", "GenerateWitnesses",
                                  false,  // CFGOnly
                                  false); // isAnalysis

static RegisterPass<MemInstrumentSetupPass>
    RegisterMemInstrumentSetupPass("mi-instrumentsetup", "MemInstrumentSetup",
                                   false,  // CFGOnly
                                   false); // isAnalysis

static RegisterPass<ITargetFilterPass>
    RegisterMemSafetyAnalysisPass("mi-itargetfilter", "ITargetFilter",
                                  false, // CFGOnly
                                  true); // isAnalysis

static RegisterPass<ITargetProviderPass>
    RegisterITargetProviderPass("mi-itargetprovider", "ITargetProvider",
                                false, // CFGOnly
                                true); // isAnalysis

static void registerMeminstrumentPass(const llvm::PassManagerBuilder &,
                                      llvm::legacy::PassManagerBase &PM) {
#if MEMINSTRUMENT_USE_PMDA
  PM.add(new checkoptimizer::CheckOptimizerPass());
#endif
  PM.add(new GenerateChecksPass());
}

static llvm::RegisterStandardPasses
    RegisterMeminstrumentPass(llvm::PassManagerBuilder::EP_OptimizerLast,
                              registerMeminstrumentPass);
} // namespace meminstrument
