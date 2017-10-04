//===-- meminstrument/ITargetProviderPass.h -- MemSafety Instr. -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_ITARGETPROVIDERPASS_H
#define MEMINSTRUMENT_ITARGETPROVIDERPASS_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"

#include <memory>
#include <vector>

namespace meminstrument {

class ITargetProviderPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the function pass interface
  ITargetProviderPass();

  /// doInitialization - Virtual method overridden by subclasses to do
  /// any necessary initialization before any pass is run.
  virtual bool doInitialization(llvm::Module &) override;

  virtual bool runOnModule(llvm::Module &F) override;
  // virtual void releaseMemory() override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;

  std::vector<std::shared_ptr<ITarget>> &
  getITargetsForFunction(llvm::Function *F);

private:
  typedef llvm::ValueMap<llvm::Function *,
                         std::vector<std::shared_ptr<ITarget>>>
      MapType;
  MapType TargetMap;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGETPROVIDERPASS_H
