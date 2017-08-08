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

WitnessGraphNode *WitnessGraph::getNodeFor(std::shared_ptr<ITarget> Target) {
  // see whether we already have a node for Target
  if (auto *Node = getNodeForOrNull(Target)) {
    return Node;
  }
  // create a new node for Target
  return createNewNodeFor(Target);
}

WitnessGraphNode *
WitnessGraph::createNewNodeFor(std::shared_ptr<ITarget> Target) {
  assert(getNodeForOrNull(Target) == nullptr);
  auto Key = std::make_pair(Target->Instrumentee, Target->Location);
  auto *NewNode = new WitnessGraphNode(Target);
  NodeMap.insert(std::make_pair(Key, NewNode));
  return NewNode;
}

WitnessGraphNode *
WitnessGraph::getNodeForOrNull(std::shared_ptr<ITarget> Target) {
  auto Key = std::make_pair(Target->Instrumentee, Target->Location);
  auto It = NodeMap.find(Key);
  if (It != NodeMap.end()) {
    auto *Node = It->getSecond();
    Node->Target->joinFlags(*Target);
    return Node;
  }
  return nullptr;
}

void WitnessGraph::printDotGraph(llvm::raw_ostream &stream) const {
  stream << "digraph witnessgraph_" << Func.getName() << "\n{\n";
  stream << "  rankdir=BT;\n";
  stream << "  label=\"Witness Graph for `" << Func.getName() << "`:\";\n";
  stream << "  labelloc=top;\n";
  stream << "  labeljust=left;\n";

  for (const auto &P : NodeMap) {
    const auto &Node = P.getSecond();
    stream << "  n" << Node->id << " [label=\"" << *(Node->Target) << "\"";
    if (Node->Required) {
      stream << ", color=blue";
    } else if (Node->ToMaterialize) {
      stream << ", color=red";
    }
    stream << "];\n";
  }

  stream << "\n";

  for (const auto &P : NodeMap) {
    const auto &Node = P.getSecond();
    for (const auto *Other : Node->Requirements) {
      stream << "  n" << Node->id << " -> n" << Other->id << ";\n";
    }
  }

  stream << "}\n";
}
