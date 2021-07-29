//===- HotnessBasedCheckRemovalPass.cpp - Remove Targets by ExecFreq ------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/HotnessBasedCheckRemovalPass.h"

#include "meminstrument/optimizations/PerfData.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"

using namespace llvm;
using namespace meminstrument;

cl::opt<double>
    RandomFilteringRatioOpt("mi-random-filter-ratio",
                            cl::desc("ratio of accesses that should not be "
                                     "instrumented, should be between 0 and 1"),
                            cl::init(0.5) // default
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

STATISTIC(HotnessCheckRemoved, "The # checks filtered by hotness");

//===--------------------------- ModulePass -------------------------------===//

char HotnessBasedCheckRemovalPass::ID = 0;

HotnessBasedCheckRemovalPass::HotnessBasedCheckRemovalPass() : ModulePass(ID) {}

bool HotnessBasedCheckRemovalPass::runOnModule(Module &module) {
  LLVM_DEBUG(dbgs() << "Running Hotness Based Removal Pass\n";);

  // Nothing to prepare for random filtering
  if (FilterOrderingOpt == FO_random) {
    return false;
  }

  // Collect the annotated access ID and look up the hotness of the instruction
  for (auto &fun : module) {
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (isa<StoreInst>(&inst) || isa<LoadInst>(&inst)) {
          if (hasAccessID(&inst)) {
            auto id = getAccessID(&inst);
            hotnessForAccesses[&inst] =
                getHotnessIndex(module.getName(), fun.getName(), id);
          }
        }
      }
    }
  }

  return false;
}

void HotnessBasedCheckRemovalPass::getAnalysisUsage(
    AnalysisUsage &analysisUsage) const {
  analysisUsage.setPreservesAll();
}

void HotnessBasedCheckRemovalPass::print(raw_ostream &stream,
                                         const Module *module) const {
  stream << "Running Hotness Based Removal Pass on `" << module->getName()
         << "`\n\n";
  for (auto &entry : hotnessForAccesses) {
    stream << entry.second << "\t" << *entry.first << "\n";
  }
}

//===--------------------- OptimizationInterface --------------------------===//

void HotnessBasedCheckRemovalPass::updateITargetsForModule(
    MemInstrumentPass &mip, std::map<Function *, ITargetVector> &targetMap) {

  // Make sure a valid ratio is given
  assert(RandomFilteringRatioOpt >= 0 && RandomFilteringRatioOpt <= 1);

  ITargetVector cpy;
  for (const auto &entry : targetMap) {
    for (auto &target : entry.second) {

      // Don't filter already invalidated targets
      if (!target->isValid()) {
        continue;
      }

      // Make sure to only include targets for loads and stores, otherwise there
      // is no hotness index available.
      auto loc = target->getLocation();
      if (isa<LoadInst>(loc) || isa<StoreInst>(loc)) {
        // TODO for SoftBound this should only include check targets.
        cpy.push_back(target);
      }
    }
  }

  if (FilterOrderingOpt == FO_random) {
    std::srand(RandomFilteringSeedOpt);
    std::random_shuffle(cpy.begin(), cpy.end());
  } else {
    std::stable_sort(
        cpy.begin(), cpy.end(), [&](const ITargetPtr &a, const ITargetPtr &b) {
          auto heatA = hotnessForAccesses[a->getLocation()];
          auto heatB = hotnessForAccesses[b->getLocation()];
          return (FilterOrderingOpt == FO_hottest) ? (heatA > heatB)
                                                   : (heatA < heatB);
        });
  }

  size_t bound = (size_t)(((double)cpy.size()) * RandomFilteringRatioOpt);

  for (size_t i = 0; i < bound; ++i) {
    HotnessCheckRemoved++;
    cpy[i]->invalidate();
  }
}
