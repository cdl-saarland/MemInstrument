//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITargetFilters.h"

#include "meminstrument/Config.h"
#include "meminstrument/Definitions.h"

#include "meminstrument/pass/PerfData.h"

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

bool subsumes(const ITarget &one, const ITarget &other) {
  assert(one.isValid());
  assert(other.isValid());

  if (!one.hasInstrumentee() || !other.hasInstrumentee())
    return false;

  if (one.getInstrumentee() != other.getInstrumentee())
    return false;

  if (auto constSizeTarget = dyn_cast<ConstSizeCheckIT>(&one)) {
    if (auto constSizeTargetOther = dyn_cast<ConstSizeCheckIT>(&other)) {
      return constSizeTarget->getAccessSize() >=
             constSizeTargetOther->getAccessSize();
    }
  }

  if (auto constSizeTarget = dyn_cast<VarSizeCheckIT>(&one)) {
    if (auto constSizeTargetOther = dyn_cast<VarSizeCheckIT>(&other)) {
      return constSizeTarget->getAccessSizeVal() ==
             constSizeTargetOther->getAccessSizeVal();
    }
  }

  if (other.isInvariant()) {
    return one.isInvariant() || isa<ConstSizeCheckIT>(&one) ||
           isa<VarSizeCheckIT>(&one);
  }

  return false;
}

void filterByDominance(Pass *ParentPass, ITargetVector &Vec, Function &F) {
  const auto &DomTree =
      ParentPass->getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

  for (auto &i1 : Vec) {
    for (auto &i2 : Vec) {
      if (i1 == i2 || !i1->isValid() || !i2->isValid())
        continue;

      if (subsumes(*i1, *i2) &&
          DomTree.dominates(i1->getLocation(), i2->getLocation())) {
        i2->invalidate();
        ++NumITargetsSubsumed;
      }
    }
  }
}

void filterByAnnotation(ITargetVector &Vec) {
  for (auto &IT : Vec) {
    auto *L = IT->getLocation();
    if (L->getMetadata("nosanitize") && (IT->isCheck())) {
      ++NumITargetsNoSanitize;
      IT->invalidate();
    }
  }
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

cl::opt<double>
    RandomFilteringRatioOpt("mi-random-filter-ratio",
                            cl::desc("ratio of accesses that should not be "
                                     "instrumented, should be between 0 and 1"),
                            cl::init(-1.0) // default
    );

cl::opt<int> RandomFilteringSeedOpt("mi-random-filter-seed",
                                    cl::desc("random seed for filtering"),
                                    cl::init(424242) // default
);

enum FilterOrdering {
  FO_random,
  FO_hottest,
  FO_coolest,
};

cl::opt<FilterOrdering> FilterOrderingOpt(
    "mi-filter-ordering", cl::desc("strategy for filtering arbitrary checks"),
    cl::values(clEnumValN(FO_random, "random", "filter checks randomly"),
               clEnumValN(FO_hottest, "hottest", "filter hottest checks"),
               clEnumValN(FO_coolest, "coolest", "filter coolest checks")),
    cl::init(FO_random) // default
);

} // namespace

void meminstrument::filterITargets(GlobalConfig &CFG, Pass *P,
                                   ITargetVector &Vec, Function &F) {
  bool UseFilters = CFG.hasUseFilters();
  if (!UseFilters) {
    return;
  }

  filterByAnnotation(Vec);

  filterByDominance(P, Vec, F);
}

void meminstrument::filterITargetsRandomly(
    GlobalConfig &, std::map<Function *, ITargetVector> TargetMap) {
  if (!(RandomFilteringRatioOpt >= 0 && RandomFilteringRatioOpt <= 1)) {
    return;
  }
  ITargetVector cpy;
  for (const auto &p : TargetMap) {
    cpy.insert(cpy.end(), p.second.begin(), p.second.end());
  }
  std::srand(RandomFilteringSeedOpt);

  std::random_shuffle(cpy.begin(), cpy.end());

  if (FilterOrderingOpt != FO_random) {
    std::stable_sort(
        cpy.begin(), cpy.end(),
        [&](const std::shared_ptr<ITarget> &a,
            const std::shared_ptr<ITarget> &b) {
          Function *funA = a->getLocation()->getParent()->getParent();
          Function *funB = b->getLocation()->getParent()->getParent();
          Module *mod = funA->getParent();
          assert(mod == funB->getParent());

          auto funAname = funA->getName().str();
          auto funBname = funB->getName().str();
          auto modname = mod->getName().str();

          uint64_t idxA = extractAccessId(a->getLocation());
          uint64_t idxB = extractAccessId(b->getLocation());

          unsigned heatA = getHotnessIndex(modname, funAname, idxA);
          unsigned heatB = getHotnessIndex(modname, funBname, idxB);
          return (FilterOrderingOpt == FO_hottest) ? (heatA > heatB)
                                                   : (heatA < heatB);
        });
  }

  size_t bound = (size_t)(((double)cpy.size()) * RandomFilteringRatioOpt);

  for (size_t i = 0; i < bound; ++i) {
    cpy[i]->invalidate();
  }
}
