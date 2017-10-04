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

#include "meminstrument/Definitions.h"
#include "meminstrument/ITargetProviderPass.h"
#include "meminstrument/MemInstrumentSetupPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"
#include "meminstrument/WitnessStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FileSystem.h"

#include "meminstrument/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {
cl::opt<bool> PrintWitnessGraphOpt("mi-print-witnessgraph",
                                   cl::desc("Print the WitnessGraph"),
                                   cl::init(false) // default
                                   );
cl::opt<bool>
    NoSimplifyWitnessGraphOpt("mi-no-simplify-witnessgraph",
                              cl::desc("Disable witness graph simplifications"),
                              cl::init(false) // default
                              );
}

GenerateWitnessesPass::GenerateWitnessesPass() : ModulePass(ID) {}

bool GenerateWitnessesPass::doInitialization(llvm::Module &) { return false; }

bool GenerateWitnessesPass::runOnModule(Module &M) {
  auto *IPPass = cast<ITargetProviderPass>(&this->getAnalysis<ITargetProviderPass>());

  const auto &WS = WitnessStrategy::get();
  auto &IM = InstrumentationMechanism::get();
  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;

    DEBUG(dbgs() << "GenerateWitnessesPass: processing function `"
                 << F.getName().str() << "`\n";);

    std::vector<std::shared_ptr<ITarget>> &Destination =
        IPPass->getITargetsForFunction(&F);
    WitnessGraph WG(F, WS);

    for (auto &Target : Destination) {
      if (Target->isValid()) {
        WG.insertRequiredTarget(Target);
      }
    }

    WG.propagateFlags();

    if (PrintWitnessGraphOpt) {
      WG.dumpDotGraph(("wg." + F.getName() + ".dot").str());
    }

    if (!NoSimplifyWitnessGraphOpt) {
      WS.simplifyWitnessGraph(WG);

      if (PrintWitnessGraphOpt) {
        WG.dumpDotGraph(("wg.simplified." + F.getName() + ".dot").str());
      }
    }

    WG.createWitnesses(IM);

    DEBUG_ALSO_WITH_TYPE("meminstrument-genwitnesses",
                         dbgs() << "targets with the same witnesses:"
                                << "\n";
                         WG.printWitnessClasses(dbgs()););

    for (auto &T : Destination) {
      if (T->isValid() && T->RequiresExplicitBounds) {
        IM.materializeBounds(*T);
      }
    }
  }
  return true;
}

void GenerateWitnessesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MemInstrumentSetupPass>();
  AU.addRequired<ITargetProviderPass>();
  AU.addRequired<MemSafetyAnalysisPass>();
  AU.addPreserved<ITargetProviderPass>();
}

char GenerateWitnessesPass::ID = 0;
