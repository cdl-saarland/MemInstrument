//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetFilters.h"

#include "meminstrument/Config.h"
#include "meminstrument/Definitions.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#include <algorithm>
#include <math.h>

#include "meminstrument/pass/Util.h"

STATISTIC(NumITargetsNoSanitize, "The # of instrumentation targets discarded "
                                 "because of nosanitize annotations");

STATISTIC(NumITargetsSubsumed, "The # of instrumentation targets discarded "
                               "because of dominating subsumption");

using namespace meminstrument;
using namespace llvm;

namespace {
void filterByDominance(Pass *ParentPass, ITargetVector &Vec, Function &F) {
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

void filterByAnnotation(ITargetVector &Vec) {
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

cl::opt<double>
    RandomFilteringRatioOpt("mi-random-filter-ratio",
                            cl::desc("ratio of accesses that should not be "
                                     "instrumented, should be between 0 and 1"),
                            cl::init(-1.0) // default
    );

cl::opt<int>
    RandomFilteringSeedOpt("mi-random-filter-seed",
                           cl::desc("random seed for filtering, only relevant "
                                    "if mi-random-filter is present"),
                           cl::init(424242) // default
    );

void filterByRandom(ITargetVector &Vec, int seed, double filter_ratio) {
  ITargetVector cpy = Vec;
  std::srand(seed);

  std::random_shuffle(cpy.begin(), cpy.end());

  size_t bound = (size_t)(((double)cpy.size()) * filter_ratio);

  for (size_t i = 0; i < bound; ++i) {
    cpy[i]->invalidate();
  }
}

} // namespace

void meminstrument::filterITargets(Pass *P, ITargetVector &Vec, Function &F) {
  bool UseFilters = GlobalConfig::get(*F.getParent()).hasUseFilters();
  if (!UseFilters) {
    return;
  }

  if (RandomFilteringRatioOpt >= 0 && RandomFilteringRatioOpt <= 1) {
    filterByRandom(Vec, RandomFilteringSeedOpt, RandomFilteringRatioOpt);
  } else {
    filterByAnnotation(Vec);

    filterByDominance(P, Vec, F);
  }
}
