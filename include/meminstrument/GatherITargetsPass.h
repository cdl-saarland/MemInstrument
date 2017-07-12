//===--- meminstrument/GatherITargetsPass.h -- MemSafety Instr. -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_GATHERITARGETSPASS_H
#define MEMINSTRUMENT_GATHERITARGETSPASS_H

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace meminstrument {

class GatherITargetsPass : public llvm::FunctionPass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the function pass interface
  GatherITargetsPass();

  /// doInitialization - Virtual method overridden by subclasses to do
  /// any necessary initialization before any pass is run.
  ///
  virtual bool doInitialization(llvm::Module &) override { return false; }

  /// \name Function pass interface
  //@{
  virtual bool runOnFunction(llvm::Function &F) override;
  // virtual void releaseMemory() override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;
  //@}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_GATHERITARGETSPASS_H
