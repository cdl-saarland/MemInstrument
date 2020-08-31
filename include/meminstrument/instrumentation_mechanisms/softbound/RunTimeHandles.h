//===--------- meminstrument/RunTimeHandles.h - TODO -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// TODO
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

  //... for the C memcpy/memset functions
  llvm::Function *memcpyCheck = nullptr;
  llvm::Function *memsetCheck = nullptr;

  //... for spatial safety
  llvm::Function *spatialCallCheck = nullptr;
  llvm::Function *spatialLoadCheck = nullptr;
  llvm::Function *spatialStoreCheck = nullptr;

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
  llvm::Type *baseTy = nullptr;
  llvm::Type *boundTy = nullptr;
  llvm::Type *voidPtrTy = nullptr;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEHANDLES_H
