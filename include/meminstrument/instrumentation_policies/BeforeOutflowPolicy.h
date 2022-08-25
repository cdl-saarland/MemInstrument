//===- meminstrument/BeforeOutflowPolicy.h - Check on Escape ----*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This policy creates check targets for loads, stores, and function pointer
/// calls. Additionally, it creates invariant targets for all locations where a
/// pointer "escapes"/"flows out"/"leaves a context". Such locations include
/// storing a pointer to memory, returning it from a function, passing it as an
/// argument to a function, and storing it into an aggregate.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_BEFOREOUTFLOWPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_BEFOREOUTFLOWPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Instruction.h"

namespace meminstrument {

class BeforeOutflowPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(ITargetVector &Dest,
                               llvm::Instruction *Loc) override;

  virtual const char *getName(void) const override { return "BeforeOutflow"; }
};

} // namespace meminstrument

#endif
