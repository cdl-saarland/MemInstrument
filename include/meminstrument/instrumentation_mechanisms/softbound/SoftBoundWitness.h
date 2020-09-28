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

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDWITNESS_H
