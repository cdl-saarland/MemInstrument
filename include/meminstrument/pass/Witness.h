//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_WITNESS_H
#define MEMINSTRUMENT_PASS_WITNESS_H

#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

namespace meminstrument {

/// A Witness is the information that an instrumentation needs to verify that
/// an access to a pointer is valid. This is typically a set of LLVM values that
/// hold some kind of bounds information.
/// Witnesses are closely related to InstructionMechanisms and should be
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
