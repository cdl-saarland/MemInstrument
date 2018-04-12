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

/// TODO Out of date!
/// This file implements the configuration mechanism of the meminstrument
/// instrumentation passes.
///
/// All configurable parameters are accessible in the code via a GlobalConfig
/// object that can be obtained via the static GlobalConfig::get(...) method.
///
/// The values that are stored in this object are determined as follows:
/// They default to the values from one of the here defined child classes of
/// Config. Which child class is used can be specified via the -mi-config
/// command line flag or via the MI_CONFIG environment variable (if both are
/// given, the cli takes precedence over the environment variable).
///
/// The default values from the basic configuration can be overridden via
/// separate command line flags.

namespace meminstrument {

class Config;

enum class MIMode {
  SETUP,
  GATHER_ITARGETS,
  FILTER_ITARGETS,
  GENERATE_WITNESSES,
  GENERATE_EXTERNAL_CHECKS,
  GENERATE_CHECKS,
  DEFAULT,
};


/// TODO Out of date!
/// Class for the actual configuration items in use.
/// On a normal run, only one of these should be created in the first call of
/// the static GlobalConfig::get() method, following calls just provide the
/// same GlobalConfig.
class GlobalConfig {
public:
  InstrumentationPolicy &getInstrumentationPolicy(void) {
    assert(_IP && "InstrumentationPolicy not set!");
    return *_IP;
  }

  InstrumentationMechanism &getInstrumentationMechanism(void) {
    assert(_IM && "InstrumentationMechanism not set!");
    return *_IM;
  }

  WitnessStrategy &getWitnessStrategy(void) {
    assert(_WS && "WitnessStrategy not set!");
    return *_WS;
  }

  MIMode getMIMode(void) { return _MIMode; }

  bool hasUseFilters(void) { return _UseFilters; }

  bool hasUseExternalChecks(void) { return _UseExternalChecks; }

  bool hasPrintWitnessGraph(void) { return _PrintWitnessGraph; }

  bool hasSimplifyWitnessGraph(void) { return _SimplifyWitnessGraph; }

  bool hasInstrumentVerbose(void) { return _InstrumentVerbose; }

  static std::unique_ptr<GlobalConfig> get(const llvm::Module &M);

  void dump(llvm::raw_ostream &Stream) const;

  ~GlobalConfig(void) {
    delete _IM;
    delete _IP;
    delete _WS;
  }

  void noteError(void);

  bool hasErrors(void) const;

  GlobalConfig(const GlobalConfig &) = delete;

  GlobalConfig& operator=(const GlobalConfig &) = delete;

private:
  GlobalConfig(Config *Cfg, const llvm::Module &M);

  InstrumentationMechanism *_IM = nullptr;
  InstrumentationPolicy *_IP = nullptr;
  WitnessStrategy *_WS = nullptr;

  MIMode _MIMode = MIMode::DEFAULT;

  bool _UseFilters = false;
  bool _UseExternalChecks = false;
  bool _PrintWitnessGraph = false;
  bool _SimplifyWitnessGraph = false;
  bool _InstrumentVerbose = false;

  uint64_t _numErrors = 0;

  const char *_ConfigName;
};

} // namespace meminstrument
#endif
