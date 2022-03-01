//===- meminstrument/ITargets.h - Instrumentation Targets -------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_ITARGET_H
#define MEMINSTRUMENT_PASS_ITARGET_H

#include "meminstrument/pass/Witness.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <functional>
#include <memory>
#include <vector>

namespace llvm {
class raw_ostream;
class DominatorTree;
} // namespace llvm

namespace meminstrument {

class ITarget;

using ITargetPtr = std::shared_ptr<ITarget>;

using ITargetVector = std::vector<ITargetPtr>;

class ITargetBuilder {
public:
  /// Static factory method for creating an ITarget for a called function
  /// pointer (instrumentee) at the location call. The result is a CallCheckIT.
  static ITargetPtr createCallCheckTarget(llvm::Value *instrumentee,
                                          llvm::CallBase *call);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location.
  /// The (constant) access size is derived from Instrumentee.
  /// The result is a CheckIT.
  /// The caller should validate with a call to
  /// meminstrument::validateSize(Instrumentee) that deriving an access size is
  /// possible.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *instrumentee,
                                             llvm::Instruction *location,
                                             bool checkUpper = true,
                                             bool checkLower = true);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with access size Size.
  /// The result is a ConstSizeCheckIT.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *instrumentee,
                                             llvm::Instruction *location,
                                             size_t size,
                                             bool checkUpper = true,
                                             bool checkLower = true);

  /// Static factory method for creating an ITarget for a spatial (upper and
  /// lower bound) check for Instrumentee at Location with variable access size
  /// Size. The result is a VarSizeCheckIT.
  static ITargetPtr createSpatialCheckTarget(llvm::Value *instrumentee,
                                             llvm::Instruction *location,
                                             llvm::Value *size,
                                             bool checkUpper = true,
                                             bool checkLower = true);

  /// Static factory method for creating an ITarget for an invariant check for
  /// the argNum-th call argument of call.
  /// The result is an ArgInvariantIT.
  static ITargetPtr createArgInvariantTarget(llvm::Value *instrumentee,
                                             llvm::CallBase *call,
                                             unsigned argNum);

  /// Static factory method for creating an ITarget for a call invariant. The
  /// result is a CallInvariantIT.
  static ITargetPtr createCallInvariantTarget(llvm::CallBase *);

  /// Static factory method for creating an ITarget for a call invariant which
  /// requires information on its arguments. The result is a CallInvariantIT.
  static ITargetPtr createCallInvariantTarget(
      llvm::CallBase *, const std::map<unsigned, llvm::Value *> &requiredArgs);

  /// Static factory method for creating an ITarget for an invariant check for
  /// Instrumentee at Location.
  /// The result is a ValInvariantIT.
  static ITargetPtr createValInvariantTarget(llvm::Value *instrumentee,
                                             llvm::Instruction *location);

  /// Static factory method for creating an ITarget that requires that bounds
  /// are available for Instrumentee at Location.
  /// The result is a BoundsIT.
  static ITargetPtr createBoundsTarget(llvm::Value *instrumentee,
                                       llvm::Instruction *location,
                                       bool checkUpper = true,
                                       bool checkLower = true);

  /// Static factory method for creating an ITarget that marks locations where
  /// instrumentation is necessary to determine the source of a pointer. Selects
  /// or phis that have pointer type are typical use cases. The result is an
  /// IntermediateIT.
  static ITargetPtr createIntermediateTarget(llvm::Value *instrumentee,
                                             llvm::Instruction *location);

  /// Static factory method for creating an ITarget that marks locations where
  /// pointer sources are. Allocas, calls and pointer loads are typical cases.
  /// The result is a SourceIT.
  static ITargetPtr createSourceTarget(llvm::Value *instrumentee,
                                       llvm::Instruction *location);

  /// Determine the number of ITargets that fulfill the given predicate.
  static size_t
  getNumITargets(const ITargetVector &,
                 const std::function<bool(const ITarget &)> &predicate);

  /// Return the number of valid targets in the given ITarget vector.
  static size_t getNumValidITargets(const ITargetVector &);

  /// Check whether each instrumentee instruction is strictly dominated by its
  /// location, return true iff this is the case.
  static bool validateITargets(const llvm::DominatorTree &,
                               const ITargetVector &);
};

