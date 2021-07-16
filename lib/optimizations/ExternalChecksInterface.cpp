//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/ExternalChecksInterface.h"

using namespace llvm;

namespace meminstrument {
ExternalChecksInterface::~ExternalChecksInterface(void) {}

bool ExternalChecksInterface::prepareModule(MemInstrumentPass &, Module &) {
  return false;
}
} // namespace meminstrument
