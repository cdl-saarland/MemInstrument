//===- meminstrument/HotnessBasedCheckRemovalPass.h -------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
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