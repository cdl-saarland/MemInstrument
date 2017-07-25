//===------- GenerateChecksPass.cpp -- MemSafety Instrumentation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/GenerateChecksPass.h"

#include "meminstrument/FancyChecksPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

GenerateChecksPass::GenerateChecksPass() : ModulePass(ID) {}

bool GenerateChecksPass::doInitialization(llvm::Module &) {
  auto *FCPass = cast<FancyChecksPass>(&this->getAnalysis<FancyChecksPass>());
  this->connectToProvider(FCPass);
  return false;
}

bool GenerateChecksPass::runOnModule(Module &M) {

  for (auto &F : M) {
    if (F.empty())
      return false;

    DEBUG(dbgs() << "GenerateChecksPass: processing function `"
                 << F.getName().str() << "`\n";);
  }
  return true;
}

void GenerateChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<FancyChecksPass>();
}

char GenerateChecksPass::ID = 0;
