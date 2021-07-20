//===- meminstrument/DummyExternalChecksPass.h - Dummy Checks ---*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_DUMMYEXTERNALCHECKSPASS_H
#define MEMINSTRUMENT_PASS_DUMMYEXTERNALCHECKSPASS_H

#include "meminstrument/optimizations/ExternalChecksInterface.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class DummyExternalChecksPass : public llvm::ModulePass,
                                public ExternalChecksInterface {
public:
  // ModulePass methods

  /// Identification
  static char ID;

  /// Default constructor to initialize the module pass interface
  DummyExternalChecksPass();

  virtual bool runOnModule(llvm::Module &) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  virtual bool doFinalization(llvm::Module &) override;

  virtual void print(llvm::raw_ostream &, const llvm::Module *) const override;

  // ExternalChecksInterface methods

  virtual void updateITargetsForFunction(MemInstrumentPass &, ITargetVector &,
                                         llvm::Function &) override;

  virtual void materializeExternalChecksForFunction(MemInstrumentPass &,
                                                    ITargetVector &,
                                                    llvm::Function &) override;

private:
  std::map<llvm::Function *, ITargetVector> WorkList;
};

} // namespace meminstrument

#endif
