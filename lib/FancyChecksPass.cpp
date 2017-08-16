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

#include "meminstrument/GatherITargetsPass.h"
#include "meminstrument/GenerateWitnessesPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

FancyChecksPass::FancyChecksPass() : ModulePass(ID) {}

bool FancyChecksPass::doInitialization(llvm::Module &) { return false; }

bool FancyChecksPass::runOnModule(Module &M) {
  auto *GWPass =
      cast<GenerateWitnessesPass>(&this->getAnalysis<GenerateWitnessesPass>());
  this->connectToProvider(GWPass);

  for (auto &F : M) {
    if (F.empty())
      return false;

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
}

char FancyChecksPass::ID = 0;
