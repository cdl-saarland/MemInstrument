//===- RunTimePrototypes.cpp - SoftBound Internal Funs Prototype Inserter -===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Injects all prototypes for internal check functions etc. into the module.
/// It additionally provides types to be used when generating calls to any of
/// the functions, such that changes in the signatures can be hidden to some
/// extend.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/softbound/RunTimePrototypes.h"

#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"
#include "meminstrument/instrumentation_mechanisms/softbound/RunTimeHandles.h"

#include <limits>

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

//===----------------------------------------------------------------------===//
//                   Implementation of PrototypeInserter
//===----------------------------------------------------------------------===//

PrototypeInserter::PrototypeInserter(Module &module)
    : module(module), context(module.getContext()) {

  // TODO find out if the native width is somehow accessible via DL/TT
  // The current solution will not properly work in a cross-compilation use
  // case.
  // One option could be to use `getLargestLegalIntType` or
  // `getSmallestLegalIntType` that the DataLayout provides, but none of them
  // corresponds to the type generated when translating the C run-time library
  // on my test machine.
  intTy = IntegerType::getIntNTy(context, sizeof(int) * CHAR_BIT);
  sizeTTy = IntegerType::getIntNTy(context, sizeof(size_t) * CHAR_BIT);

  voidTy = Type::getVoidTy(context);
  voidPtrTy = Type::getInt8PtrTy(context);
  baseTy = voidPtrTy;
  boundTy = voidPtrTy;
  basePtrTy = PointerType::getUnqual(baseTy);
  boundPtrTy = PointerType::getUnqual(boundTy);
  varArgProxyTy = PointerType::getUnqual(PointerType::getUnqual(sizeTTy));
  varArgProxyPtrTy = PointerType::getUnqual(varArgProxyTy);
}

auto PrototypeInserter::insertRunTimeProtoypes() const -> RunTimeHandles {

  RunTimeHandles handles;

  handles.baseTy = baseTy;
  handles.boundTy = boundTy;
  handles.voidPtrTy = voidPtrTy;

  handles.intTy = intTy;
  handles.sizeTTy = sizeTTy;

  handles.varArgProxyTy = varArgProxyTy;
  handles.varArgProxyPtrTy = varArgProxyPtrTy;

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
  insertFailAndStatsFunctions(handles);

  return handles;
}

//===---------------------------- private ---------------------------------===//

void PrototypeInserter::insertSpatialOnlyRunTimeProtoypes(
    RunTimeHandles &handles) const {

  handles.loadInMemoryPtrInfo =
      createAndInsertPrototype("__softboundcets_metadata_load", voidTy,
                               voidPtrTy, basePtrTy, boundPtrTy);
  handles.storeInMemoryPtrInfo = createAndInsertPrototype(
      "__softboundcets_metadata_store", voidTy, voidPtrTy, baseTy, boundTy);
}

void PrototypeInserter::insertSpatialRunTimeProtoypes(
    RunTimeHandles &handles) const {

  // Shadow stack operations
  handles.loadBaseStack = createAndInsertPrototype(
      "__softboundcets_load_base_shadow_stack", baseTy, intTy);
  handles.loadBoundStack = createAndInsertPrototype(
      "__softboundcets_load_bound_shadow_stack", boundTy, intTy);
  handles.storeBaseStack = createAndInsertPrototype(
      "__softboundcets_store_base_shadow_stack", voidTy, baseTy, intTy);
  handles.storeBoundStack = createAndInsertPrototype(
      "__softboundcets_store_bound_shadow_stack", voidTy, boundTy, intTy);

  // Check functions
  handles.spatialCallCheck =
      createAndInsertPrototype("__softboundcets_spatial_call_dereference_check",
                               voidTy, baseTy, boundTy, voidPtrTy);
  handles.spatialCheck =
      createAndInsertPrototype("__softboundcets_spatial_dereference_check",
                               voidTy, baseTy, boundTy, voidPtrTy, sizeTTy);

  // VarArg related
  handles.loadNextInfoVarArgProxy =
      createAndInsertPrototype("__softboundcets_next_va_arg_metadata", voidTy,
                               varArgProxyTy, basePtrTy, boundPtrTy);
}

void PrototypeInserter::insertTemporalRunTimeProtoypes(
    RunTimeHandles &handles) const {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertTemporalOnlyRunTimeProtoypes(
    RunTimeHandles &handles) const {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertFullSafetyRunTimeProtoypes(
    RunTimeHandles &handles) const {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertSetupFunctions(RunTimeHandles &handles) const {

  handles.init = createAndInsertPrototype("__softboundcets_init", voidTy);
}

void PrototypeInserter::insertCommonFunctions(RunTimeHandles &handles) const {

  handles.allocateShadowStack = createAndInsertPrototype(
      "__softboundcets_allocate_shadow_stack_space", voidTy, intTy);
  handles.deallocateShadowStack = createAndInsertPrototype(
      "__softboundcets_deallocate_shadow_stack_space", voidTy);
  handles.copyInMemoryMetadata = createAndInsertPrototype(
      "__softboundcets_copy_metadata", voidTy, voidPtrTy, voidPtrTy, sizeTTy);

  // VarArg related
  handles.allocateVarArgProxy = createAndInsertPrototype(
      "__softboundcets_allocate_va_arg_proxy", varArgProxyTy, intTy);

  handles.copyVarArgProxy = createAndInsertPrototype(
      "__softboundcets_copy_va_arg_proxy", varArgProxyTy, varArgProxyTy);

  handles.freeVarArgProxy = createAndInsertPrototype(
      "__softboundcets_free_va_arg_proxy", voidTy, varArgProxyTy);

  handles.loadVarArgProxyStack = createAndInsertPrototype(
      "__softboundcets_load_proxy_shadow_stack", varArgProxyTy, intTy);

  handles.storeVarArgProxyStack = createAndInsertPrototype(
      "__softboundcets_store_proxy_shadow_stack", voidTy, varArgProxyTy, intTy);

  handles.loadInMemoryProxyPtrInfo =
      createAndInsertPrototype("__softboundcets_proxy_metadata_load", voidTy,
                               voidPtrTy, varArgProxyPtrTy);

  handles.storeInMemoryProxyPtrInfo = createAndInsertPrototype(
      "__softboundcets_proxy_metadata_store", voidTy, voidPtrTy, varArgProxyTy);
}

void PrototypeInserter::insertFailAndStatsFunctions(
    RunTimeHandles &handles) const {

  if (InternalSoftBoundConfig::hasRunTimeStatsEnabled()) {
    handles.externalCheckCounter =
        createAndInsertPrototype("__rt_stat_inc_external_check", voidTy);
  }

  handles.failFunction =
      createAndInsertPrototype("__softboundcets_abort", voidTy);
}

template <typename... ArgsTy>
auto PrototypeInserter::createAndInsertPrototype(const StringRef &name,
                                                 Type *retType,
                                                 ArgsTy... args) const
    -> Function * {

  // Insert the function prototype into the module
  Function *fun = dyn_cast<Function>(
      module.getOrInsertFunction(name, retType, args...).getCallee());

  // Mark the prototypes we inserted for easy recognition
  MDNode *node = MDNode::get(context, MDString::get(context, "RTPrototype"));
  fun->setMetadata(InternalSoftBoundConfig::getMetadataKind(), node);

  return fun;
}
