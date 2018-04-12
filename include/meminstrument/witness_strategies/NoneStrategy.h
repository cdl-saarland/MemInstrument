//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_NONESTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_NONESTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/WitnessGraph.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class NoneStrategy : public WitnessStrategy {
public:
  NoneStrategy(GlobalConfig &cfg) : WitnessStrategy(cfg) {}

  virtual void addRequired(WitnessGraphNode *Node) const override;

  virtual void simplifyWitnessGraph(WitnessGraph &WG) const override;

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const override;

  virtual const char *getName(void) const override { return "None"; }
};

} // namespace meminstrument

#endif
