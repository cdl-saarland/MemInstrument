//===- meminstrument/AnnotationBasedRemovalPass.h ---------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//
#ifndef MEMINSTRUMENT_OPTIMIZATION_ANNOTATIONBASEDCHECKREMOVALPASS_H
#define MEMINSTRUMENT_OPTIMIZATION_ANNOTATIONBASEDCHECKREMOVALPASS_H

#include "meminstrument/optimizations/OptimizationInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class AnnotationBasedRemovalPass : public llvm::ModulePass,
                                   public OptimizationInterface {
public:
  // ModulePass methods

  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  AnnotationBasedRemovalPass();

  virtual bool runOnModule(llvm::Module &) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  // OptimizationInterface methods

  virtual void updateITargetsForFunction(MemInstrumentPass &, ITargetVector &,
                                         llvm::Function &) override;
};

} // namespace meminstrument

#endif