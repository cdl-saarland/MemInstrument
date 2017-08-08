//===----- GenerateWitnessesPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/GenerateWitnessesPass.h"

#include "meminstrument/MemInstrumentSetupPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"
#include "meminstrument/WitnessStrategy.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

GenerateWitnessesPass::GenerateWitnessesPass() : ModulePass(ID) {}

bool GenerateWitnessesPass::doInitialization(llvm::Module &) { return false; }

bool GenerateWitnessesPass::runOnModule(Module &M) {
  auto *MSAPass =
      cast<MemSafetyAnalysisPass>(&this->getAnalysis<MemSafetyAnalysisPass>());
  this->connectToProvider(MSAPass);

  // auto WS = std::unique_ptr<WitnessStrategy>(new WitnessStrategy()); // TODO

  for (auto &F : M) {
    if (F.empty())
      return false;

    DEBUG(dbgs() << "GenerateWitnessesPass: processing function `"
                 << F.getName().str() << "`\n";);

    std::vector<std::shared_ptr<ITarget>> &Destination =
        this->getITargetsForFunction(&F);
    WitnessGraph WG(F);
    TodoBetterNameStrategy WS;

    for (auto &Target : Destination) {
      auto *Node = WS.constructWitnessGraph(WG, Target);
      Node->Required = true;
    }

    WG.printDotGraph(dbgs());
    // TODO
  }
  return true;
}

void GenerateWitnessesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<MemSafetyAnalysisPass>();
  // AU.addRequiredTransitive<MemInstrumentSetupPass>();
}

char GenerateWitnessesPass::ID = 0;
