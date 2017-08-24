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
#include "meminstrument/GenerateWitnessesPass.h"
#include "meminstrument/InstrumentationMechanism.h"

#include "llvm/ADT/Statistic.h"

#include "meminstrument/Util.h"

using namespace meminstrument;
using namespace llvm;

GenerateChecksPass::GenerateChecksPass() : ModulePass(ID) {}

bool GenerateChecksPass::doInitialization(llvm::Module &) { return false; }

bool GenerateChecksPass::runOnModule(Module &M) {
  auto *FCPass = cast<FancyChecksPass>(&this->getAnalysis<FancyChecksPass>());
  this->connectToProvider(FCPass);
  // auto *GWPass =
  //     cast<GenerateWitnessesPass>(&this->getAnalysis<GenerateWitnessesPass>());
  // this->connectToProvider(GWPass);

  auto &IM = InstrumentationMechanism::get();

  for (auto &F : M) {
    if (F.empty())
      continue;

    DEBUG(dbgs() << "GenerateChecksPass: processing function `"
                 << F.getName().str() << "`\n";);

    std::vector<std::shared_ptr<ITarget>> &Destination =
        this->getITargetsForFunction(&F);

    for (auto &T : Destination) {
      IM.insertCheck(*T);
    }
  }
  return true;
}

void GenerateChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  // AU.addRequiredTransitive<GenerateWitnessesPass>();
  AU.addRequiredTransitive<FancyChecksPass>();
}

char GenerateChecksPass::ID = 0;
