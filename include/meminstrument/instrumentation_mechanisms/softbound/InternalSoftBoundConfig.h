//===--- meminstrument/InternalSoftBoundConfig.h - SoftBound ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Configuration of SoftBound
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H

#include <string>

namespace meminstrument {

namespace softbound {

enum SafetyLevel { SPATIAL_ONLY, TEMPORAL_ONLY, FULL_SAFETY };

class InternalSoftBoundConfig {

public:
  static bool ensureSpatial();

  static bool ensureOnlySpatial();

  static bool ensureTemporal();

  static bool ensureOnlyTemporal();

  static bool ensureFullSafety();

  static bool hasRunTimeStatsEnabled();

  /// Definitions of string for metadata nodes used to annotate the generated IR
  static auto getMetadataKind() -> std::string;
  static auto getShadowStackInfoStr() -> std::string;
  static auto getShadowStackLoadStr() -> std::string;
  static auto getShadowStackStoreStr() -> std::string;
  static auto getMetadataInfoStr() -> std::string;
  static auto getSetupInfoStr() -> std::string;

private:
  static const SafetyLevel level;
  static const bool runTimeStatsEnabled;

  static auto constexpr initialize() -> SafetyLevel;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H
