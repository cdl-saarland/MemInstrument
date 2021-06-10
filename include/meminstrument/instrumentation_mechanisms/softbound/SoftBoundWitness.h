//===----- meminstrument/SoftBoundWitness.h - Bound Witnesses ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Witness implementation for SoftBound.
///
/// Two kinds of witnesses are available:
/// The commonly used `SoftBoundWitness` directly provides the metadata required
/// for checking.
///
/// The `SoftBoundVarArgWitness` is a special witness for variable argument
/// functions. This witness does not directly provide the metadata, but it can
/// be used to query metadata for varargs from the run-time.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDWITNESS_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDWITNESS_H

#include "meminstrument/pass/Witness.h"

namespace meminstrument {

class SoftBoundWitness : public Witness {

public:
  SoftBoundWitness(llvm::Value *lowerBound, llvm::Value *upperBound,
                   llvm::Value *ptr);

  virtual auto getLowerBound() const -> llvm::Value * override;

  virtual auto getUpperBound() const -> llvm::Value * override;

  static bool classof(const Witness *W);

private:
  llvm::Value *lowerBound = nullptr;
  llvm::Value *upperBound = nullptr;
  llvm::Value *ptr = nullptr;
};

class SoftBoundVarArgWitness : public Witness {

public:
  SoftBoundVarArgWitness(llvm::Value *varArgProxy);

  virtual auto getLowerBound() const -> llvm::Value * override;

  virtual auto getUpperBound() const -> llvm::Value * override;

  auto getProxy() const -> llvm::Value *;

  static bool classof(const Witness *W);

private:
  llvm::Value *varArgProxy = nullptr;
};

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDWITNESS_H
