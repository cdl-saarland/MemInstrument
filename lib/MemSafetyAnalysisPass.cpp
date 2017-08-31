//===----- MemSafetyAnalysisPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/MemSafetyAnalysisPass.h"

#include "meminstrument/GatherITargetsPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"

#include <algorithm>

#include "meminstrument/Util.h"

STATISTIC(NumITargetsNoSanitize, "The # of instrumentation targets discarded "
                                 "because of nosanitize annotations");

using namespace meminstrument;
using namespace llvm;

MemSafetyAnalysisPass::MemSafetyAnalysisPass() : ModulePass(ID) {}

bool MemSafetyAnalysisPass::doInitialization(llvm::Module &) { return false; }

bool MemSafetyAnalysisPass::runOnModule(Module &M) {
  auto *GITPass =
      cast<GatherITargetsPass>(&this->getAnalysis<GatherITargetsPass>());
  this->connectToProvider(GITPass);

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    auto &Vec = this->getITargetsForFunction(&F);

    Vec.erase(std::remove_if(Vec.begin(), Vec.end(),
                             [](std::shared_ptr<ITarget> &IT) {
                               auto *L = IT->Location;
                               auto *V = IT->Instrumentee;
                               bool res =
                                   L->getMetadata("nosanitize") &&
                                   ((isa<LoadInst>(L) && (V == L->getOperand(0))) ||
                                    (isa<StoreInst>(L) && (V == L->getOperand(1))));
                               if (res) {
                                 ++NumITargetsNoSanitize;
                               }
                               return res;
                             }),
              Vec.end());

    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-memsafetyanalysis",
        dbgs() << "remaining instrumentation targets after filter:"
               << "\n";
        for (auto &Target
             : Vec) { dbgs() << "  " << *Target << "\n"; });
  }
  return false;
}

void MemSafetyAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<GatherITargetsPass>();
  AU.setPreservesAll();
}

char MemSafetyAnalysisPass::ID = 0;
