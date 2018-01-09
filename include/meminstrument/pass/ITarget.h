//===------ meminstrument/pass/ITarget.h -- MemSafety Instr. -*- C++ -*----===//
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

#ifndef MEMINSTRUMENT_PASS_ITARGET_H
#define MEMINSTRUMENT_PASS_ITARGET_H

#include "meminstrument/pass/Witness.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <vector>

namespace meminstrument {

/// TODO document
struct ITarget {
  /// value that should be checked by the instrumentation
  llvm::Value *Instrumentee;

  /// instruction before which the instrumentee should be checked
  llvm::Instruction *Location;

  /// access size in bytes that should be checked starting from the instrumentee
  size_t AccessSize;

  bool HasConstAccessSize;

  llvm::Value *AccessSizeVal;

  /// indicator whether instrumentee should be checked against its upper bound
  bool CheckUpperBoundFlag;

  /// indicator whether instrumentee should be checked against its lower bound
  bool CheckLowerBoundFlag;

  /// indicator whether temporal safety of instrumentee should be checked
  bool CheckTemporalFlag;

  /// indicator whether explicit bound information is required
  bool RequiresExplicitBounds;

  /// indicator whether the ITarget has been invalidated and should therefore
  /// not be realized.
  bool isValid(void) const { return !_Invalidated; }

  void invalidate(void) { _Invalidated = true; }

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

  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          llvm::Value *AccessSize, bool CheckUpperBoundFlag,
          bool CheckLowerBoundFlag, bool CheckTemporalFlag,
          bool RequiresExplicitBounds);
  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          llvm::Value *AccessSize, bool CheckUpperBoundFlag,
          bool CheckLowerBoundFlag, bool RequiresExplicitBounds);
  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          llvm::Value *AccessSize, bool RequiresExplicitBounds);

  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          llvm::Value *AccessSize);

  ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
          bool RequiresExplicitBounds); // TODO think about that

  ITarget(llvm::Value *Instrumentee,
          llvm::Instruction *Location); // TODO think about that

  bool subsumes(const ITarget &other) const;

  bool joinFlags(const ITarget &other);

  bool hasWitness(void) const;

  void printLocation(llvm::raw_ostream &Stream) const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Stream,
                                       const ITarget &It);

private:
  bool _Invalidated = false;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &stream, const ITarget &IT);

typedef std::vector<std::shared_ptr<ITarget>> ITargetVector;

} // namespace meminstrument

#endif
