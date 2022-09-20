//===- meminstrument/ExampleExternalChecksPass.h - Example Pass -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This pass demonstrates the usage of the optimization interface for an
/// advanced optimization that not only filters out targets, but also replaces
/// targets by others. Optimizations can, for example, query bounds from a
/// memory safety instrumentation and use these bounds to place checks at
/// different locations in the program. For this purpose, the optimization can
/// insert additional bounds targets, and drop check targets for memory accesses
/// that it guards with its own checks.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_EXAMPLEEXTERNALCHECKSPASS_H
#define MEMINSTRUMENT_OPTIMIZATION_EXAMPLEEXTERNALCHECKSPASS_H

#include "meminstrument/optimizations/OptimizationInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class ExampleExternalChecksPass : public llvm::ModulePass,
                                  public OptimizationInterface {
public:
  // ModulePass methods

  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  ExampleExternalChecksPass();

  virtual bool runOnModule(llvm::Module &) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  virtual bool doFinalization(llvm::Module &) override;

  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  // OptimizationInterface methods

  virtual void updateITargetsForFunction(MemInstrumentPass &, ITargetVector &,
                                         llvm::Function &) override;

  virtual void materializeExternalChecksForFunction(MemInstrumentPass &,
                                                    ITargetVector &,
                                                    llvm::Function &) override;

private:
  std::map<llvm::Function *, ITargetVector> WorkList;
};

} // namespace meminstrument

#endif
