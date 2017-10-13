//===------ meminstrument/pass/Util.h -- MemSafety Instrumentation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
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

void setNoInstrument(llvm::GlobalObject *O);

void setNoInstrument(llvm::Constant *C);

void setNoInstrument(llvm::Instruction *I);

bool hasNoInstrument(llvm::GlobalObject *O);

bool hasNoInstrument(llvm::Instruction *I);
} // namespace meminstrument

#endif
