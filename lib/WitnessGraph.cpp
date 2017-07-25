//===----------- WitnessGraph.cpp -- MemSafety Instrumentation ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/WitnessGraph.h"

using namespace meminstrument;
using namespace llvm;

WitnessGraphNode* WitnessGraph::getNodeFor(ITarget *Target) {
  // see whether we already have a node for Target
  auto It = NodeMap.find(std::make_pair(Target->Instrumentee, Target->Location));
  if (It != NodeMap.end()) { // we found one, so just use this
    auto* Node = It->getSecond();
    Node->Target->joinFlags(*Target);
    return Node;
  }
  // create a new node for Target
  auto* NewNode = new WitnessGraphNode(Target);
  NodeMap.insert(std::make_pair(std::make_pair(Target->Instrumentee, Target->Location), NewNode));
  return NewNode;
}

