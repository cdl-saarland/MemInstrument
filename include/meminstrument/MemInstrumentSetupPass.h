//===- meminstrument/MemInstrumentSetupPass.h - MemSafety Instr -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_MEMINSTRUMENTSETUPPASS_H
#define MEMINSTRUMENT_MEMINSTRUMENTSETUPPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class MemInstrumentSetupPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the module pass interface
  MemInstrumentSetupPass();

  /// doInitialization - Virtual method overridden by subclasses to do
  /// any necessary initialization before any pass is run.
  ///
  virtual bool doInitialization(llvm::Module &) override { return false; }

  /// \name Function pass interface
  //@{
  virtual bool runOnModule(llvm::Module &M) override;
  // virtual void releaseMemory() override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;
  //@}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_MEMINSTRUMENTSETUPPASS_H