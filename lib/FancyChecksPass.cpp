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
#include "meminstrument/ITargetProviderPass.h"
#include "meminstrument/GenerateWitnessesPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"

#include "meminstrument/Util.h"

using namespace meminstrument;
using namespace llvm;

FancyChecksPass::FancyChecksPass() : ModulePass(ID) {}

bool FancyChecksPass::doInitialization(llvm::Module &) { return false; }

bool FancyChecksPass::runOnModule(Module &M) {
  auto *IPPass = cast<ITargetProviderPass>(&this->getAnalysis<ITargetProviderPass>());

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;

    DEBUG(dbgs() << "FancyChecksPass: processing function `"
                 << F.getName().str() << "`\n";);
  }
  return true;
}

void FancyChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ITargetProviderPass>();
  AU.addRequired<GenerateWitnessesPass>();
  AU.addPreserved<ITargetProviderPass>();
}

char FancyChecksPass::ID = 0;
