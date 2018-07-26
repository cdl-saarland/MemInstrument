//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_LIFETIMEKILLER_LIFETIMEKILLERPASS_H
#define MEMINSTRUMENT_LIFETIMEKILLER_LIFETIMEKILLERPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class LifeTimeKillerPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the module pass interface
  LifeTimeKillerPass();

  virtual bool doInitialization(llvm::Module &) override { return false; }

  virtual bool runOnModule(llvm::Module &M) override;
  virtual void releaseMemory(void) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual void print(llvm::raw_ostream &O, const llvm::Module *) const override;
};

} // namespace meminstrument

#endif
