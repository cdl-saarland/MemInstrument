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

private:
  static const SafetyLevel level;

  static auto constexpr initialize() -> SafetyLevel;
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_SOFTBOUNDCONFIG_H
