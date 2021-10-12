//===- meminstrument/RunTimeHandles.h - Handles for C functions -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This class abstracts from the names of the C run-time functions and contains
/// handles to all available internal metadata/check/etc. functions, such that
/// calls to them can easily be inserted.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEHANDLES_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEHANDLES_H

#include "llvm/IR/Function.h"

namespace meminstrument {

namespace softbound {

class RunTimeHandles {
public:
  RunTimeHandles() = default;

  // Init function
  llvm::FunctionCallee init = nullptr;

  // Check functions

  //... for spatial safety
  llvm::FunctionCallee spatialCallCheck = nullptr;
  llvm::FunctionCallee spatialCheck = nullptr;

  //... for temporal safety [not implemented]

  // Shadow stack operations
  llvm::FunctionCallee allocateShadowStack = nullptr;
  llvm::FunctionCallee deallocateShadowStack = nullptr;

  //... for spatial safety
  llvm::FunctionCallee loadBaseStack = nullptr;
  llvm::FunctionCallee loadBoundStack = nullptr;
  llvm::FunctionCallee storeBaseStack = nullptr;
  llvm::FunctionCallee storeBoundStack = nullptr;

  //... for temporal safety [not implemented]

  // In memory pointer information gathering/propagation
  llvm::FunctionCallee loadInMemoryPtrInfo = nullptr;
  llvm::FunctionCallee storeInMemoryPtrInfo = nullptr;
  llvm::FunctionCallee copyInMemoryMetadata = nullptr;

  // Argument types
  llvm::PointerType *baseTy = nullptr;
  llvm::PointerType *boundTy = nullptr;
  llvm::PointerType *voidPtrTy = nullptr;

  llvm::Type *intTy = nullptr;
  llvm::Type *sizeTTy = nullptr;

  // VarArg handling functions
  // Type of the proxy object that allows to load bounds for varargs
  llvm::PointerType *varArgProxyTy = nullptr;
  llvm::PointerType *varArgProxyPtrTy = nullptr;

  llvm::FunctionCallee allocateVarArgProxy = nullptr;
  llvm::FunctionCallee loadNextInfoVarArgProxy = nullptr;
  llvm::FunctionCallee copyVarArgProxy = nullptr;
  llvm::FunctionCallee freeVarArgProxy = nullptr;
  llvm::FunctionCallee loadVarArgProxyStack = nullptr;
  llvm::FunctionCallee storeVarArgProxyStack = nullptr;
  llvm::FunctionCallee loadInMemoryProxyPtrInfo = nullptr;
  llvm::FunctionCallee storeInMemoryProxyPtrInfo = nullptr;

  // Fail functions and statistics
  llvm::FunctionCallee externalCheckCounter = nullptr;

  // Highest valid pointer value
  uintptr_t highestAddr;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEHANDLES_H
