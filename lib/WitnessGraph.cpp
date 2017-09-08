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

#include "llvm/Support/FileSystem.h"

#include <queue>
#include <set>

using namespace meminstrument;
using namespace llvm;

void WitnessGraphNode::addRequirement(WitnessGraphNode *N) {
  assert(N != nullptr);

  _Requirements.push_back(N);
  N->_RequiredBy.push_back(this);
}

void WitnessGraphNode::clearRequirements(void) {
  for (auto *Req : _Requirements) {
    auto &Vec = Req->_RequiredBy;
    Vec.erase(std::remove_if(Vec.begin(), Vec.end(),
                             [&Req](WitnessGraphNode *&N) { return N == Req; }),
              Vec.end());
  }
  _Requirements.clear();
}

WitnessGraphNode *
WitnessGraph::getInternalNode(std::shared_ptr<ITarget> Target) {
  // see whether we already have a node for Target
  if (auto *Node = getInternalNodeOrNull(Target)) {
    return Node;
  }
  // create a new node for Target
  return createNewInternalNode(Target);
}

WitnessGraphNode *
WitnessGraph::createNewInternalNode(std::shared_ptr<ITarget> Target) {
  assert(getInternalNodeOrNull(Target) == nullptr &&
         "Internal node already exists!");
  auto Key = std::make_pair(Target->Instrumentee, Target->Location);
  auto *NewNode = new WitnessGraphNode(*this, Target);
  InternalNodes.insert(std::make_pair(Key, NewNode));

  return NewNode;
}

WitnessGraphNode *
WitnessGraph::getInternalNodeOrNull(std::shared_ptr<ITarget> Target) {
  auto Key = std::make_pair(Target->Instrumentee, Target->Location);
  auto It = InternalNodes.find(Key);
  if (It != InternalNodes.end()) {
    auto *Node = It->getSecond();
    return Node;
  }
  return nullptr;
}

void WitnessGraph::insertRequiredTarget(std::shared_ptr<ITarget> T) {
  auto *Res = new WitnessGraphNode(*this, T);
  LeafNodes.push_back(Res);
  Strategy.addRequired(Res);
  AlreadyPropagated = false;
}

void WitnessGraph::propagateFlags(void) {
  std::queue<WitnessGraphNode *> Worklist;
  for (auto &Node : LeafNodes) {
    Worklist.push(Node);
  }

  while (!Worklist.empty()) {
    auto *Node = Worklist.front();
    Worklist.pop();
    for (auto &Succ : Node->getRequiredNodes()) {
      if (Succ->Target->joinFlags(*Node->Target)) {
        Worklist.push(Succ);
      }
    }
  }

  AlreadyPropagated = true;
}

void WitnessGraph::createWitnesses(InstrumentationMechanism &IM) {
  assert(AlreadyPropagated &&
         "Call `propagateRequirements()` before creating witnesses!");
  for (auto &Node : LeafNodes) {
    Strategy.createWitness(IM, Node);
  }
}

void WitnessGraph::printDotGraph(llvm::raw_ostream &stream) const {
  std::map<WitnessGraphNode *, WitnessGraphNode *> edges;
  printDotGraph(stream, edges);
}

void WitnessGraph::printDotGraph(
    llvm::raw_ostream &stream,
    std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const {
  stream << "digraph witnessgraph_" << Func.getName() << "\n{\n";
  stream << "  rankdir=BT;\n";
  stream << "  label=\"Witness Graph for `" << Func.getName() << "`:\";\n";
  stream << "  labelloc=top;\n";
  stream << "  labeljust=left;\n";

  for (const auto &Node : LeafNodes) {
    stream << "  n" << Node->id << " [label=\"" << *(Node->Target) << "\"";
    stream << ", color=blue];\n";
  }

  for (const auto &P : InternalNodes) {
    const auto &Node = P.getSecond();
    stream << "  n" << Node->id << " [label=\"" << *(Node->Target) << "\"";
    if (Node->getRequiredNodes().size() == 0) {
      stream << ", color=red";
    }
    stream << "];\n";
  }

  stream << "\n";

  for (const auto &Node : LeafNodes) {
    for (const auto *Other : Node->getRequiredNodes()) {
      stream << "  n" << Node->id << " -> n" << Other->id << ";\n";
    }
  }

  for (const auto &P : InternalNodes) {
    const auto &Node = P.getSecond();
    for (const auto *Other : Node->getRequiredNodes()) {
      stream << "  n" << Node->id << " -> n" << Other->id << ";\n";
    }
  }

  for (const auto &E : edges) {
    if (E.first == nullptr || E.second == nullptr) {
      continue;
    }
    stream << "  n" << E.first->id << " -> n" << E.second->id
           << "[color=green];\n";
  }

  stream << "}\n";
}

void WitnessGraph::dumpDotGraph(const std::string &filename) const {
  std::map<WitnessGraphNode *, WitnessGraphNode *> edges;
  dumpDotGraph(filename, edges);
}

void WitnessGraph::dumpDotGraph(
    const std::string &filename,
    std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const {
  std::error_code EC;
  raw_ostream *fout = new raw_fd_ostream(filename, EC, sys::fs::F_None);

  if (EC) {
    errs() << "Failed to open file for witness graph\n";
  } else {
    printDotGraph(*fout, edges);
  }
  delete (fout);
}

void WitnessGraph::printWitnessClasses(llvm::raw_ostream &Stream) const {
  llvm::DenseMap<Witness *, std::set<ITarget *>> PrintMap;

  for (auto &Node : LeafNodes) {
    auto &Target = Node->Target;
    assert(Target->hasWitness());
    PrintMap[Target->BoundWitness.get()].insert(Target.get());
  }

  for (auto &Pair : InternalNodes) {
    auto *Node = Pair.getSecond();
    auto &Target = Node->Target;
    assert(Target->hasWitness());
    PrintMap[Target->BoundWitness.get()].insert(Target.get());
  }

  for (auto &Pair : PrintMap) {
    for (auto *Target : Pair.getSecond()) {
      Stream << "(" << Target->Instrumentee->getName() << ", ";
      Target->printLocation(Stream);
      Stream << ")"
             << "; ";
    }
    Stream << "\n";
  }
}

namespace {

void markAsReachable(std::set<WitnessGraphNode *> &Res, WitnessGraphNode *N) {
  if (Res.find(N) != Res.end()) {
    return;
  }

  Res.insert(N);

  for (auto &O : N->getRequiredNodes()) {
    markAsReachable(Res, O);
  }
}
}

void WitnessGraph::removeDeadNodes(void) {
  std::set<WitnessGraphNode *> DoNotRemove;

  for (auto &N : LeafNodes) {
    markAsReachable(DoNotRemove, N);
  }

  for (auto it = InternalNodes.begin(); it != InternalNodes.end();) {
    if (DoNotRemove.find(it->second) != DoNotRemove.end()) {
      ++it;
    } else {
      it->second->clearRequirements();
      delete (it->second);
      InternalNodes.erase(it++);
    }
  }
}

void WitnessGraph::map(const std::function<void(WitnessGraphNode *)> &f) {
  for (auto &N : LeafNodes) {
    f(N);
  }
  for (auto &Pair : InternalNodes) {
    f(Pair.second);
  }
}
