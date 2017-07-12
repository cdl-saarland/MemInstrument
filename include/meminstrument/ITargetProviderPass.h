//===- meminstrument/ITargetProviderPass.h -- MemSafety Instr. --*- C++ -*-===//
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

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

namespace meminstrument {

class ITargetProviderPass : public llvm::FunctionPass {
public:

  /// \brief Default constructor to initialize the function pass interface
  ITargetProviderPass(char& ID);

  virtual std::vector<ITarget> *getITargets(llvm::Function* F) = 0;

  virtual ~ITargetProviderPass(void) {}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGETPROVIDERPASS_H
