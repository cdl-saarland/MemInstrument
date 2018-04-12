//= meminstrument/instrumentation_policies/InstrumentationPolicy.h -*- C++ -*-//
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

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class GlobalConfig;

class InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<std::shared_ptr<ITarget>> &Dest,
                               llvm::Instruction *Loc) = 0;

  virtual const char *getName(void) const = 0;

  virtual ~InstrumentationPolicy(void) {}

protected:
  GlobalConfig &_CFG;

  InstrumentationPolicy(GlobalConfig &cfg) : _CFG(cfg) {}

  size_t getPointerAccessSize(const llvm::DataLayout &DL, llvm::Value *V);
};

} // namespace meminstrument

#endif
