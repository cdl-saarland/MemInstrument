//===- meminstrument/AccessOnlyPolicy.h - Check only Accesses ---*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_ACCESSONLYPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_ACCESSONLYPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Instruction.h"

namespace meminstrument {

class AccessOnlyPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(ITargetVector &Dest,
                               llvm::Instruction *Loc) override;

  virtual const char *getName(void) const override { return "AccessOnly"; }
};

} // namespace meminstrument

#endif
