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

/// ITargets are indicators that a Value needs some treatment at a specific
/// location. The Kind specifies, which treatment.
class ITarget {
public:
  enum class Kind {
    Bounds,         /// explicit bounds need to be available
    ConstSizeCheck, /// inbounds check with a compile-time constant size
    VarSizeCheck,   /// inbounds check with a compile-time variable size
    Intermediate,   /// witnesses need to be propagated
    Invariant,      /// an invariant needs to be checked
    CallCheck,      /// a function call needs to be checked
    CallInvariant,  /// a function call invariant needs to be checked
  };

  bool is(Kind k) const;

  Kind getKind(void) const;

  /// returns true iff the target is a (const, var size or call) check
  bool isCheck() const;

  /// returns true iff the target is a (call) invariant
  bool isInvariant() const;

  /// value that should be checked by the instrumentation
  llvm::Value *getInstrumentee(void) const;

  /// instruction before which the instrumentee should be checkable
  llvm::Instruction *getLocation(void) const;

  size_t getAccessSize(void) const;

  llvm::Value *getAccessSizeVal(void) const;

  /// returns true iff the target has an instrumentee
  bool hasInstrumentee(void) const;

  /// indicator whether instrumentee should be checked against its upper bound
  bool hasUpperBoundFlag(void) const;

  /// indicator whether instrumentee should be checked against its lower bound
  bool hasLowerBoundFlag(void) const;

  /// indicator whether temporal safety of instrumentee should be checked
  bool hasTemporalFlag(void) const;

  /// indicator whether explicit bound information is required
  bool requiresExplicitBounds(void) const;

  /// indicator whether a bound witness is available
  bool hasBoundWitness(void) const;

  /// indicator whether a bound witness is required for this target
  bool needsNoBoundWitness(void) const;

  /// indicator whether a bound witness is available if required
  bool hasWitnessIfNeeded(void) const;

  std::shared_ptr<Witness> getBoundWitness(void) const;

  void setBoundWitness(std::shared_ptr<Witness> BoundWitness);

  /// indicator whether the ITarget has been invalidated and should therefore
  /// not be realized.
  bool isValid(void) const;

  void invalidate(void);

  bool joinFlags(const ITarget &other);

  /// Static factory method for creating an ITarget that requires that bounds
  /// are available for Instrumentee at Location.
  /// The result will have the kind ITarget::Kind::Bounds.
  static ITargetPtr createBoundsTarget(llvm::Value *Instrumentee,
                                       llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for an invariant check for
  /// Instrumentee at Location.
  /// The result will have the kind ITarget::Kind::Invariant.
  static ITargetPtr createInvariantTarget(llvm::Value *Instrumentee,
                                          llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location.
  /// The (constant) access size is derived from Instrumentee.
  /// The result will have the kind ITarget::Kind::ConstSizeCheck.
  /// The caller should validate with a call to
  /// meminstrument::validateSize(Instrumentee) that deriving an access size is
  /// possible.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with access size Size.
  /// The result will have the kind ITarget::Kind::ConstSizeCheck.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location,
                                             size_t Size);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with variable access size
  /// Size. The result will have the kind ITarget::Kind::VarSizeCheck.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location,
                                             llvm::Value *Size);

  static ITargetPtr createIntermediateTarget(llvm::Value *Instrumentee,
                                             llvm::Instruction *Location);

  /// Static factory method for creating an ITarget for a called function
  /// pointer (Instrumentee) at the location Call. The result will have the kind
  /// ITarget::Kind::CallCheck.
  static ITargetPtr createCallCheckTarget(llvm::Value *Instrumentee,
                                          llvm::CallInst *Call);

  /// Static factory method for creating an ITarget for a call invariant. The
  /// result will have the kind ITarget::Kind::CallInvariant.
  static ITargetPtr createCallInvariantTarget(llvm::CallInst *Call);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &Stream,
                                       const ITarget &It);

  void printLocation(llvm::raw_ostream &Stream) const;

  bool operator==(const ITarget &) const;

private:
  ITarget(Kind k) : kind(k), boundWitness(std::shared_ptr<Witness>(nullptr)) {}

  llvm::Value *instrumentee;

  llvm::Instruction *location;

  const Kind kind;

  bool checkUpperBoundFlag = false;

  bool checkLowerBoundFlag = false;

  bool checkTemporalFlag = false;

  bool explicitBoundsFlag = false;

  bool invalidated = false;

  std::shared_ptr<Witness> boundWitness;

  size_t accessSize = 0;
  llvm::Value *accessSizeVal = nullptr;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &stream, const ITarget &IT);

} // namespace meminstrument

#endif
