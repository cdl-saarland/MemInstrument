//===- meminstrument/DominanceBasedCheckRemovalPass.h -----------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Whenever the same memory location is accessed multiple times, and the first
/// access dominates the others, the subsequent spatial checks can be elided.
/// This optimization filters out targets for such dominated accesses.
///
/// Some instrumentations have invariants that perform in-bounds checks. This
/// optimization can filter dominated invariants upon request.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_DOMINANCEBASEDCHECKREMOVALPASS_H
#define MEMINSTRUMENT_OPTIMIZATION_DOMINANCEBASEDCHECKREMOVALPASS_H

#include "meminstrument/optimizations/OptimizationInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class DominanceBasedCheckRemovalPass : public llvm::ModulePass,
                                       public OptimizationInterface {
public:
  // ModulePass methods

  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  DominanceBasedCheckRemovalPass();

  virtual bool runOnModule(llvm::Module &) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  // OptimizationInterface methods

  virtual void updateITargetsForFunction(MemInstrumentPass &, ITargetVector &,
                                         llvm::Function &) override;

private:
  /// Check if the target \p one accesses the same or strictly more memory than
  /// \p other. Returns true if this is the case.
  bool subsumes(const ITarget &one, const ITarget &other) const;
};

} // namespace meminstrument

#endif
