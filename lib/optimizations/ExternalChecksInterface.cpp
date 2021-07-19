//===- ExternalChecksInterface.cpp - Interface for Optimizations ----------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/ExternalChecksInterface.h"

using namespace llvm;

namespace meminstrument {
ExternalChecksInterface::~ExternalChecksInterface(void) {}

bool ExternalChecksInterface::prepareModule(MemInstrumentPass &, Module &) {
  return false;
}
} // namespace meminstrument
