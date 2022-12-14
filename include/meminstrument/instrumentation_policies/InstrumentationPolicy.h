//===- meminstrument/InstrumentationPolicy.h - Policy Interface -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The instrumentation policy describes which locations in the code /
/// instructions of the program require instrumentation. A typical example are
/// in-bounds checks that should be placed at loads or stores. The specific
/// policy (a subclass of this abstract one) describes the kinds of
/// instrumentation that will be made. E.g., it describes locations where checks
/// are placed or at which invariants are established. What the checks
/// or invariants look like is defined by the specific instrumentation
/// mechanism, and not the task of the policy.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

/// Subclasses of this abstract class describe where memory safety checks
/// should be placed.
/// This class is not concerned with how these checks are implemented, refer to
/// InstrumentationMechanism for this issue.
class InstrumentationPolicy {
public:
  /// Add the ITargets necessary at instruction Loc to ensure memory safety to
  /// Dest.
  virtual void classifyTargets(ITargetVector &Dest, llvm::Instruction *Loc) = 0;

  /// Returns the name of the instrumentation policy for printing and easy
  /// recognition.
  virtual const char *getName(void) const = 0;

  virtual ~InstrumentationPolicy(void) {}

protected:
  InstrumentationPolicy();

  /// Helper function to check whether Ptr is a Value with type pointer
  /// pointing to some type with a size. If this is not the case, an error is
  /// reported.
  void validateSize(llvm::Value *Ptr);

  /// Create check targets for the given intrinsic.
  bool insertCheckTargetsForIntrinsic(ITargetVector &Dest,
                                      llvm::IntrinsicInst *II) const;

  /// Create check targets for a load or store instruction.
  void insertCheckTargetsLoadStore(ITargetVector &Dest,
                                   llvm::Instruction *Inst);

  /// Create check targets for the given call.
  void insertCheckTargetsForCall(ITargetVector &Dest, llvm::CallBase *);

  /// Iff a pointer is stored to memory, this function will create an invariant
  /// target for the stored pointer.
  void insertInvariantTargetStore(ITargetVector &Dest, llvm::StoreInst *) const;

  /// Iff a pointer is returned from a function, create an
  /// invariant target for the returned pointer.
  void insertInvariantTargetReturn(ITargetVector &Dest,
                                   llvm::ReturnInst *) const;

  /// Iff a pointer is inserted into an aggregate, create an
  /// invariant target for the stored value.
  void insertInvariantTargetInsertVal(ITargetVector &,
                                      llvm::InsertValueInst *) const;
};

} // namespace meminstrument

#endif
