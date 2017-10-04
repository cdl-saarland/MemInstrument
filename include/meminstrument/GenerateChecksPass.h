//===-- meminstrument/GenerateChecksPass.h -- MemSafety Instr. --*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_GENERATECHECKSPASS_H
#define MEMINSTRUMENT_GENERATECHECKSPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class GenerateChecksPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the function pass interface
  GenerateChecksPass();

  /// doInitialization - Virtual method overridden by subclasses to do
  /// any necessary initialization before any pass is run.
  ///
  virtual bool doInitialization(llvm::Module &M) override;

  virtual bool runOnModule(llvm::Module &M) override;
  // virtual void releaseMemory() override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_GENERATECHECKSPASS_H
