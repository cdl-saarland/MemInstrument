//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_EXTERNALCHECKSPASS_H
#define MEMINSTRUMENT_PASS_EXTERNALCHECKSPASS_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class ExternalChecksPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the module pass interface
  ExternalChecksPass();

  virtual bool doInitialization(llvm::Module &) override { return false; }

  virtual bool doFinalization(llvm::Module &) override {
    WorkList.clear();
    return false;
  }

  virtual bool runOnModule(llvm::Module &M) override;
  // virtual void releaseMemory() override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  // virtual void print(llvm::raw_ostream &O, const llvm::Module *) const
  // override;

  void updateITargetsForFunction(ITargetVector &Vec, llvm::Function &F);

  void materializeExternalChecksForFunction(ITargetVector &Vec, llvm::Function &F);

private:
  std::map<llvm::Function*, ITargetVector> WorkList;
};

}

#endif
