//===----- meminstrument/ITargetFilterPass.h - MemSafety Instr. -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_ITARGETFILTERPASS_H
#define MEMINSTRUMENT_ITARGETFILTERPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "meminstrument/ITargetProviderPass.h"

namespace meminstrument {

class ITargetFilter;

class ITargetFilterPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the function pass interface
  ITargetFilterPass();

  /// doInitialization - Virtual method overridden by subclasses to do
  /// any necessary initialization before any pass is run.
  ///
  virtual bool doInitialization(llvm::Module &M) override;

  virtual bool runOnModule(llvm::Module &M) override;
  virtual void releaseMemory(void) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;
};

class ITargetFilter {
public:
  ITargetFilter(ITargetFilterPass *P) : ParentPass(P) {}

  virtual ~ITargetFilter(void) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const = 0;

protected:
  ITargetFilterPass *ParentPass;
};

class DominanceFilter : public ITargetFilter {
public:
  DominanceFilter(ITargetFilterPass *P) : ITargetFilter(P) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const override;
};

class AnnotationFilter : public ITargetFilter {
public:
  AnnotationFilter(ITargetFilterPass *P) : ITargetFilter(P) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const override;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGETFILTERPASS_H
