//===- meminstrument/SleepMechanism.h - Checks That Sleep -------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This mechanism does not implement a real safety instrumentation. It inserts
/// calls to functions in the sleep runtime which waste time by performing
/// volatile stores instead of checks, bounds loads and stores, and the like.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SLEEPMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SLEEPMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"

#include "llvm/IR/Module.h"

namespace meminstrument {

class GlobalConfig;

class SleepMechanism : public SplayMechanism {
public:
  SleepMechanism(GlobalConfig &cfg) : SplayMechanism(cfg) {}

  virtual void initialize(llvm::Module &) override;

  virtual const char *getName(void) const override { return "Sleep"; }
};

} // namespace meminstrument

#endif
