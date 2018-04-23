//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_MEMINSTRUMENTPASS_H
#define MEMINSTRUMENT_PASS_MEMINSTRUMENTPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "meminstrument/Config.h"

namespace meminstrument {

class MemInstrumentPass : public llvm::ModulePass {
public:
  /// \brief Identification
  static char ID;

  /// \brief Default constructor to initialize the module pass interface
  MemInstrumentPass();

  virtual bool doInitialization(llvm::Module &) override { return false; }

  virtual bool runOnModule(llvm::Module &M) override;
  virtual void releaseMemory(void) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual void print(llvm::raw_ostream &O, const llvm::Module *) const override;

  GlobalConfig &getConfig(void);

private:
  std::unique_ptr<GlobalConfig> CFG;
};

} // namespace meminstrument

#endif
