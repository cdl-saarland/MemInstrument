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
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <memory>
#include <vector>

namespace meminstrument {

class ITarget;

typedef std::shared_ptr<ITarget> ITargetPtr;

typedef std::vector<ITargetPtr> ITargetVector;

size_t getNumITargets(const ITargetVector &IV,
                      const std::function<bool(const ITarget &)> &Predicate);

size_t getNumValidITargets(const ITargetVector &IV);

/// check whether each instrumentee instruction is strictly dominated by its
/// location, return true iff this is the case
bool validateITargets(const llvm::DominatorTree &dt, const ITargetVector &IV);

class ITarget {
public:
  enum class Kind {
    Bounds,
    ConstSizeCheck,
    VarSizeCheck,
    Intermediate,
    Invariant,
  };

  bool is(Kind k) const;

  Kind getKind(void) const;

  /// returns true iff the target is a (const or var size) check
  bool isCheck() const;

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

  std::shared_ptr<Witness> getBoundWitness(void);

  void setBoundWitness(std::shared_ptr<Witness> BoundWitness);

  /// indicator whether the ITarget has been invalidated and should therefore
  /// not be realized.
  bool isValid(void) const;

  void invalidate(void);

  bool subsumes(const ITarget &other) const;

  bool joinFlags(const ITarget &other);

  /// Static factory method for creating an ITarget that requires that bounds
  /// are available for Instrumentee at Location.
  /// The result will have the kind Instrumentee::Kind::Bounds.
  static ITargetPtr createBoundsTarget(llvm::Value *Instrumentee,
                                       llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for an invariant check for
  /// Instrumentee at Location.
  /// The result will have the kind Instrumentee::Kind::Invariant.
  static ITargetPtr createInvariantTarget(llvm::Value *Instrumentee,
                                          llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location.
  /// The (constant) access size is derived from Instrumentee.
  /// The result will have the kind Instrumentee::Kind::ConstSizeCheck.
  /// The caller should validate with a call to
  /// meminstrument::validateSize(Instrumentee) that deriving an access size is
  /// possible.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with access size Size.
  /// The result will have the kind Instrumentee::Kind::ConstSizeCheck.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location,
                                             size_t Size);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with variable access size
  /// Size. The result will have the kind Instrumentee::Kind::VarSizeCheck.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location,
                                             llvm::Value *Size);

  static ITargetPtr createIntermediateTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location);

  static ITargetPtr createIntermediateTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location,
                                             const ITarget &other);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Stream,
                                       const ITarget &It);

  void printLocation(llvm::raw_ostream &Stream) const;

private:
  ITarget(Kind k)
      : _Kind(k), _BoundWitness(std::shared_ptr<Witness>(nullptr)) {}

  llvm::Value *_Instrumentee;

  llvm::Instruction *_Location;

  const Kind _Kind;

  bool _CheckUpperBoundFlag = false;

  bool _CheckLowerBoundFlag = false;

  bool _CheckTemporalFlag = false;

  bool _RequiresExplicitBounds = false;

  bool _Invalidated = false;

  std::shared_ptr<Witness> _BoundWitness;

  size_t _AccessSize = 0;
  llvm::Value *_AccessSizeVal = nullptr;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &stream, const ITarget &IT);

} // namespace meminstrument

#endif
