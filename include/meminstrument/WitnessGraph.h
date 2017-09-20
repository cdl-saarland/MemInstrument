//===----- meminstrument/WitnessGraph.h -- MemSafety Instr. --*- C++ -*----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESSGRAPH_H
#define MEMINSTRUMENT_WITNESSGRAPH_H

#include "meminstrument/ITarget.h"
#include "meminstrument/InstrumentationMechanism.h"
#include "meminstrument/WitnessStrategy.h"

#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <memory>

namespace meminstrument {

class WitnessGraph;
class WitnessStrategy;

struct WitnessGraphNode {
  WitnessGraph &Graph;
  std::shared_ptr<ITarget> Target;

  /// Gets a vector of nodes that are required by this node.
  ///
  /// Allows traversal from actual instrumentation targets to the defintions
  /// of the contributing pointers.
  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiredNodes() const {
    return _Requirements;
  }

  /// Gets a vector of nodes that require this node.
  ///
  /// Allows traversal from the defintions of the contributing pointers to
  /// actual instrumentation targets.
  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiringNodes() const {
    return _RequiredBy;
  }

  /// Unlinks this Node from the Nodes that are required by it.
  void clearRequirements(void);

  void addRequirement(WitnessGraphNode *N);

  bool HasAllRequirements = false;

  ~WitnessGraphNode(void);

private:
  WitnessGraphNode(WitnessGraph &WG, std::shared_ptr<ITarget> Target)
      : Graph(WG), Target(Target) {
    static unsigned long RunningId = 0;
    id = RunningId++;
  }

  llvm::SmallVector<WitnessGraphNode *, 4> _Requirements;

  llvm::SmallVector<WitnessGraphNode *, 4> _RequiredBy;

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
  void insertRequiredTarget(std::shared_ptr<ITarget> T);

  WitnessGraphNode *getInternalNodeOrNull(std::shared_ptr<ITarget> T);

  WitnessGraphNode *getInternalNode(std::shared_ptr<ITarget> T);

  WitnessGraphNode *createNewInternalNode(std::shared_ptr<ITarget> T);

  void removeDeadNodes(void);

  void propagateFlags(void);

  void createWitnesses(InstrumentationMechanism &IM);

  WitnessGraph(const llvm::Function &F, const WitnessStrategy &WS)
      : Func(F), Strategy(WS) {}

  ~WitnessGraph(void) {
    for (auto &N : ExternalNodes) {
      delete N;
    }
    for (auto &P : InternalNodes) {
      delete (P.second);
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
  typedef std::pair<llvm::Value *, llvm::Instruction *> KeyType;

  /// A map of artificial nodes that correspond to temporary intermediate
  /// targets that are inserted to generate witnesses for the externally
  /// required nodes.
  llvm::DenseMap<KeyType, WitnessGraphNode *> InternalNodes;

  /// A vector of nodes that are required externally and which need witnesses.
  std::vector<WitnessGraphNode *> ExternalNodes;

  bool AlreadyPropagated = false;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSGRAPH_H
