//===- InternalSoftBoundConfig.cpp - C run-time compliant configuration ---===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The C run-time can be configured to ensure spatial safety, temporal safety
/// or both. Align to the configuration that the run-time uses (otherwise the
/// compiled program will not work). Provide some convenience functions to be
/// independent of the defines in the rest of the code.
/// This configuration encapsulates information that the C run-time communicates
/// to the pass and makes them available independent from the details of the C
/// run-time.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"

#include "meminstrument-rt/SBRTInfo.h"
#include "meminstrument-rt/SBWrapper.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

//===----------------------------------------------------------------------===//
//                 Implementation of InternalSoftBoundConfig
//===----------------------------------------------------------------------===//

bool InternalSoftBoundConfig::ensureSpatial() {
  return level == SPATIAL_ONLY || level == FULL_SAFETY;
}

bool InternalSoftBoundConfig::ensureOnlySpatial() {
  return level == SPATIAL_ONLY;
}

bool InternalSoftBoundConfig::ensureTemporal() {
  return level == TEMPORAL_ONLY || level == FULL_SAFETY;
}

bool InternalSoftBoundConfig::ensureOnlyTemporal() {
  return level == TEMPORAL_ONLY;
}

bool InternalSoftBoundConfig::ensureFullSafety() {
  return level == FULL_SAFETY;
}

bool InternalSoftBoundConfig::hasRunTimeStatsEnabled() {
  return runTimeStatsEnabled;
}

auto InternalSoftBoundConfig::getWrappedName(const StringRef funName)
    -> std::string {
  auto wrappedName = getWrapperPrefix() + funName.str();
  if (std::find(availableWrappers.begin(), availableWrappers.end(),
                wrappedName) != availableWrappers.end()) {
    return wrappedName;
  }
  return funName.str();
}

bool InternalSoftBoundConfig::isWrappedName(const StringRef funName) {
  return funName.startswith(getWrapperPrefix());
}

auto InternalSoftBoundConfig::getMetadataKind() -> std::string {
  return "SoftBound";
}

auto InternalSoftBoundConfig::getShadowStackInfoStr() -> std::string {
  return "ShadowStack";
}

auto InternalSoftBoundConfig::getShadowStackLoadStr() -> std::string {
  return getShadowStackInfoStr() + "_Load";
}

auto InternalSoftBoundConfig::getShadowStackStoreStr() -> std::string {
  return getShadowStackInfoStr() + "_Store";
}

auto InternalSoftBoundConfig::getMetadataInfoStr() -> std::string {
  return "Metadata";
}

auto InternalSoftBoundConfig::getSetupInfoStr() -> std::string {
  return "Setup";
}

auto InternalSoftBoundConfig::getCheckInfoStr() -> std::string {
  return "Check";
}

void InternalSoftBoundConfig::setSoftBoundMetadata(GlobalObject *glObj,
                                                   const StringRef str) {
  auto &context = glObj->getContext();
  auto mdKind = InternalSoftBoundConfig::getMetadataKind();
  MDNode *node = MDNode::get(context, MDString::get(context, str));
  node = MDNode::concatenate(glObj->getMetadata(mdKind), node);
  glObj->setMetadata(mdKind, node);
}

void InternalSoftBoundConfig::setSoftBoundMetadata(Instruction *inst,
                                                   const StringRef str) {
  auto &context = inst->getContext();
  auto mdKind = InternalSoftBoundConfig::getMetadataKind();
  MDNode *node = MDNode::get(context, MDString::get(context, str));
  node = MDNode::concatenate(inst->getMetadata(mdKind), node);
  inst->setMetadata(mdKind, node);
}

//===---------------------------- private ---------------------------------===//

const SafetyLevel InternalSoftBoundConfig::level =
    InternalSoftBoundConfig::initialize();

#if ENABLE_RT_STATS
const bool InternalSoftBoundConfig::runTimeStatsEnabled = true;
#else
const bool InternalSoftBoundConfig::runTimeStatsEnabled = false;
#endif

auto constexpr InternalSoftBoundConfig::initialize() -> SafetyLevel {

  static_assert(
      __SOFTBOUNDCETS_SPATIAL || __SOFTBOUNDCETS_TEMPORAL ||
          __SOFTBOUNDCETS_SPATIAL_TEMPORAL,
      "Invalid configuration (one of spatial/temporal/spatial+temporal must be "
      "true). If you modified SBRTInfo.h manually, please revert the "
      "changes (to change the safety guarantees, configure the C run-time "
      "appropriately).");

  static_assert(
      (__SOFTBOUNDCETS_SPATIAL ^ __SOFTBOUNDCETS_TEMPORAL ^
       __SOFTBOUNDCETS_SPATIAL_TEMPORAL) &&
          !(__SOFTBOUNDCETS_SPATIAL && __SOFTBOUNDCETS_TEMPORAL &&
            __SOFTBOUNDCETS_SPATIAL_TEMPORAL),
      "Invalid configuration (only exactly one of "
      "spatial/temporal/spatial+temporal can be true). If you modified "
      "SBRTInfo.h manually, please revert the changes (to change the safety "
      "guarantees, configure the C run-time appropriately).");

#if __SOFTBOUNDCETS_SPATIAL
  return SafetyLevel::SPATIAL_ONLY;
#endif
#if __SOFTBOUNDCETS_TEMPORAL
  return SafetyLevel::TEMPORAL_ONLY;
#endif
#if __SOFTBOUNDCETS_SPATIAL_TEMPORAL
  return SafetyLevel::FULL_SAFETY;
#endif
}

auto InternalSoftBoundConfig::getWrapperPrefix() -> std::string {
  return "softboundcets_";
}
