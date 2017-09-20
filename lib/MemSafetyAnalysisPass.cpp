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

#include "meminstrument/Definitions.h"
#include "meminstrument/GatherITargetsPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#if MEMINSTRUMENT_USE_PMDA
#include "PMDA/PMDA.h"
#endif

#include <algorithm>

#include "meminstrument/Util.h"

STATISTIC(NumITargetsNoSanitize, "The # of instrumentation targets discarded "
                                 "because of nosanitize annotations");

STATISTIC(NumITargetsSubsumed, "The # of instrumentation targets discarded "
                               "because of dominating subsumption");

using namespace meminstrument;
using namespace llvm;

namespace {
cl::opt<bool> NoOptimizations(
    "mi-no-optimize",
    cl::desc("Disable all memsafety instrumentation optimizations"),
    cl::init(false));
}

MemSafetyAnalysisPass::MemSafetyAnalysisPass() : ModulePass(ID) {}

bool MemSafetyAnalysisPass::doInitialization(llvm::Module &) { return false; }

void filterByDominance(const DominatorTree &DomTree,
                       std::vector<std::shared_ptr<ITarget>> &Vec) {
  for (auto &i1 : Vec) {
    for (auto &i2 : Vec) {
      if (i1 == i2 || !i1->isValid() || !i2->isValid())
        continue;

      if (i1->subsumes(*i2) && DomTree.dominates(i1->Location, i2->Location)) {
        i2->invalidate();
        ++NumITargetsSubsumed;
      }
    }
  }
}

bool MemSafetyAnalysisPass::runOnModule(Module &M) {
  auto *GITPass =
      cast<GatherITargetsPass>(&this->getAnalysis<GatherITargetsPass>());
  this->connectToProvider(GITPass);

  if (NoOptimizations) {
    return false;
  }

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    auto &Vec = this->getITargetsForFunction(&F);

    for (auto &IT : Vec) {
      auto *L = IT->Location;
      auto *V = IT->Instrumentee;
      bool res = L->getMetadata("nosanitize") &&
                 ((isa<LoadInst>(L) && (V == L->getOperand(0))) ||
                  (isa<StoreInst>(L) && (V == L->getOperand(1))));
      if (res) {
        ++NumITargetsNoSanitize;
        IT->invalidate();
      }
    }

    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-memsafetyanalysis",
        dbgs() << "remaining instrumentation targets after filter:"
               << "\n";
        for (auto &Target
             : Vec) { dbgs() << "  " << *Target << "\n"; });

    const auto &DomTree = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
    filterByDominance(DomTree, Vec);
  }

  return false;
}

void MemSafetyAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const {
#if MEMINSTRUMENT_USE_PMDA
  AU.addRequiredTransitive<pmda::PMDA>();
#endif
  AU.addRequiredTransitive<GatherITargetsPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesAll();
}

char MemSafetyAnalysisPass::ID = 0;
