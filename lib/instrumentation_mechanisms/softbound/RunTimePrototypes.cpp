//===--------------------- RunTimePrototypes.cpp - TODO -------------------===//
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

#include "meminstrument/instrumentation_mechanisms/softbound/RunTimePrototypes.h"

#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"
#include "meminstrument/instrumentation_mechanisms/softbound/RunTimeHandles.h"

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

//===----------------------------------------------------------------------===//
//                   Implementation of PrototypeInserter
//===----------------------------------------------------------------------===//

PrototypeInserter::PrototypeInserter(Module &module)
    : module(module), context(module.getContext()) {

  // TODO find out if the native width is somehow accessible via DL/TT
  // The current solution will not properly work for cross-compilation
  auto byteSize = 8;
  intTy = IntegerType::getIntNTy(context, sizeof(int) * byteSize);
  sizeTTy = IntegerType::getIntNTy(context, sizeof(size_t) * byteSize);

  voidTy = Type::getVoidTy(context);
  voidPtrTy = Type::getInt8PtrTy(context);
  baseTy = voidPtrTy;
  boundTy = voidPtrTy;
  basePtrTy = PointerType::getUnqual(voidPtrTy);
  boundPtrTy = basePtrTy;
}

auto PrototypeInserter::insertRunTimeProtoypes() -> RunTimeHandles {

  RunTimeHandles handles;

  if (InternalSoftBoundConfig::ensureOnlySpatial()) {
    insertSpatialOnlyRunTimeProtoypes(handles);
    insertSpatialRunTimeProtoypes(handles);
  }

  if (InternalSoftBoundConfig::ensureOnlyTemporal()) {
    insertTemporalOnlyRunTimeProtoypes(handles);
    insertTemporalRunTimeProtoypes(handles);
  }

  if (InternalSoftBoundConfig::ensureFullSafety()) {
    insertSpatialRunTimeProtoypes(handles);
    insertTemporalRunTimeProtoypes(handles);
    insertFullSafetyRunTimeProtoypes(handles);
  }

  insertSetupFunctions(handles);
  insertCommonFunctions(handles);

  return handles;
}

//===---------------------------- private -------------------------------===//

void PrototypeInserter::insertSpatialOnlyRunTimeProtoypes(
    RunTimeHandles &handles) {

  handles.memcpyCheck = createAndInsertPrototype(
      "__softboundcets_memcopy_check", voidTy, voidPtrTy, voidPtrTy, sizeTTy,
      baseTy, boundTy, baseTy, boundTy);
  handles.memsetCheck =
      createAndInsertPrototype("__softboundcets_memset_check", voidTy,
                               voidPtrTy, sizeTTy, baseTy, boundTy);
  handles.loadInMemoryPtrInfo =
      createAndInsertPrototype("__softboundcets_metadata_load", voidTy,
                               voidPtrTy, basePtrTy, boundPtrTy);
  handles.storeInMemoryPtrInfo = createAndInsertPrototype(
      "__softboundcets_metadata_store", voidTy, voidPtrTy, baseTy, boundTy);
}

void PrototypeInserter::insertSpatialRunTimeProtoypes(RunTimeHandles &handles) {

  // Shadow stack operations
  handles.loadBaseStack = createAndInsertPrototype(
      "__softboundcets_load_base_shadow_stack", voidTy, intTy);
  handles.loadBoundStack = createAndInsertPrototype(
      "__softboundcets_load_bound_shadow_stack", voidTy, intTy);
  handles.storeBaseStack = createAndInsertPrototype(
      "__softboundcets_store_base_shadow_stack", voidTy, baseTy, intTy);
  handles.storeBoundStack = createAndInsertPrototype(
      "__softboundcets_store_bound_shadow_stack", voidTy, boundTy, intTy);

  // Check functions
  handles.spatialCallCheck =
      createAndInsertPrototype("__softboundcets_spatial_call_dereference_check",
                               voidTy, baseTy, boundTy, voidPtrTy);
  handles.spatialLoadCheck =
      createAndInsertPrototype("__softboundcets_spatial_load_dereference_check",
                               voidTy, baseTy, boundTy, voidPtrTy, sizeTTy);
  handles.spatialStoreCheck = createAndInsertPrototype(
      "__softboundcets_spatial_store_dereference_check", voidTy, baseTy,
      boundTy, voidPtrTy, sizeTTy);
}

void PrototypeInserter::insertTemporalRunTimeProtoypes(
    RunTimeHandles &handles) {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertTemporalOnlyRunTimeProtoypes(
    RunTimeHandles &handles) {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertFullSafetyRunTimeProtoypes(
    RunTimeHandles &handles) {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertSetupFunctions(RunTimeHandles &handles) {

  handles.init = createAndInsertPrototype("__softboundcets_init", voidTy);
}

void PrototypeInserter::insertCommonFunctions(RunTimeHandles &handles) {

  handles.allocateShadowStack = createAndInsertPrototype(
      "__softboundcets_allocate_shadow_stack_space", voidTy, intTy);
  handles.deallocateShadowStack = createAndInsertPrototype(
      "__softboundcets_deallocate_shadow_stack_space", voidTy);
  handles.copyInMemoryMetadata = createAndInsertPrototype(
      "__softboundcets_copy_metadata", voidTy, voidPtrTy, voidPtrTy, sizeTTy);
}

template <typename... ArgsTy>
auto PrototypeInserter::createAndInsertPrototype(const StringRef &name,
                                                 Type *retType, ArgsTy... args)
    -> Function * {

  // Insert the function prototype into the module
  Function *fun = dyn_cast<Function>(
      module.getOrInsertFunction(name, retType, args...).getCallee());

  // Mark the prototypes we inserted for easy recognition
  MDNode *node = MDNode::get(context, MDString::get(context, "RTPrototype"));
  fun->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);

  return fun;
}
