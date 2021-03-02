//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_CONFIG_H
#define MEMINSTRUMENT_CONFIG_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

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

namespace meminstrument {

class Config;

enum class MIMode {
  NOTHING,
  SETUP,
  GATHER_ITARGETS,
  FILTER_ITARGETS,
  GENERATE_WITNESSES,
  GENERATE_EXTERNAL_CHECKS,
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

  bool hasUseFilters(void) { return useFilters; }

  bool hasUseExternalChecks(void) { return useExternalChecks; }

  bool hasPrintWitnessGraph(void) { return printWitnessGraph; }

  bool hasSimplifyWitnessGraph(void) { return simplifyWitnessGraph; }

  bool hasInstrumentVerbose(void) { return instrumentVerbose; }

  uint32_t getNoopGenBoundsTime(void);
  uint32_t getNoopDerefCheckTime(void);
  uint32_t getNoopInvarCheckTime(void);
  uint32_t getNoopStackAllocTime(void);
  uint32_t getNoopHeapAllocTime(void);
  uint32_t getNoopGlobalAllocTime(void);
  uint32_t getNoopHeapFreeTime(void);

  bool hasUseNoop(void);

  static std::unique_ptr<GlobalConfig> create(const llvm::Module &M);

  void dump(llvm::raw_ostream &Stream) const;

  ~GlobalConfig(void) {
    delete instrumentationMechanism;
    delete instrumentationPolicy;
    delete witnessStrategy;
  }

  void noteError(void);

  bool hasErrors(void) const;

  GlobalConfig(const GlobalConfig &) = delete;

  GlobalConfig &operator=(const GlobalConfig &) = delete;

private:
  GlobalConfig(Config *Cfg, const llvm::Module &M);

  InstrumentationMechanism *instrumentationMechanism = nullptr;
  InstrumentationPolicy *instrumentationPolicy = nullptr;
  WitnessStrategy *witnessStrategy = nullptr;

  MIMode mode = MIMode::DEFAULT;

  bool useFilters = false;
  bool useExternalChecks = false;
  bool printWitnessGraph = false;
  bool simplifyWitnessGraph = false;
  bool instrumentVerbose = false;

  uint64_t numErrors = 0;

  const char *configName;
};

} // namespace meminstrument
#endif
