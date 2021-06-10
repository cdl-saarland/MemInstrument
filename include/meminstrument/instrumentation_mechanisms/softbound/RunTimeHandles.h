//===- meminstrument/RunTimeHandles.h - Handles for C functions -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
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
  llvm::Function *init = nullptr;

  // Check functions

  //... for spatial safety
  llvm::Function *spatialCallCheck = nullptr;
  llvm::Function *spatialCheck = nullptr;

  //... for temporal safety [not implemented]

  // Shadow stack operations
  llvm::Function *allocateShadowStack = nullptr;
  llvm::Function *deallocateShadowStack = nullptr;

  //... for spatial safety
  llvm::Function *loadBaseStack = nullptr;
  llvm::Function *loadBoundStack = nullptr;
  llvm::Function *storeBaseStack = nullptr;
  llvm::Function *storeBoundStack = nullptr;

  //... for temporal safety [not implemented]

  // In memory pointer information gathering/propagation
  llvm::Function *loadInMemoryPtrInfo = nullptr;
  llvm::Function *storeInMemoryPtrInfo = nullptr;
  llvm::Function *copyInMemoryMetadata = nullptr;

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

  llvm::Function *allocateVarArgProxy;
  llvm::Function *loadNextInfoVarArgProxy;
  llvm::Function *copyVarArgProxy;
  llvm::Function *freeVarArgProxy;
  llvm::Function *loadVarArgProxyStack;
  llvm::Function *storeVarArgProxyStack;

  // Fail functions and statistics
  llvm::Function *externalCheckCounter = nullptr;
  llvm::Function *failFunction = nullptr;

  // Highest valid pointer value
  uintptr_t highestAddr;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEHANDLES_H
