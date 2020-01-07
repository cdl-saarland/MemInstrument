//===---------------------------------------------------------------------===///
///
/// \file This file provides some helper functions and gives the right settings
///     for LLVM's debug printing.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_UTIL_H
#define MEMINSTRUMENT_PASS_UTIL_H

#include "llvm/IR/GlobalObject.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

#ifndef NDEBUG
#define DEBUG_ALSO_WITH_TYPE(TYPE, X)                                          \
  do {                                                                         \
    if (::llvm::DebugFlag && (::llvm::isCurrentDebugType(DEBUG_TYPE) ||        \
                              ::llvm::isCurrentDebugType(TYPE))) {             \
      X;                                                                       \
    }                                                                          \
  } while (false)
#else
#define DEBUG_ALSO_WITH_TYPE(TYPE, X)                                          \
  do {                                                                         \
  } while (false)
#endif

namespace meminstrument {

/// Add metadata to a Value to indiate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps. Limited to GlobalObjects and Instructions.
void setNoInstrument(llvm::Value *V);

/// Add metadata an instruction to indiate that it was introduced by the
/// instrumentation to handle byval arguments.
void setByvalHandling(llvm::Instruction *I);

/// Check a Value for metadata to indiate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(llvm::GlobalObject *O);

/// Check a Value for metadata to indiate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(llvm::Instruction *I);

/// Check for metadata on an instruction to indiate that it was introduced by
/// the instrumentation to handle byval arguments.
bool hasByvalHandling(llvm::Instruction *I);

/// Check whether V has type pointer to something with a size or pointer to a
/// function.
bool hasPointerAccessSize(llvm::Value *V);

/// Get the size of an access to the pointer Value V.
size_t getPointerAccessSize(const llvm::DataLayout &DL, llvm::Value *V);

} // namespace meminstrument

#endif
