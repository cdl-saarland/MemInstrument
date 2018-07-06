//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ExternalChecksInterface.h"

namespace meminstrument {
ExternalChecksInterface::~ExternalChecksInterface(void) {}

bool ExternalChecksInterface::prepareModule(MemInstrumentPass &P,
                                            llvm::Module &M) {
  return false;
}
} // namespace meminstrument
