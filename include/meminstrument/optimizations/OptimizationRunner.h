//===- meminstrument/OptimizationRunner.h - Optimization Runner -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Handling of optimizations for the memory safety instrumentations.
///
/// Defines comand line flags for availables optimizations and contains the
/// logic on how to apply these optimizations.
///
/// New optimizations can be registered here.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_OPTIMIZATIONRUNNER_H
#define MEMINSTRUMENT_OPTIMIZATION_OPTIMIZATIONRUNNER_H

#include "meminstrument/optimizations/OptimizationInterface.h"

namespace meminstrument {

/// Currently available optimizations
///
/// Suffixes
/// * `checkrem`: Optimization filters out checks, but does not place new ones
/// * `checkopt`: Optimization filters out checks and places new ones
///
/// Future optimizations could use `metarem` or the like if they optimize the
/// metadata propagation.
enum InstrumentationOptimizations {
  annotation_checkrem,
  dominance_checkrem,
  hotness_checkrem,
  dummy_checkopt,
  pico_checkopt
};

class OptimizationRunner {

public:
  OptimizationRunner(MemInstrumentPass &);

  /// Run the preparation phase of each optimization
  void runPreparation(llvm::Module &) const;

  /// Use the update of ITargets that the optimizations define
  void updateITargets(std::map<llvm::Function *, ITargetVector> &) const;

  /// Let the optimizations place checks
  void placeChecks(ITargetVector &, llvm::Function &) const;

  /// Add analyses that the optimizations we run need in order to work properly
  static void addRequiredAnalyses(llvm::AnalysisUsage &);

  ~OptimizationRunner();

  OptimizationRunner() = delete;
  OptimizationRunner(const OptimizationRunner &) = delete;
  OptimizationRunner &operator=(const OptimizationRunner &) = delete;

private:
  MemInstrumentPass &mip;

  llvm::SmallVector<OptimizationInterface *, 2> selectedOpts;

  /// Create a vector of the optimizations that are selected
  static auto computeSelectedOpts(MemInstrumentPass &)
      -> llvm::SmallVector<OptimizationInterface *, 2>;

  void validate(const std::map<llvm::Function *, ITargetVector> &) const;
};

} // namespace meminstrument

#endif
