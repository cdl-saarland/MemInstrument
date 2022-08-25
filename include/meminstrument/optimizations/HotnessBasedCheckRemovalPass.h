//===- meminstrument/HotnessBasedCheckRemovalPass.h -------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This pass can filter out check targets based on various criteria: at random
/// or the x percent hottest or coolest checks. For the filtering of the hottest
/// or coolest checks, the frequency data has to be collected in a separate run
/// by RuntimeStatMechanism first. The purpose of this filtering is to get a
/// sense of the runtime cost of individual checks for a benchmark. Oftentimes,
/// the actual execution frequency (or general impact) of a (static) check on
/// the execution time varies widely. Hence, this filter can give a handle to
/// grasp the impact of individual checks in a benchmark, but is not a valid
/// optimization.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_HOTNESSBASEDCHECKREMOVALPASS_H
#define MEMINSTRUMENT_OPTIMIZATION_HOTNESSBASEDCHECKREMOVALPASS_H

#include "meminstrument/optimizations/OptimizationInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class HotnessBasedCheckRemovalPass : public llvm::ModulePass,
                                     public OptimizationInterface {
public:
  // ModulePass methods

  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  HotnessBasedCheckRemovalPass();

  virtual bool runOnModule(llvm::Module &) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  // OptimizationInterface methods

  virtual void
  updateITargetsForModule(MemInstrumentPass &,
                          std::map<llvm::Function *, ITargetVector> &) override;

private:
  std::map<llvm::Instruction *, uint64_t> hotnessForAccesses;
};

} // namespace meminstrument

#endif