/// ITargets are indicators that a Value needs some treatment at a specific
/// location. The ITargetKind specifies which treatment.
class ITarget {
public:
  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum class ITargetKind {
    ITK_Bounds,
    ITK_Intermediate,
    ITK_Source,
    ITK_CallCheck,
    ITK_ConstSizeCheck,
    ITK_VarSizeCheck,
    ITK_ArgInvariant,
    ITK_CallInvariant,
    ITK_ValInvariant,
  };

  /// Returns true iff the target is a (constant, variable size or call) check.
  bool isCheck() const;

  /// Returns true iff the target is an (argument, call or value) invariant.
  bool isInvariant() const;

  /// Returns true iff the target has an instrumentee.
  bool hasInstrumentee() const;

  /// The value that should be checked by the instrumentation.
  llvm::Value *getInstrumentee() const;

  /// The instruction before which the instrumentee should be checkable.
  llvm::Instruction *getLocation() const;

  /// Indicator whether instrumentee should be checked against its upper bound.
  bool hasUpperBoundFlag() const;

  /// Indicator whether instrumentee should be checked against its lower bound.
  bool hasLowerBoundFlag() const;

  /// Indicator whether temporal safety of instrumentee should be checked.
  bool hasTemporalFlag() const;

  /// Indicator whether explicit bound information is required.
  virtual bool requiresExplicitBounds() const;

  /// Indicator whether a bound witness is available.
  bool hasBoundWitnesses() const;

  /// Indicator whether a bound witness is required for this target.
  virtual bool needsNoBoundWitnesses() const;

  /// Indicator whether a bound witness is available if required.
  bool hasWitnessesIfNeeded() const;

  /// Returns the bound witness(es) of the target. Make sure to check that it
  /// has a bound witness before requesting it.
  WitnessPtr getSingleBoundWitness() const;
  WitnessPtr getBoundWitness(unsigned index) const;
  WitnessMap getBoundWitnesses() const;

  /// Adds (a) bound witness(es) to the target.
  void setSingleBoundWitness(WitnessPtr);
  void setBoundWitness(WitnessPtr, unsigned index);
  void setBoundWitnesses(const WitnessMap &);

  /// Indicator whether the ITarget has been invalidated and should therefore
  /// not be realized.
  bool isValid() const;

  /// Invalidate the target.
  void invalidate();

  /// Returns the kind of this ITarget. Required for LLVM-style RTTI, should not
  /// be necessary anywhere else.
  ITargetKind getKind() const;

  ITarget() = delete;

  virtual ~ITarget() = default;

  virtual bool operator==(const ITarget &) const = 0;

  void printLocation(llvm::raw_ostream &) const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, const ITarget &);

private:
  const ITargetKind kind;

protected:
  ITarget(ITargetKind k, llvm::Value *instrumentee, llvm::Instruction *location,
          bool checkUpper, bool checkLower, bool checkTemporal);

  llvm::Value *instrumentee;

  llvm::Instruction *location;

  bool checkUpperBoundFlag = false;

  bool checkLowerBoundFlag = false;

  bool checkTemporalFlag = false;

  bool invalidated;

  /// List of bound witnesses for this target.
  /// This will only contain more than one element for aggregate type. These can
  /// contain several pointers and therefore require a witness for each of them.
  /// The individual witnesses will be inserted in an arbitrary order, the index
  /// into the map describes to which value in the aggregate the witness
  /// belongs.
  WitnessMap boundWitnesses;

  virtual void dump(llvm::raw_ostream &) const = 0;

  /// Compares two targets equal.
  /// Provided to allow subclasses to conveniently implement their comparison
  /// operator.
  bool equals(const ITarget &) const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &, const ITarget &);

