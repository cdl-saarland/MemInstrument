//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetFilters.h"

#include "meminstrument/Config.h"
#include "meminstrument/Definitions.h"

#include "meminstrument/perf_data.h"

#include "llvm/ADT/Statistic.h"
// #include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#include <algorithm>
#include <string>
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
                           cl::desc("random seed for filtering"),
                           cl::init(424242) // default
    );

enum FilterOrdering {
  FO_random,
  FO_hottest,
  FO_coolest,
};

cl::opt<FilterOrdering>
    FilterOrderingOpt("mi-filter-ordering",
                      cl::desc("strategy for filtering arbitrary checks"),
                      cl::values(clEnumValN(FO_random, "random", "filter checks randomly")),
                      cl::values(clEnumValN(FO_hottest, "hottest", "filter hottest checks")),
                      cl::values(clEnumValN(FO_coolest, "coolest", "filter coolest checks")),
                      cl::init(FO_random) // default
    );


// void filterByRandom(ITargetVector &Vec, int seed, double filter_ratio) {
//   ITargetVector cpy = Vec;
//   std::srand(seed);
//
//   std::random_shuffle(cpy.begin(), cpy.end());
//
//   size_t bound = (size_t)(((double)cpy.size()) * filter_ratio);
//
//   for (size_t i = 0; i < bound; ++i) {
//     cpy[i]->invalidate();
//   }
// }

// void filterByLoopDepth(Pass *ParentPass, ITargetVector &Vec, Function &F, unsigned removeFromInnermost, unsigned removeFromOutermost) {
//   const auto &LI = ParentPass->getAnalysis<LoopInfo>(F);
//
//   // remove checks from the `removeFromOutermost` outermost loop levels
//   // and from the `removeFromInnermost` innermost loop levels
//   // if both values are 0, this is a no-op
//
//   unsigned MaxLoopDepth = 0;
//
//   for (auto &i : Vec) {
//     BasicBlock *BB = i->Location->getParent()
//     unsigned depth = LI.getLoopDepth(BB);
//     if (depth > MaxLoopDepth) {
//       MaxLoopDepth = depth;
//     }
//   }
//
//   for (auto &i : Vec) {
//     BasicBlock *BB = i->Location->getParent()
//     unsigned depth = LI.getLoopDepth(BB);
//     if (depth < removeFromOutermost) {
//       i->invalidate();
//     }
//   }
// }



} // namespace

void meminstrument::filterITargets(Pass *P, ITargetVector &Vec, Function &F) {
  bool UseFilters = GlobalConfig::get(*F.getParent()).hasUseFilters();
  if (!UseFilters) {
    return;
  }

  filterByAnnotation(Vec);

  filterByDominance(P, Vec, F);
}

uint64_t extractAccessId(Instruction *I) {
  if (auto *N = I->getMetadata("mi_access_id")) {
    assert(N->getNumOperands() == 1);

    assert(isa<MDString>(N->getOperand(0)));
    auto *Str = cast<MDString>(N->getOperand(0));
    return std::stoi(Str->getString());
  } else {
    assert(false && "Missing access id metadata!");
    return 0;
  }
}

void meminstrument::filterITargetsRandomly(Pass *ParentPass,
    std::map<llvm::Function *, ITargetVector> TargetMap) {
  if (!(RandomFilteringRatioOpt >= 0 && RandomFilteringRatioOpt <= 1)) {
    return;
  }
  ITargetVector cpy;
  for (const auto &p : TargetMap) {
    cpy.insert(cpy.end(), p.second.begin(), p.second.end());
  }
  std::srand(RandomFilteringSeedOpt);

  std::random_shuffle(cpy.begin(), cpy.end());

  if (FilterOrderingOpt != FO_random)
  std::stable_sort(cpy.begin(), cpy.end(), [&](const std::shared_ptr<ITarget> &a, const std::shared_ptr<ITarget> &b){
      Function *funA = a->Location->getParent()->getParent();
      Function *funB = b->Location->getParent()->getParent();
      Module *mod = funA->getParent();
      assert(mod == funB->getParent());

      const char* funAname = funA->getName().str().c_str();
      const char* funBname = funB->getName().str().c_str();
      const char* modname = mod->getName().str().c_str();

      uint64_t idxA = extractAccessId(a->Location);
      uint64_t idxB = extractAccessId(b->Location);

      unsigned heatA = getHotnessIndex(modname, funAname, idxA);
      unsigned heatB = getHotnessIndex(modname, funBname, idxB);
      return (FilterOrderingOpt == FO_hottest) ? (heatA > heatB) : (heatA > heatB);
      });

  size_t bound = (size_t)(((double)cpy.size()) * RandomFilteringRatioOpt);

  for (size_t i = 0; i < bound; ++i) {
    cpy[i]->invalidate();
  }
}
