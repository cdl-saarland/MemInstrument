//===- meminstrument/PointerBoundsPolicy.h -- Ptr-based Policy --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// A policy for a pointer-based (i.e. not object-based) memory safety
/// instrumentation.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_POINTERBOUNDSPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_POINTERBOUNDSPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"

namespace meminstrument {

class PointerBoundsPolicy : public InstrumentationPolicy {

public:
  PointerBoundsPolicy(GlobalConfig &);

  auto getName() const -> const char * override;

  void classifyTargets(std::vector<ITargetPtr> &, llvm::Instruction *) override;

private:
  /// Add all targets required upon a call.
  void addCallTargets(std::vector<ITargetPtr> &, llvm::CallInst *);

  /// Insert an invariant target if the current instruction returns or stores an
  /// aggregate type.
  void insertInvariantTargetAggregate(std::vector<ITargetPtr> &,
                                      llvm::Instruction *);

  /// Check if the given type is not a nested aggregate, e.g. a struct
  /// containing struct members.
  bool isNested(const llvm::Type *Aggregate) const;

  /// Checks if the given aggregate type contains pointer values.
  /// Does not work for nested aggregates.
  bool containsPointer(const llvm::Type *Aggregate) const;
};

} // namespace meminstrument

#endif
