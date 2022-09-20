//===- meminstrument/Config.h - Instrumentation configuration ---*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
///
/// \file
/// This file implements the configuration mechanism of the meminstrument
/// instrumentation passes.
///
/// Configurable parameters are accessible in the code via a GlobalConfig
/// object that can be obtained via the static GlobalConfig::create(...) method.
/// Only one of these should be created and passed to the relevant places.
///
/// The values that are stored in this object are determined as follows:
/// They default to the values from one of the here defined child classes of
/// Config. Which child class is used can be specified via the -mi-config
/// command line flag.
/// The default values from the basic configuration can be overridden via
/// separate command line flags.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_CONFIG_H
#define MEMINSTRUMENT_CONFIG_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

namespace meminstrument {

class Config;

enum class MIMode {
  NOTHING,
  SETUP,
  GATHER_ITARGETS,
  FILTER_ITARGETS,
  GENERATE_WITNESSES,
  GENERATE_INVARIANTS,
  GENERATE_OPTIMIZATION_CHECKS,
  GENERATE_CHECKS,
  DEFAULT,
};

/// Class for the actual configuration items in use.
/// On a normal run, only one of these should be created via a call to the
/// static GlobalConfig::get() method.
class GlobalConfig {
public:
  InstrumentationPolicy &getInstrumentationPolicy(void) {
    assert(instrumentationPolicy && "InstrumentationPolicy not set!");
    return *instrumentationPolicy;
  }

  InstrumentationMechanism &getInstrumentationMechanism(void) {
    assert(instrumentationMechanism && "InstrumentationMechanism not set!");
    return *instrumentationMechanism;
  }

  WitnessStrategy &getWitnessStrategy(void) {
    assert(witnessStrategy && "WitnessStrategy not set!");
    return *witnessStrategy;
  }

  MIMode getMIMode(void) { return mode; }

  bool hasPrintWitnessGraph(void) { return printWitnessGraph; }

  bool hasInstrumentVerbose(void) { return instrumentVerbose; }

  static std::unique_ptr<GlobalConfig> create(const llvm::Module &);

  void dump(llvm::raw_ostream &) const;

  ~GlobalConfig(void) {
    delete instrumentationMechanism;
    delete instrumentationPolicy;
    delete witnessStrategy;
  }

  GlobalConfig(const GlobalConfig &) = delete;

  GlobalConfig &operator=(const GlobalConfig &) = delete;

private:
  GlobalConfig(Config *, const llvm::Module &);

  InstrumentationMechanism *instrumentationMechanism = nullptr;
  InstrumentationPolicy *instrumentationPolicy = nullptr;
  WitnessStrategy *witnessStrategy = nullptr;

  MIMode mode = MIMode::DEFAULT;

  bool printWitnessGraph = false;
  bool instrumentVerbose = false;

  const char *configName;
};

} // namespace meminstrument
#endif
