//===--------- FancyChecksPass.cpp -- MemSafety Instrumentation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/FancyChecksPass.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/GatherITargetsPass.h"
#include "meminstrument/GenerateWitnessesPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"

#if MEMINSTRUMENT_USE_PMDA
#include "PMDA/PMDA.h"
#endif

#include "meminstrument/Util.h"

using namespace meminstrument;
using namespace llvm;

FancyChecksPass::FancyChecksPass() : ModulePass(ID) {}

bool FancyChecksPass::doInitialization(llvm::Module &) { return false; }

bool FancyChecksPass::runOnModule(Module &M) {
  auto *GWPass =
      cast<GenerateWitnessesPass>(&this->getAnalysis<GenerateWitnessesPass>());
  this->connectToProvider(GWPass);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;

    DEBUG(dbgs() << "FancyChecksPass: processing function `"
                 << F.getName().str() << "`\n";);
  }
  return true;
}

void FancyChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<GenerateWitnessesPass>();
  AU.addPreserved<GatherITargetsPass>();
  AU.addPreserved<MemSafetyAnalysisPass>();
  AU.addPreserved<GenerateWitnessesPass>();
#if MEMINSTRUMENT_USE_PMDA
  AU.addPreserved<pmda::PMDA>();
#endif
}

char FancyChecksPass::ID = 0;
