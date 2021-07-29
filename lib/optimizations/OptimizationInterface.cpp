//===- OptimizationInterface.cpp - Interface for Optimizations ------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/OptimizationInterface.h"

using namespace llvm;

namespace meminstrument {

bool OptimizationInterface::prepareModule(MemInstrumentPass &, Module &) {
  return false;
}

void OptimizationInterface::updateITargetsForModule(
    MemInstrumentPass &mip,
    std::map<Function *, ITargetVector> &targetsPerFun) {
  for (auto &entry : targetsPerFun) {
    updateITargetsForFunction(mip, entry.second, *entry.first);
  }
}

void OptimizationInterface::updateITargetsForFunction(MemInstrumentPass &,
                                                      ITargetVector &,
                                                      Function &) {}

void OptimizationInterface::materializeExternalChecksForFunction(
    MemInstrumentPass &, ITargetVector &, Function &) {}

OptimizationInterface::~OptimizationInterface() {}

} // namespace meminstrument
