//===---------------------------------------------------------------------===///
///
/// \file See corresponding header for more descriptions.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "meminstrument/witness_strategies/AfterInflowStrategy.h"
#include "meminstrument/witness_strategies/NoneStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

WitnessGraphNode *WitnessStrategy::getInternalNode(WitnessGraph &WG,
                                                   Value *Instrumentee,
                                                   Instruction *Location) {
  auto NewTarget =
      ITargetBuilder::createIntermediateTarget(Instrumentee, Location);
  return WG.getInternalNode(NewTarget);
}

void WitnessStrategy::requireRecursively(WitnessGraphNode *Node, Value *Req,
                                         Instruction *Loc) const {
  auto &WG = Node->Graph;

  auto *NewNode = getInternalNode(WG, Req, Loc);
  addRequired(NewNode);
  Node->addRequirement(NewNode);
}

void WitnessStrategy::requireSource(WitnessGraphNode *Node, Value *Req,
                                    Instruction *Loc) const {
  auto &WG = Node->Graph;
  auto *NewNode = getInternalNode(WG, Req, Loc);
  NewNode->HasAllRequirements = true;
  if (NewNode != Node) {
    Node->addRequirement(NewNode);
  }
}
