//===- SoftBoundWitness.cpp - Lower+Upper Bound Witnesses for SoftBound ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Connects a pointer ptr with its lower and upper bound value, lowerBound and
/// upperBound.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/softbound/SoftBoundWitness.h"

#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace meminstrument;

//===----------------------------------------------------------------------===//
//                   Implementation of SoftBoundWitness
//===----------------------------------------------------------------------===//

SoftBoundWitness::SoftBoundWitness(Value *lowerBound, Value *upperBound,
                                   Value *ptr)
    : Witness(WK_SoftBound), lowerBound(lowerBound), upperBound(upperBound),
      ptr(ptr) {
  assert(lowerBound && upperBound && ptr);
}

auto SoftBoundWitness::getLowerBound() const -> Value * { return lowerBound; }

auto SoftBoundWitness::getUpperBound() const -> Value * { return upperBound; }

bool SoftBoundWitness::classof(const Witness *W) {
  return W->getKind() == WK_SoftBound;
}
