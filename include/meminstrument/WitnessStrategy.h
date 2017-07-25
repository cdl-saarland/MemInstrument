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
  void addImmediatelyRequiredTargets(WitnessGraph& WG, ITarget* Target) const;

  void insertImmediateWitness(WitnessGraphNode* Node) const;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESSSTRATEGY_H
