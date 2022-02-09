//===- meminstrument/NoneStrategy.h - None Strategy -------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This strategy does not not actually track pointers back to an origin and
/// builds a witness graph, but simply requires witnesses for all targets from
/// the mechanism.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_NONESTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_NONESTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/WitnessGraph.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

namespace meminstrument {

class NoneStrategy : public WitnessStrategy {
public:
  virtual void addRequired(WitnessGraphNode *) const override;

  virtual void createWitness(InstrumentationMechanism &,
                             WitnessGraphNode *) const override;

  virtual const char *getName(void) const override { return "None"; }
};

} // namespace meminstrument

#endif
