//===---- meminstrument/ITarget.h -- MemSafety Instrumentation  -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_ITARGET_H
#define MEMINSTRUMENT_ITARGET_H

#include "meminstrument/Witness.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

namespace meminstrument {

/// TODO document
struct ITarget {
  /// value that should be checked by the instrumentation
  llvm::Value *Instrumentee;

  /// instruction before which the instrumentee should be checked
  llvm::Instruction *Location;

  /// access size in bytes that should be checked starting from the instrumentee
  size_t AccessSize;

  /// indicator whether instrumentee should be checked against its upper bound
  bool CheckUpperBoundFlag;

  /// indicator whether instrumentee should be checked against its lower bound
  bool CheckLowerBoundFlag;

  /// indicator whether temporal safety of instrumentee should be checked
  bool CheckTemporalFlag;

  /// indicator whether explicit bound information is required
  bool RequiresExplicitBounds;

  std::shared_ptr<Witness> BoundWitness;

  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          size_t AccessSize, bool CheckUpperBoundFlag, bool CheckLowerBoundFlag,
          bool CheckTemporalFlag, bool RequiresExplicitBounds);
  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          size_t AccessSize, bool CheckUpperBoundFlag, bool CheckLowerBoundFlag,
          bool RequiresExplicitBounds);
  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          size_t AccessSize, bool RequiresExplicitBounds);
  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          size_t AccessSize);

  void joinFlags(const ITarget &other);

  bool hasWitness(void) const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Stream,
                                       const ITarget &It);
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &stream, const ITarget &IT);

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGET_H
