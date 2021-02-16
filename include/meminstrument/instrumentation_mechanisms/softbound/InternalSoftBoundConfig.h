//===--- meminstrument/InternalSoftBoundConfig.h - SoftBound ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Configuration of SoftBound derived from the C run-time library.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H

#include <string>

namespace llvm {
class StringRef;
}

namespace meminstrument {

namespace softbound {

/// The pass can either ensure spatial memory safety, temporal memory safety or
/// both. Use the SafetyLevel to track this state.
enum SafetyLevel { SPATIAL_ONLY, TEMPORAL_ONLY, FULL_SAFETY };

class InternalSoftBoundConfig {

public:
  /// Returns true if the run-time is configured to ensure spatial safety.
  static bool ensureSpatial();

  /// Returns true if the run-time is configured to ensure only spatial safety.
  static bool ensureOnlySpatial();

  /// Returns true if the run-time is configured to ensure temporal safety.
  static bool ensureTemporal();

  /// Returns true if the run-time is configured to ensure only temporal safety.
  static bool ensureOnlyTemporal();

  /// Returns true if the run-time is configured to ensure spatial and temporal
  /// safety.
  static bool ensureFullSafety();

  /// Returns true iff the run-time is configured to track run-time statistics.
  static bool hasRunTimeStatsEnabled();

  /// Get the name of the function wrapper for the given function name if
  /// available. Return the unmodified name otherwise.
  static auto getWrappedName(const llvm::StringRef funName) -> std::string;

  /// Returns true iff the given name is the name of a wrapper
  static bool isWrappedName(const llvm::StringRef funName);

  /// Definitions of metadata strings used to annotate the generated IR
  static auto getMetadataKind() -> std::string;
  static auto getShadowStackInfoStr() -> std::string;
  static auto getShadowStackLoadStr() -> std::string;
  static auto getShadowStackStoreStr() -> std::string;
  static auto getMetadataInfoStr() -> std::string;
  static auto getSetupInfoStr() -> std::string;

private:
  static const SafetyLevel level;
  static const bool runTimeStatsEnabled;

  /// Use the information given by the run-time to determine the safety level.
  static auto constexpr initialize() -> SafetyLevel;

  /// The prefix used in the C run-time library to mark wrapped standard library
  /// functions.
  static auto getWrapperPrefix() -> std::string;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H
