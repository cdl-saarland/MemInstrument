//===---- meminstrument/Witness.h -- MemSafety Instrumentation  -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_WITNESS_H
#define MEMINSTRUMENT_WITNESS_H

#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

namespace meminstrument {

/// TODO document
struct Witness {
public:
  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum WitnessKind {
    WK_Dummy,
    WK_Splay,
    // Insert new WitnessKinds for each new type of witnesses here!
  };

  virtual llvm::Value *getLowerBound(void) const = 0;
  virtual llvm::Value *getUpperBound(void) const = 0;

  virtual ~Witness(void) {}

  Witness(WitnessKind Kind) : Kind(Kind) {}

  WitnessKind getKind(void) const { return Kind; }

private:
  const WitnessKind Kind;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESS_H
