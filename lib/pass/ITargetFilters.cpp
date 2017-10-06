//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetFilters.h"

#include "meminstrument/Definitions.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#include <algorithm>

#include "meminstrument/pass/Util.h"

STATISTIC(NumITargetsNoSanitize, "The # of instrumentation targets discarded "
                                 "because of nosanitize annotations");

STATISTIC(NumITargetsSubsumed, "The # of instrumentation targets discarded "
                               "because of dominating subsumption");

using namespace meminstrument;
using namespace llvm;

namespace {
cl::opt<bool> NoFiltersOpt(
    "mi-no-filter",
    cl::desc("Disable all memsafety instrumentation target filters"),
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

void meminstrument::filterITargets(Pass *P, ITargetVector &Vec, Function &F) {
  if (NoFiltersOpt) {
    return;
  }

  std::vector<ITargetFilter *> Filters{
      new AnnotationFilter(P),
      new DominanceFilter(P),
  };

  for (auto *Filter : Filters) {
    Filter->filterForFunction(F, Vec);
  }

  DEBUG_ALSO_WITH_TYPE(
      "meminstrument-itargetfilter",
      dbgs() << "remaining instrumentation targets after filter:"
             << "\n";
      for (auto &Target
           : Vec) { dbgs() << "  " << *Target << "\n"; });

  for (auto *Filter : Filters) {
    delete Filter;
  }
}
