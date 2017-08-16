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

#include "meminstrument/GatherITargetsPass.h"
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

  const auto &WS = WitnessStrategy::get();
  auto &IM = InstrumentationMechanism::get();
  for (auto &F : M) {
    if (F.empty())
      continue;

    DEBUG(dbgs() << "GenerateWitnessesPass: processing function `"
                 << F.getName().str() << "`\n";);

    std::vector<std::shared_ptr<ITarget>> &Destination =
        this->getITargetsForFunction(&F);
    WitnessGraph WG(F, WS);

    for (auto &Target : Destination) {
      WG.insertRequiredTarget(Target);
    }

    // DEBUG(WG.printDotGraph(dbgs()););
    WG.printDotGraph(dbgs());

    WG.createWitnesses(IM);

    for (auto &T : Destination) {
      if (T->RequiresExplicitBounds) {
        IM.materializeBounds(*T);
      }
    }
  }
  return true;
}

void GenerateWitnessesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MemInstrumentSetupPass>();
  AU.addRequiredTransitive<MemSafetyAnalysisPass>();
  AU.addPreserved<GatherITargetsPass>();
  AU.addPreserved<MemSafetyAnalysisPass>();
}

char GenerateWitnessesPass::ID = 0;
