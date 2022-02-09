//===- meminstrument/NonePolicy.h - Don't generate any target ---*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Policy that does not generate any instrumentation target.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_NONEPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_NONEPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"

namespace meminstrument {

class NonePolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(ITargetVector &Dest,
                               llvm::Instruction *Loc) override;

  virtual const char *getName(void) const override { return "None"; }
};

} // namespace meminstrument

#endif
