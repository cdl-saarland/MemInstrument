//===- meminstrument/WitnessGraph.h - Witness Graph -------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_WITNESSGRAPH_H
#define MEMINSTRUMENT_PASS_WITNESSGRAPH_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include <functional>
#include <memory>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace meminstrument {

class WitnessGraph;
class WitnessStrategy;

struct WitnessGraphNode {
  WitnessGraph &Graph;
  ITargetPtr Target;

  /// Gets a vector of nodes that are required by this node.
  ///
  /// Allows traversal from actual instrumentation targets to the definitions
  /// of the contributing pointers.
  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiredNodes() const {
    return requirements;
  }

  /// Gets a vector of nodes that require this node.
  ///
  /// Allows traversal from the definitions of the contributing pointers to
  /// actual instrumentation targets.
  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiringNodes() const {
    return requiredBy;
  }

  /// Unlinks this Node from the Nodes that are required by it.
  void clearRequirements(void);

  void addRequirement(WitnessGraphNode *N);

  bool HasAllRequirements = false;

  ~WitnessGraphNode(void);

private:
  WitnessGraphNode(WitnessGraph &WG, ITargetPtr Target)
      : Graph(WG), Target(Target) {
    static unsigned long RunningId = 0;
    id = RunningId++;
  }

  llvm::SmallVector<WitnessGraphNode *, 4> requirements;

  llvm::SmallVector<WitnessGraphNode *, 4> requiredBy;

  unsigned long id;

  friend class WitnessGraph;
};

/// A graph for representing data flow from definitions of pointer values to
/// ITargets.
///
/// The WitnessGraph contains externally required nodes (or extern nodes), and
/// internal nodes.
/// External nodes can be added with a call to `insertRequiredTarget` for a
/// given ITarget. The goal is to compute witnesses for these ITargets.
/// Internal nodes only represent temporary ITargets that are required to
/// compute witnesses for the external nodes.
/// There is at most one internal node for each instrumentee/location pair,
/// however there can be additionally arbitrary many external nodes for the same
/// pair.
///
/// In a complete witness graph, every node should have require-paths from one
/// or more external source nodes and to one or more (typically internal) sink
/// nodes (i.e. nodes with no requirements).
class WitnessGraph {
public:
  void insertRequiredTarget(ITargetPtr T);

  WitnessGraphNode *getInternalNodeOrNull(ITargetPtr T);

  WitnessGraphNode *getInternalNode(ITargetPtr T);

  WitnessGraphNode *createNewInternalNode(ITargetPtr T);

  void removeDeadNodes(void);

  void propagateFlags(void);

  void createWitnesses(InstrumentationMechanism &IM);

  WitnessGraph(const llvm::Function &F, const WitnessStrategy &WS)
      : Func(F), Strategy(WS) {}

  ~WitnessGraph(void) {
    for (auto &N : ExternalNodes) {
      delete N;
    }
    for (auto &N : InternalNodes) {
      delete N;
    }
  }

  /// Gets a vector of the nodes registered as externally required and that
  /// should get witnesses.
  std::vector<WitnessGraphNode *> &getExternalNodes(void) {
    return ExternalNodes;
  }

  /// Applies a function to every node in the WitnessGraph (including internal
  /// and externally required nodes).
  void map(const std::function<void(WitnessGraphNode *)> &f);

  /// Prints a representation of this WitnessGraph in the graphviz/dot format to
  /// a stream.
  void printDotGraph(llvm::raw_ostream &stream) const;

  /// Prints a representation of this WitnessGraph in the graphviz/dot format to
  /// a stream with additional highlighted edges.
  void
  printDotGraph(llvm::raw_ostream &stream,
                std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const;

  /// Prints a representation of this WitnessGraph in the graphviz/dot format to
  /// a new file.
  void dumpDotGraph(const std::string &filename) const;

  /// Prints a representation of this WitnessGraph in the graphviz/dot format to
  /// a new file with additional highlighted edges.
  void
  dumpDotGraph(const std::string &filename,
               std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const;

  /// Print all ITargets in the graph grouped by their associated witnesses.
  ///
  /// ITargets are printed in the same line iff they have the same witness
  /// value. (Used for testing.)
  void printWitnessClasses(llvm::raw_ostream &stream) const;

private:
  const llvm::Function &Func;
  const WitnessStrategy &Strategy;

  /// Artificial nodes that correspond to temporary intermediate
  /// targets that are inserted to generate witnesses for the externally
  /// required nodes.
  std::vector<WitnessGraphNode *> InternalNodes;

  /// A vector of nodes that are required externally and which need witnesses.
  std::vector<WitnessGraphNode *> ExternalNodes;

  bool AlreadyPropagated = false;
};

} // namespace meminstrument

#endif
