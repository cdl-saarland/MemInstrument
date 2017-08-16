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
#include "llvm/ADT/DenseMapInfo.h"

#include <memory>

namespace meminstrument {

enum WGNKind {
  WitnessSource,
  WitnessPropagator,
  WitnessSink,
  WGNKEmpty,
  WGNKTombstone,
};

struct WitnessGraphNode {
  WGNKind Kind;
  std::shared_ptr<ITarget> Target;
  llvm::SmallVector<WitnessGraphNode *, 4> Requirements;

private:
  WitnessGraphNode(WGNKind Kind, std::shared_ptr<ITarget> Target) : Kind(Kind), Target(Target) {
    static unsigned long RunningId = 0;
    id = RunningId++;
  }

  friend class WitnessGraph;

  unsigned long id;
};

class WitnessGraph {
public:
  WitnessGraphNode *getNodeForOrNull(WGNKind Kind, std::shared_ptr<ITarget> T);

  WitnessGraphNode *getNodeFor(WGNKind Kind, std::shared_ptr<ITarget> T);

  WitnessGraphNode *createNewNodeFor(WGNKind Kind, std::shared_ptr<ITarget> T);

  WitnessGraph(const llvm::Function &F) : Func(F) {}

  void propagateRequirements(void);

  ~WitnessGraph(void) {
    for (auto &P : NodeMap) {
      delete (P.second);
    }
  }

  void printDotGraph(llvm::raw_ostream &stream) const;

private:
  const llvm::Function &Func;
  typedef std::pair<std::pair<llvm::Value *, llvm::Instruction *>, WGNKind> KeyType;

  llvm::DenseMap<KeyType, WitnessGraphNode *> NodeMap;
};

} // end namespace meminstrument

namespace llvm {

template<> struct DenseMapInfo<meminstrument::WGNKind> {
  static inline meminstrument::WGNKind getEmptyKey() { return meminstrument::WGNKEmpty; }
  static inline meminstrument::WGNKind getTombstoneKey() { return meminstrument::WGNKTombstone; }
  static unsigned getHashValue(const meminstrument::WGNKind& Val) { return unsigned(Val); }

  static bool isEqual(const meminstrument::WGNKind &LHS, const meminstrument::WGNKind &RHS) {
    return LHS == RHS;
  }
};
}

#endif // MEMINSTRUMENT_WITNESSGRAPH_H
