//===- meminstrument/MemInstrumentPass.h - Main Pass ------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Main pass that instruments the program.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_MEMINSTRUMENTPASS_H
#define MEMINSTRUMENT_PASS_MEMINSTRUMENTPASS_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "meminstrument/Config.h"

namespace llvm {
ModulePass *createMemInstrumentPass();
} // namespace llvm

namespace meminstrument {

class MemInstrumentPass : public llvm::ModulePass {
public:
  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  MemInstrumentPass();

  virtual bool runOnModule(llvm::Module &) override;
  virtual void releaseMemory(void) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;
  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  GlobalConfig &getConfig(void);

private:
  std::unique_ptr<GlobalConfig> CFG;
};

} // namespace meminstrument

#endif
