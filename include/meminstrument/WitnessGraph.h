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

#include <memory>

namespace meminstrument {

struct WitnessGraphNode {
  std::shared_ptr<ITarget> Target;
  llvm::SmallVector<WitnessGraphNode *, 4> Requirements;

  WitnessGraphNode(std::shared_ptr<ITarget> Target) : Target(Target) {}
};

class WitnessGraph {
public:
  WitnessGraphNode *getNodeForOrNull(std::shared_ptr<ITarget> T);

  WitnessGraphNode *getNodeFor(std::shared_ptr<ITarget> T);

  WitnessGraphNode *createNewNodeFor(std::shared_ptr<ITarget> T);

  void propagateITargetFlags(void);

  ~WitnessGraph(void) {
    for (auto &P : NodeMap) {
      delete (P.second);
    }
  }

private:
  typedef std::pair<llvm::Value *, llvm::Instruction *> KeyType;

  llvm::DenseMap<KeyType, WitnessGraphNode *> NodeMap;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSGRAPH_H
