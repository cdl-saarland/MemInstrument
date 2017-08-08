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

#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace meminstrument {

struct WitnessGraphNode {
  std::shared_ptr<ITarget> Target;
  llvm::SmallVector<WitnessGraphNode *, 4> Requirements;

  WitnessGraphNode(std::shared_ptr<ITarget> Target) : Target(Target) {
    static unsigned long RunningId = 0;
    id = RunningId++;
  }

  bool ToMaterialize = false;

  bool Required = false;

  friend class WitnessGraph;

private:
  unsigned long id;
};

class WitnessGraph {
public:
  WitnessGraphNode *getNodeForOrNull(std::shared_ptr<ITarget> T);

  WitnessGraphNode *getNodeFor(std::shared_ptr<ITarget> T);

  WitnessGraphNode *createNewNodeFor(std::shared_ptr<ITarget> T);

  void propagateITargetFlags(void);

  WitnessGraph(const llvm::Function &F) : Func(F) {}

  ~WitnessGraph(void) {
    for (auto &P : NodeMap) {
      delete (P.second);
    }
  }

  void printDotGraph(llvm::raw_ostream &stream) const;

private:
  const llvm::Function &Func;
  typedef std::pair<llvm::Value *, llvm::Instruction *> KeyType;

  llvm::DenseMap<KeyType, WitnessGraphNode *> NodeMap;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSGRAPH_H
