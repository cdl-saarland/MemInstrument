//===--- InternalSoftBoundConfig.cpp - C run-time compliant configuration -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
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

#include "meminstrument/softbound/SBRTInfo.h"
#include "meminstrument/softbound/SBWrapper.h"

#include "llvm/ADT/StringRef.h"

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
  return funName;
}

bool InternalSoftBoundConfig::isWrappedName(const llvm::StringRef funName) {
  return funName.startswith(getWrapperPrefix());
}

auto InternalSoftBoundConfig::getMetadataKind() -> std::string {
  return "SoftBound";
}

auto InternalSoftBoundConfig::getShadowStackInfoStr() -> std::string {
  return "ShadowStack";
}

auto InternalSoftBoundConfig::getShadowStackLoadStr() -> std::string {
  return getShadowStackInfoStr() + "_load";
}

auto InternalSoftBoundConfig::getShadowStackStoreStr() -> std::string {
  return getShadowStackInfoStr() + "_store";
}

auto InternalSoftBoundConfig::getMetadataInfoStr() -> std::string {
  return "Metadata";
}

auto InternalSoftBoundConfig::getSetupInfoStr() -> std::string {
  return "Setup";
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
