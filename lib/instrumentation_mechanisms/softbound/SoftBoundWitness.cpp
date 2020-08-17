//===------------------- SoftBoundWitness.cpp - TODO ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// TODO
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/softbound/SoftBoundWitness.h"

#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace meminstrument;

//===----------------------------------------------------------------------===//
//                   Implementation of SoftBoundWitness
//===----------------------------------------------------------------------===//

auto SoftBoundWitness::getLowerBound() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundWitness::getUpperBound() const -> Value * {
  llvm_unreachable("TODO implement");
}

bool SoftBoundWitness::classof(const Witness *W) {
  return W->getKind() == WK_SoftBound;
}
