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

WitnessGraphNode *WitnessGraph::getNodeFor(WGNKind Kind, std::shared_ptr<ITarget> Target) {
  // see whether we already have a node for Target
  if (auto *Node = getNodeForOrNull(Kind, Target)) {
    return Node;
  }
  // create a new node for Target
  return createNewNodeFor(Kind, Target);
}

WitnessGraphNode *
WitnessGraph::createNewNodeFor(WGNKind Kind, std::shared_ptr<ITarget> Target) {
  assert(getNodeForOrNull(Kind, Target) == nullptr);
  auto Key = std::make_pair(std::make_pair(Target->Instrumentee, Target->Location), Kind);
  auto *NewNode = new WitnessGraphNode(Kind, Target);
  NodeMap.insert(std::make_pair(Key, NewNode));
  return NewNode;
}

WitnessGraphNode *
WitnessGraph::getNodeForOrNull(WGNKind Kind, std::shared_ptr<ITarget> Target) {
  auto Key = std::make_pair(std::make_pair(Target->Instrumentee, Target->Location), Kind);
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
    if (Node->Kind == WitnessSink) {
      stream << ", color=blue";
    } else if (Node->Kind == WitnessSource) {
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
