//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_AFTERINFLOWSTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_AFTERINFLOWSTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/WitnessGraph.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class AfterInflowStrategy : public WitnessStrategy {
public:
  AfterInflowStrategy(GlobalConfig &cfg) : WitnessStrategy(cfg) {}

  virtual void addRequired(WitnessGraphNode *Node) const override;

  virtual void simplifyWitnessGraph(WitnessGraph &WG) const override;

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const override;

  virtual const char *getName(void) const override { return "AfterInflow"; }
};

} // namespace meminstrument

#endif
