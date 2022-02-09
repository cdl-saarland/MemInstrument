//===- meminstrument/AfterInflowStrategy.h - After Inflow -------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Track pointers down to where they are first loaded or created in the
/// program.
/// "Inflow" locations include pointers loaded from memory, pointer arguments,
/// inttoptr casts, calls that return pointers, alloca instructions etc. Makes
/// sure that this information is properly propagated up to the point of the
/// check, e.g., through phis, selects, insert or extract value instructions.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_AFTERINFLOWSTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_AFTERINFLOWSTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/WitnessGraph.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "llvm/IR/Constant.h"
#include "llvm/IR/Instruction.h"

namespace meminstrument {

class AfterInflowStrategy : public WitnessStrategy {
public:
  AfterInflowStrategy(GlobalConfig &cfg) : WitnessStrategy(cfg) {}

  virtual void addRequired(WitnessGraphNode *Node) const override;

  virtual void simplifyWitnessGraph(WitnessGraph &WG) const override;

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const override;

  virtual const char *getName(void) const override { return "AfterInflow"; }

private:
  void getPointerOperands(std::vector<llvm::Value *> &Results,
                          llvm::Constant *C) const;
};

} // namespace meminstrument

#endif
