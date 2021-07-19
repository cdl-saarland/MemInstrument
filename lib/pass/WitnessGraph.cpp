//===- WitnessGraph.cpp - Graph for Witness Information -------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///

#include "meminstrument/pass/WitnessGraph.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <queue>
#include <set>

using namespace meminstrument;
using namespace llvm;

void WitnessGraphNode::addRequirement(WitnessGraphNode *N) {
  assert(N != nullptr);

  requirements.push_back(N);
  N->requiredBy.push_back(this);
}

void WitnessGraphNode::clearRequirements(void) {
  for (auto *Req : requirements) {
    auto &Vec = Req->requiredBy;
    Vec.erase(std::remove_if(Vec.begin(), Vec.end(),
                             [&Req](WitnessGraphNode *&N) { return N == Req; }),
              Vec.end());
  }
  requirements.clear();
}

WitnessGraphNode::~WitnessGraphNode(void) { clearRequirements(); }

WitnessGraphNode *WitnessGraph::getInternalNode(ITargetPtr Target) {
  // see whether we already have a node for Target
  if (auto *Node = getInternalNodeOrNull(Target)) {
    return Node;
  }
  // create a new node for Target
  return createNewInternalNode(Target);
}

WitnessGraphNode *WitnessGraph::createNewInternalNode(ITargetPtr Target) {
  assert(getInternalNodeOrNull(Target) == nullptr &&
         "Internal node already exists!");

  auto *NewNode = new WitnessGraphNode(*this, Target);
  InternalNodes.push_back(NewNode);

  return NewNode;
}

WitnessGraphNode *WitnessGraph::getInternalNodeOrNull(ITargetPtr Target) {

  for (auto *WGN : InternalNodes) {
    if (*WGN->Target == *Target) {
      return WGN;
    }
  }

  return nullptr;
}

void WitnessGraph::insertRequiredTarget(ITargetPtr T) {
  auto *Res = new WitnessGraphNode(*this, T);
  ExternalNodes.push_back(Res);
  Strategy.addRequired(Res);
  AlreadyPropagated = false;
}

void WitnessGraph::propagateFlags(void) {
  std::queue<WitnessGraphNode *> Worklist;
  for (auto &Node : ExternalNodes) {
    Worklist.push(Node);
  }

  while (!Worklist.empty()) {
    auto *Node = Worklist.front();
    Worklist.pop();
    for (auto &Succ : Node->getRequiredNodes()) {
      auto InterIT = cast<IntermediateIT>(Succ->Target);
      if (InterIT->joinFlags(*Node->Target)) {
        Worklist.push(Succ);
      }
    }
  }

  AlreadyPropagated = true;
}

void WitnessGraph::createWitnesses(InstrumentationMechanism &IM) {
  assert(AlreadyPropagated &&
         "Call `propagateRequirements()` before creating witnesses!");
  for (auto &Node : ExternalNodes) {
    Strategy.createWitness(IM, Node);
  }
}

void WitnessGraph::printDotGraph(raw_ostream &stream) const {
  std::map<WitnessGraphNode *, WitnessGraphNode *> edges;
  printDotGraph(stream, edges);
}

void WitnessGraph::printDotGraph(
    raw_ostream &stream,
    std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const {
  stream << "digraph witnessgraph_" << Func.getName() << "\n{\n";
  stream << "  rankdir=BT;\n";
  stream << "  label=\"Witness Graph for `" << Func.getName() << "`:\";\n";
  stream << "  labelloc=top;\n";
  stream << "  labeljust=left;\n";

  for (const auto &Node : ExternalNodes) {
    stream << "  n" << Node->id << " [label=\"" << *(Node->Target) << "\"";
    stream << ", color=blue];\n";
  }

  for (const auto &Node : InternalNodes) {
    stream << "  n" << Node->id << " [label=\"" << *(Node->Target) << "\"";
    if (Node->getRequiredNodes().size() == 0) {
      stream << ", color=red";
    }
    stream << "];\n";
  }

  stream << "\n";

  for (const auto &Node : ExternalNodes) {
    for (const auto *Other : Node->getRequiredNodes()) {
      stream << "  n" << Node->id << " -> n" << Other->id << ";\n";
    }
  }

  for (const auto &Node : InternalNodes) {
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

void WitnessGraph::printWitnessClasses(raw_ostream &Stream) const {
  DenseMap<Witness *, std::set<ITarget *>> PrintMap;

  for (auto &Node : ExternalNodes) {
    auto &Target = Node->Target;
    if (Target->needsNoBoundWitnesses()) {
      continue;
    }
    assert(Target->hasBoundWitnesses());
    for (auto &KV : Target->getBoundWitnesses()) {
      PrintMap[KV.second.get()].insert(Target.get());
    }
  }

  for (auto *Node : InternalNodes) {
    auto &Target = Node->Target;
    if (Target->needsNoBoundWitnesses()) {
      continue;
    }
    assert(Target->hasBoundWitnesses());
    for (auto &KV : Target->getBoundWitnesses()) {
      PrintMap[KV.second.get()].insert(Target.get());
    }
  }

  for (auto &Pair : PrintMap) {
    for (auto *Target : Pair.getSecond()) {

      Stream << "("
             << (Target->hasInstrumentee()
                     ? (Target->getInstrumentee()->hasName()
                            ? Target->getInstrumentee()->getName()
                            : "None")
                     : "None")
             << ", ";
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
} // namespace

void WitnessGraph::removeDeadNodes(void) {
  std::set<WitnessGraphNode *> DoNotRemove;

  std::vector<WitnessGraphNode *> ToDelete;

  ExternalNodes.erase(std::remove_if(ExternalNodes.begin(), ExternalNodes.end(),
                                     [&](WitnessGraphNode *N) {
                                       bool res = !N->Target->isValid();
                                       if (res) {
                                         ToDelete.push_back(N);
                                       }
                                       return res;
                                     }),
                      ExternalNodes.end());

  for (auto *N : ToDelete) {
    delete N;
  }

  for (auto &N : ExternalNodes) {
    markAsReachable(DoNotRemove, N);
  }

  for (auto it = InternalNodes.begin(); it != InternalNodes.end();) {
    if (DoNotRemove.find(*it) != DoNotRemove.end()) {
      ++it;
    } else {
      delete *it;
      it = InternalNodes.erase(it);
    }
  }
}

void WitnessGraph::map(const std::function<void(WitnessGraphNode *)> &f) {
  for (auto &N : ExternalNodes) {
    f(N);
  }
  for (auto &E : InternalNodes) {
    f(E);
  }
}
