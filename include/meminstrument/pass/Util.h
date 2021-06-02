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

/// Add metadata to a Value to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps. Limited to GlobalObjects and Instructions.
void setNoInstrument(llvm::Value *V);

/// Add metadata to a Value to indicate that it was identified as vaarg
/// handling.
void setVarArgHandling(llvm::Value *V);

/// Add metadata an instruction to indicate that it was introduced by the
/// instrumentation to handle byval arguments.
void setByvalHandling(llvm::Instruction *I);

/// Check a Value for metadata to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(const llvm::GlobalObject *O);

/// Check a Value for metadata to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(const llvm::Instruction *I);

/// Check an Instruction for metadata that indicates that this instruction was
/// identified as vararg handling.
bool hasVarArgHandling(const llvm::Instruction *I);

/// Check for metadata on an instruction to indicate that it was introduced by
/// the instrumentation to handle byval arguments.
bool hasByvalHandling(const llvm::Instruction *I);

/// Check whether V has type pointer to something with a size or pointer to a
/// function.
bool hasPointerAccessSize(const llvm::Value *V);

/// Get the size of an access to the pointer Value V.
size_t getPointerAccessSize(const llvm::DataLayout &DL, llvm::Value *V);

/// Determine whether the given type is or points to the llvm type for a va_list
bool isVarArgMetadataType(llvm::Type *type);

/// If the function has varargs or a va_list as argument, return the values
/// through which the varargs can be accessed.
/// For non-vararg function or declarations, the returned list is empty.
llvm::SmallVector<llvm::Value *, 2> getVarArgHandles(llvm::Function &F);

} // namespace meminstrument

#endif
