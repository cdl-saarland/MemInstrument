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

  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiredNodes() const {
    return _Requirements;
  }

  const llvm::SmallVectorImpl<WitnessGraphNode *> &getRequiringNodes() const {
    return _RequiredBy;
  }

  void clearRequirements(void);

  void addRequirement(WitnessGraphNode *N);

  bool HasAllRequirements = false;

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
    for (auto &N : LeafNodes) {
      delete N;
    }
    for (auto &P : InternalNodes) {
      delete (P.second);
    }
  }

  void map(const std::function<void(WitnessGraphNode *)> &f);

  void printDotGraph(llvm::raw_ostream &stream) const;

  void
  printDotGraph(llvm::raw_ostream &stream,
                std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const;

  void dumpDotGraph(const std::string &filename) const;

  void
  dumpDotGraph(const std::string &filename,
               std::map<WitnessGraphNode *, WitnessGraphNode *> &edges) const;

  void printWitnessClasses(llvm::raw_ostream &stream) const;

private:
  const llvm::Function &Func;
  const WitnessStrategy &Strategy;
  typedef std::pair<llvm::Value *, llvm::Instruction *> KeyType;

  llvm::DenseMap<KeyType, WitnessGraphNode *> InternalNodes;

  std::vector<WitnessGraphNode *> LeafNodes;

  bool AlreadyPropagated = false;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSGRAPH_H
