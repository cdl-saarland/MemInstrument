//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/NoneStrategy.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void NoneStrategy::addRequired(WitnessGraphNode *Node) const {
  Node->HasAllRequirements = true;
}

void NoneStrategy::createWitness(InstrumentationMechanism &IM,
                                 WitnessGraphNode *Node) const {
  if (Node->Target->hasWitness()) {
    // We already handled this node.
    return;
  }

  IM.insertWitness(*(Node->Target));
}

void NoneStrategy::simplifyWitnessGraph(WitnessGraph &WG) const {}
