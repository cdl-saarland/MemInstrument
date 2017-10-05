//===-------- ITargetFilterPass.cpp -- MemSafety Instrumentation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITargetFilterPass.h"

#include "meminstrument/Definitions.h"
#include "meminstrument/ITargetProviderPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#if MEMINSTRUMENT_USE_PMDA
#include "CheckOptimizer/CheckOptimizerPass.h"
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

void DominanceFilter::filterForFunction(llvm::Function &F,
                                        ITargetVector &Vec) const {
  const auto &DomTree =
      ParentPass->getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

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

void AnnotationFilter::filterForFunction(llvm::Function &F,
                                         ITargetVector &Vec) const {
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
}

ITargetFilterPass::ITargetFilterPass() : ModulePass(ID) {}

bool ITargetFilterPass::doInitialization(llvm::Module &) { return false; }

void ITargetFilterPass::releaseMemory(void) {}

bool ITargetFilterPass::runOnModule(Module &M) {
  if (NoOptimizations) {
    return false;
  }

  std::vector<ITargetFilter *> Filters{
      new AnnotationFilter(this),
      new DominanceFilter(this),
  };

  auto *IPPass = GET_ITARGET_PROVIDER_PASS;

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    auto &Vec = IPPass->getITargetsForFunction(&F);

    for (auto *Filter : Filters) {
      Filter->filterForFunction(F, Vec);
    }

    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-itargetfilter",
        dbgs() << "remaining instrumentation targets after filter:"
               << "\n";
        for (auto &Target
             : Vec) { dbgs() << "  " << *Target << "\n"; });
  }

  for (auto *Filter : Filters) {
    delete Filter;
  }
  return false;
}

void ITargetFilterPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ITargetProviderPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesAll();
}

char ITargetFilterPass::ID = 0;
