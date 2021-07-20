//===- OptimizationInterface.cpp - Interface for Optimizations ------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/OptimizationInterface.h"

using namespace llvm;

namespace meminstrument {

OptimizationInterface::~OptimizationInterface() {}

bool OptimizationInterface::prepareModule(MemInstrumentPass &, Module &) {
  return false;
}

void OptimizationInterface::materializeExternalChecksForFunction(
    MemInstrumentPass &, ITargetVector &, llvm::Function &) {}

} // namespace meminstrument
