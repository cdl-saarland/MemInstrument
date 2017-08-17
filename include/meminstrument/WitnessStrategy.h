//===--- meminstrument/WitnessStrategy.h -- MemSafety Instr. --*- C++ -*---===//
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

#ifndef MEMINSTRUMENT_WITNESSSTRATEGY_H
#define MEMINSTRUMENT_WITNESSSTRATEGY_H

#include "meminstrument/ITarget.h"
#include "meminstrument/InstrumentationMechanism.h"
#include "meminstrument/WitnessGraph.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

struct WitnessGraphNode;

class WitnessStrategy {
public:
  virtual ~WitnessStrategy(void) {}

  virtual void addRequired(WitnessGraphNode *Node) const = 0;

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const = 0;

  static const WitnessStrategy &get(void);

protected:
  void requireRecursively(WitnessGraphNode *Node, llvm::Value *Req,
                          llvm::Instruction *Loc, ITarget &Target) const;
  void requireSource(WitnessGraphNode *Node, llvm::Value *Req,
                     llvm::Instruction *Loc, ITarget &Target) const;
};

// TODO rename this!
class SimpleStrategy : public WitnessStrategy {
public:
  virtual void addRequired(WitnessGraphNode *Node) const override;

  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const override;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSSTRATEGY_H
