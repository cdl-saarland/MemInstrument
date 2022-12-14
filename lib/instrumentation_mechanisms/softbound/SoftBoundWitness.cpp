//===- SoftBoundWitness.cpp - Lower+Upper Bound Witnesses for SoftBound ---===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// SoftBoundWitness:
/// Connects a pointer ptr with its lower and upper bound value, lowerBound and
/// upperBound.
///
/// SoftBoundVarArgWitness:
/// Contains the proxy object for varargs. It can be used to request metadata
/// for varargs.
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

//===----------------------------------------------------------------------===//
//                 Implementation of SoftBoundVarArgWitness
//===----------------------------------------------------------------------===//

SoftBoundVarArgWitness::SoftBoundVarArgWitness(Value *varArgProxy)
    : Witness(WK_SoftBoundVarArg), varArgProxy(varArgProxy) {
  assert(varArgProxy);
}

auto SoftBoundVarArgWitness::getLowerBound() const -> Value * {
  llvm_unreachable("Operation not supported by this witness kind");
}

auto SoftBoundVarArgWitness::getUpperBound() const -> Value * {
  llvm_unreachable("Operation not supported by this witness kind");
}

auto SoftBoundVarArgWitness::getProxy() const -> Value * { return varArgProxy; }

bool SoftBoundVarArgWitness::classof(const Witness *W) {
  return W->getKind() == WK_SoftBoundVarArg;
}
