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

void NoneStrategy::simplifyWitnessGraph(WitnessGraph &WG) const {}
