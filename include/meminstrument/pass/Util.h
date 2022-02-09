//===- meminstrument/Util.h - Helper functions ------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
///
/// \file
/// This file provides some helper functions and gives the right settings for
/// LLVM's debug printing.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_UTIL_H
#define MEMINSTRUMENT_PASS_UTIL_H

#include "llvm/ADT/Twine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"

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

namespace llvm {
class Function;
class GlobalObject;
class Instruction;
class Value;
class DataLayout;
template <typename T, unsigned> class SmallVector;
class Type;
} // namespace llvm

namespace meminstrument {

/// MemInstrument mechanism to report fatal errors.
class MemInstrumentError : public llvm::ErrorInfo<MemInstrumentError> {
public:
  static char ID;

  MemInstrumentError(const llvm::Twine &msg) : errorMessage(msg) {}

  std::error_code convertToErrorCode() const override;

  void log(llvm::raw_ostream &) const override;

  [[noreturn]] static void report(const llvm::Twine &);

  [[noreturn]] static void report(const llvm::Twine &, llvm::Value *);

private:
  const llvm::Twine &errorMessage;
};

/// Add metadata to a Value to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps. Limited to GlobalObjects and Instructions.
void setNoInstrument(llvm::Value *);

/// Add metadata to a Value to indicate that it was identified as vararg
/// handling.
void setVarArgHandling(llvm::Value *);

/// Add metadata to a Value to indicate that it was identified as a load of a
/// vararg.
void setVarArgLoadArg(llvm::Value *);

/// Add metadata to a Value to indicate that it was introduced by the pointer
/// deobfuscation transformation.
void setPointerDeobfuscation(llvm::Value *);

/// Add metadata with the given \p id as value.
void setAccessID(llvm::Instruction *, uint64_t id);

/// Check a Value for metadata to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(const llvm::GlobalObject *);

/// Check a Value for metadata to indicate that it was introduced by the
/// instrumentation and should therefore be ignored in further instrumentation
/// steps.
bool hasNoInstrument(const llvm::Instruction *);

/// Check an Instruction for metadata that indicates that this instruction was
/// identified as vararg handling.
bool hasVarArgHandling(const llvm::Instruction *);

/// Check an Instruction for metadata that indicates that this instruction loads
/// an actual vararg.
bool hasVarArgLoadArg(const llvm::Instruction *);

/// Check if the given instruction has an ID.
bool hasAccessID(llvm::Instruction *);

/// Extract the access ID from the given instruction.
uint64_t getAccessID(llvm::Instruction *);

/// Check whether V has type pointer to something with a size or pointer to a
/// function.
bool hasPointerAccessSize(const llvm::Value *);

/// Get the size of an access to the pointer Value V.
size_t getPointerAccessSize(const llvm::DataLayout &, llvm::Value *);

/// Determines whether the given value can hold metadata.
bool canHoldMetadata(const llvm::Value *);

/// Determine whether the given type is or points to the llvm type for a va_list
bool isVarArgMetadataType(const llvm::Type *);

/// If the function has varargs or a va_list as argument, return the values
/// through which the varargs can be accessed.
/// For non-vararg function or declarations, the returned list is empty.
llvm::SmallVector<llvm::Value *, 2> getVarArgHandles(llvm::Function &);

} // namespace meminstrument

#endif
