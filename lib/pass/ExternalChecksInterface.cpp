//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ExternalChecksInterface.h"

namespace meminstrument {
ExternalChecksInterface::~ExternalChecksInterface(void) {}

bool ExternalChecksInterface::prepareModule(MemInstrumentPass &,
                                            llvm::Module &) {
  return false;
}
} // namespace meminstrument
