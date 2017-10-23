//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_CONFIG_H
#define MEMINSTRUMENT_CONFIG_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/witness_strategies/WitnessStrategy.h"

class Config {
public:
  enum class MIMode {
    SETUP,
    GATHER_ITARGETS,
    FILTER_ITARGETS,
    GENERATE_WITNESSES,
    GENERATE_EXTERNAL_CHECKS,
    GENERATE_CHECKS,
  };

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

  MIMode getMIMode(void) {
    return _MIMode;
  }

  bool hasUseFilters(void) {
    return _UseFilters;
  }

  bool hasUseExternalChecks(void) {
    return _UseExternalChecks;
  }

  bool hasPrintWitnessGraph(void) {
    return _PrintWitnessGraph;
  }

  bool hasSimplifyWitnessGraph(void) {
    return _SimplifyWitnessGraph;
  }

  bool hasInstrumentVerbose(void) {
    return _InstrumentVerbose;
  }


  static Config &get(void);

private:
  InstrumentationMechanism *_IM = nullptr;
  InstrumentationPolicy *_IP = nullptr;
  WitnessStrategy *_WS = nullptr;

  MIMode _MIMode = MIMode::GENERATE_CHECKS;

  bool _UseFilters = false;
  bool _UseExternalChecks = false;
  bool _PrintWitnessGraph = false;
  bool _SimplifyWitnessGraph = false;
  bool _InstrumentVerbose = false;
};

#endif
