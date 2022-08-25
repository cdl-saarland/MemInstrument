//===- meminstrument/Witness.h - Pointer Witness Interface ------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// A witness provides the information that is necessary to perform a memory
/// safety check. The information that is stored in the witness is specific to
/// the InstrumentationMechanism, and hence, each one defines its own kind of
/// witness.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_WITNESS_H
#define MEMINSTRUMENT_PASS_WITNESS_H

#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <map>

namespace meminstrument {

class Witness;

using WitnessPtr = std::shared_ptr<Witness>;

using WitnessMap = std::map<unsigned, WitnessPtr>;

/// A Witness is the information that an instrumentation needs to verify that
/// an access to a pointer is valid. This is typically a set of LLVM values that
/// hold some kind of bounds information.
/// Witnesses are closely related to InstrumentationMechanisms and should be
/// implemented with them.
struct Witness {
public:
  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum WitnessKind {
    WK_Dummy,
    WK_Splay,
    WK_RuntimeStat,
    WK_Noop,
    WK_Lowfat,
    WK_SoftBound,
    WK_SoftBoundVarArg,
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

} // namespace meminstrument

#endif
