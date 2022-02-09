//===- WitnessStrategy.cpp - Strategy Interface ---------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/WitnessStrategy.h"

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
