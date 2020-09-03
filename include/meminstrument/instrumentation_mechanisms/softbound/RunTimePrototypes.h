//===- meminstrument/RunTimeProtoypes.h - SoftBound Prototypes --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// The prototype inserter injects run-time library prototypes into LLVM
/// modules. Calls to check functions, metadata propagation functions,
/// initialization functions etc. will be inserted during compilation, the
/// prototypes ensure that the module compiles although there is no definition
/// of the function. The definitions are in the C run-time library, which is
/// build stand-alone and linked later.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H

#include "llvm/IR/Module.h"

namespace meminstrument {

namespace softbound {

class RunTimeHandles;

class PrototypeInserter {

public:
  PrototypeInserter(llvm::Module &);

  auto insertRunTimeProtoypes() -> RunTimeHandles;

private:
  llvm::Module &module;
  llvm::LLVMContext &context;

  llvm::Type *intTy = nullptr;
  llvm::Type *sizeTTy = nullptr;
  llvm::Type *voidTy = nullptr;
  llvm::PointerType *voidPtrTy = nullptr;
  llvm::PointerType *baseTy = nullptr;
  llvm::PointerType *boundTy = nullptr;
  llvm::PointerType *basePtrTy = nullptr;
  llvm::PointerType *boundPtrTy = nullptr;

  void insertSpatialRunTimeProtoypes(RunTimeHandles &);

  void insertSpatialOnlyRunTimeProtoypes(RunTimeHandles &);

  void insertTemporalRunTimeProtoypes(RunTimeHandles &);

  void insertTemporalOnlyRunTimeProtoypes(RunTimeHandles &);

  void insertFullSafetyRunTimeProtoypes(RunTimeHandles &);

  void insertSetupFunctions(RunTimeHandles &);

  void insertCommonFunctions(RunTimeHandles &);

  template <typename... ArgsTy>
  auto createAndInsertPrototype(const llvm::StringRef &name,
                                llvm::Type *retType, ArgsTy... args)
      -> llvm::Function *;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H