class CheckIT : public ITarget {
public:
  virtual bool operator==(const ITarget &) const override = 0;

  static bool classof(const ITarget *);

protected:
  CheckIT(ITargetKind, llvm::Value *instrumentee, llvm::Instruction *location,
          bool checkUpper, bool checkLower, bool checkTemporal);
};

class CallCheckIT : public CheckIT {
public:
  CallCheckIT(llvm::Value *instrumentee, llvm::CallBase *call);

  llvm::CallBase *getCall() const;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);
};

class ConstSizeCheckIT : public CheckIT {
public:
  ConstSizeCheckIT(llvm::Value *instrumentee, llvm::Instruction *location,
                   size_t accessSize, bool checkUpper, bool checkLower);

  size_t getAccessSize() const;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);

private:
  size_t accessSize = 0;
};

class VarSizeCheckIT : public CheckIT {
public:
  VarSizeCheckIT(llvm::Value *instrumentee, llvm::Instruction *location,
                 llvm::Value *accessSizeVal, bool checkUpper, bool checkLower);

  llvm::Value *getAccessSizeVal() const;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);

private:
  llvm::Value *accessSizeVal = nullptr;
};

class InvariantIT : public ITarget {
public:
  virtual bool operator==(const ITarget &) const override = 0;

  static bool classof(const ITarget *);

protected:
  InvariantIT(ITargetKind, llvm::Value *instrumentee,
              llvm::Instruction *location);
};

class ArgInvariantIT : public InvariantIT {
public:
  ArgInvariantIT(llvm::Value *instrumentee, llvm::CallBase *call,
                 unsigned argNum);

  unsigned getArgNum() const;

  llvm::CallBase *getCall() const;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);

private:
  unsigned argNum = 0;
};

class CallInvariantIT : public InvariantIT {
public:
  CallInvariantIT(llvm::CallBase *);

  CallInvariantIT(llvm::CallBase *,
                  const std::map<unsigned, llvm::Value *> &requiredArgs);

  bool needsNoBoundWitnesses() const override;

  llvm::CallBase *getCall() const;

  std::map<unsigned, llvm::Value *> getRequiredArgs() const;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);

private:
  std::map<unsigned, llvm::Value *> requiredArgs;
};

class ValInvariantIT : public InvariantIT {
public:
  ValInvariantIT(llvm::Value *instrumentee, llvm::Instruction *location);

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);
};

class BoundsIT : public ITarget {
public:
  BoundsIT(llvm::Value *instrumentee, llvm::Instruction *location,
           bool checkUpper, bool checkLower);

  bool requiresExplicitBounds() const override;

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);
};

class WitnessSupplyIT : public ITarget {
public:
  virtual bool operator==(const ITarget &) const override = 0;

  bool requiresExplicitBounds() const override;

  bool joinFlags(const ITarget &);

  static bool classof(const ITarget *);

protected:
  WitnessSupplyIT(ITargetKind, llvm::Value *instrumentee,
                  llvm::Instruction *location);

  bool explicitBoundsFlag = false;
};

class IntermediateIT : public WitnessSupplyIT {
public:
  IntermediateIT(llvm::Value *instrumentee, llvm::Instruction *location);

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);
};

class SourceIT : public WitnessSupplyIT {
public:
  SourceIT(llvm::Value *instrumentee, llvm::Instruction *location);

  void dump(llvm::raw_ostream &) const override;

  bool operator==(const ITarget &) const override;

  static bool classof(const ITarget *);
};

} // namespace meminstrument

namespace llvm {
// Enable the usage of ITargetPtr combined with LLVM-style RTTI
template <> struct simplify_type<meminstrument::ITargetPtr> {
  using SimpleType = meminstrument::ITarget *;

  static SimpleType getSimplifiedValue(meminstrument::ITargetPtr &val) {
    return val.get();
  }
};
} // namespace llvm

#endif
