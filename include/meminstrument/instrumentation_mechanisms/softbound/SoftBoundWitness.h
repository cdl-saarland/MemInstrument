//===-------- meminstrument/SoftBoundWitness.h - TODO -----------*- C++ -*-===//
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
  virtual auto getLowerBound() const -> llvm::Value * override;

  virtual auto getUpperBound() const -> llvm::Value * override;

  static bool classof(const Witness *W);
};

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDWITNESS_H
