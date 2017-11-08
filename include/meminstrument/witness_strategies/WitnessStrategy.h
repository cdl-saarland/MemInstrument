//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_WITNESSSTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_WITNESSSTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/WitnessGraph.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

struct WitnessGraphNode;

class WitnessGraph;

class WitnessStrategy {
public:
  virtual ~WitnessStrategy(void) {}

  virtual void addRequired(WitnessGraphNode *Node) const = 0;

  virtual void simplifyWitnessGraph(WitnessGraph &WG) const {}

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const = 0;

protected:
  void requireRecursively(WitnessGraphNode *Node, llvm::Value *Req,
                          llvm::Instruction *Loc) const;
  void requireSource(WitnessGraphNode *Node, llvm::Value *Req,
                     llvm::Instruction *Loc) const;

  static WitnessGraphNode *getInternalNode(WitnessGraph &WG,
                                           llvm::Value *Instrumentee,
                                           llvm::Instruction *Location);
};
} // namespace meminstrument

#endif
