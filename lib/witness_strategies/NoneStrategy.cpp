//===- NoneStrategy.cpp - None Strategy -----------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/NoneStrategy.h"

using namespace meminstrument;
using namespace llvm;

void NoneStrategy::addRequired(WitnessGraphNode *Node) const {
  Node->HasAllRequirements = true;
}

void NoneStrategy::createWitness(InstrumentationMechanism &IM,
                                 WitnessGraphNode *Node) const {
  if (Node->Target->hasBoundWitnesses()) {
    // We already handled this node.
    return;
  }

  if (Node->Target->needsNoBoundWitnesses()) {
    // No witness required.
    return;
  }

  IM.insertWitnesses(*(Node->Target));
}
