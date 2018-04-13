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

typedef std::shared_ptr<ITarget> ITargetPtr;

typedef std::vector<ITargetPtr> ITargetVector;

class ITarget {
public:
  enum class Kind {
    Bounds,
    Check,
    VarSizeCheck,
    Intermediate,
    Invariant,
  };

  bool is(Kind k) const;

  Kind getKind(void) const;

  /// value that should be checked by the instrumentation
  llvm::Value *getInstrumentee(void) const;

  /// instruction before which the instrumentee should be checkable
  llvm::Instruction *getLocation(void) const;

  size_t getAccessSize(void) const;

  llvm::Value *getAccessSizeVal(void) const;

  /// indicator whether instrumentee should be checked against its upper bound
  bool hasUpperBoundFlag(void) const;

  /// indicator whether instrumentee should be checked against its lower bound
  bool hasLowerBoundFlag(void) const;

  /// indicator whether temporal safety of instrumentee should be checked
  bool hasTemporalFlag(void) const;

  /// indicator whether explicit bound information is required
  bool requiresExplicitBounds(void) const;

  bool hasBoundWitness(void) const;

  Witness &getBoundWitness(void);


  /// indicator whether the ITarget has been invalidated and should therefore
  /// not be realized.
  bool isValid(void) const;

  void invalidate(void);


  bool subsumes(const ITarget &other) const;

  bool joinFlags(const ITarget &other);


  static ITargetPtr createBoundsTarget(llvm::Value* Instrumentee, llvm::Value* Location);

  static ITargetPtr createInvariantTarget(llvm::Value* Instrumentee, llvm::Value* Location);

  static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, size_t Size);

  static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, llvm::Value *Size);

  static ITargetPtr createIntermediateTarget(llvm::Value* Instrumentee, llvm::Value* Location, const ITarget &other);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Stream,
                                       const ITarget &It);
private:
  ITarget(Kind k) : _Kind(k), _BoundWitness(std::shared_ptr(nullptr)) {}

  bool _CheckUpperBoundFlag = false;

  bool _CheckLowerBoundFlag = false;

  bool _CheckTemporalFlag = false;

  bool _RequiresExplicitBounds = false;

  bool _Invalidated = false;

  std::shared_ptr<Witness> _BoundWitness;

  size_t _AccessSize = 0;
  llvm::Value *_AccessSizeVal = nullptr;

  const Kind _Kind;
};


llvm::raw_ostream &operator<<(llvm::raw_ostream &stream, const ITarget &IT);

} // namespace meminstrument

#endif
