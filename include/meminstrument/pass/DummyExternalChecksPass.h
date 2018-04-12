//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_DUMMYEXTERNALCHECKSPASS_H
#define MEMINSTRUMENT_PASS_DUMMYEXTERNALCHECKSPASS_H

#include "meminstrument/pass/ExternalChecksInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class DummyExternalChecksPass : public llvm::ModulePass,
                                public ExternalChecksInterface {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the module pass interface
  DummyExternalChecksPass();

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

  virtual void updateITargetsForFunction(GlobalConfig &CFG,
      ITargetVector &Vec,
                                         llvm::Function &F) override;

  virtual void materializeExternalChecksForFunction(GlobalConfig &CFG,
      ITargetVector &Vec,
                                                    llvm::Function &F) override;

private:
  std::map<llvm::Function *, ITargetVector> WorkList;
};

} // namespace meminstrument

#endif
