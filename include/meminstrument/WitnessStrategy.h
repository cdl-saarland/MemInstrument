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
#include "meminstrument/WitnessGraph.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class WitnessStrategy {
public:
  virtual WitnessGraphNode *
  constructWitnessGraph(WitnessGraph &WG,
                        std::shared_ptr<ITarget> Target) const = 0;

  virtual ~WitnessStrategy(void) {}
};

// FIXME/TODO rename this!
class TodoBetterNameStrategy : public WitnessStrategy {
public:
  virtual WitnessGraphNode *
  constructWitnessGraph(WitnessGraph &WG,
                        std::shared_ptr<ITarget> Target) const override;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSSTRATEGY_H
