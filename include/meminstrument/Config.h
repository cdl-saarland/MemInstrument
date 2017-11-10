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

namespace meminstrument {
class Config {
public:
  enum class MIMode {
    SETUP,
    GATHER_ITARGETS,
    FILTER_ITARGETS,
    GENERATE_WITNESSES,
    GENERATE_EXTERNAL_CHECKS,
    GENERATE_CHECKS,
    DEFAULT,
  };

  virtual ~Config(void) { }

  virtual InstrumentationPolicy *createInstrumentationPolicy(const llvm::DataLayout& DL) = 0;

  virtual InstrumentationMechanism *createInstrumentationMechanism(void) = 0;

  virtual WitnessStrategy *createWitnessStrategy(void) = 0;

  virtual MIMode getMIMode(void) = 0;

  virtual bool hasUseFilters(void) = 0;

  virtual bool hasUseExternalChecks(void) = 0;

  virtual bool hasPrintWitnessGraph(void) = 0;

  virtual bool hasSimplifyWitnessGraph(void) = 0;

  virtual bool hasInstrumentVerbose(void) = 0;
};


class SplayConfig : public Config {
public:
  virtual ~SplayConfig(void) { }

  virtual InstrumentationPolicy *createInstrumentationPolicy(const llvm::DataLayout& D) override;
  virtual InstrumentationMechanism *createInstrumentationMechanism(void) override;
  virtual WitnessStrategy *createWitnessStrategy(void) override;
  virtual MIMode getMIMode(void) override;
  virtual bool hasUseFilters(void) override;
  virtual bool hasUseExternalChecks(void) override;
  virtual bool hasPrintWitnessGraph(void) override;
  virtual bool hasSimplifyWitnessGraph(void) override;
  virtual bool hasInstrumentVerbose(void) override;
};

class RTStatConfig : public Config {
public:
  virtual ~RTStatConfig(void) { }

  virtual InstrumentationPolicy *createInstrumentationPolicy(const llvm::DataLayout& D) override;
  virtual InstrumentationMechanism *createInstrumentationMechanism(void) override;
  virtual WitnessStrategy *createWitnessStrategy(void) override;
  virtual MIMode getMIMode(void) override;
  virtual bool hasUseFilters(void) override;
  virtual bool hasUseExternalChecks(void) override;
  virtual bool hasPrintWitnessGraph(void) override;
  virtual bool hasSimplifyWitnessGraph(void) override;
  virtual bool hasInstrumentVerbose(void) override;
};

class GlobalConfig {
public:
  GlobalConfig(Config* Cfg, const llvm::Module& M);

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

  Config::MIMode getMIMode(void) {
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

  static GlobalConfig &get(const llvm::Module& M);

private:
  InstrumentationMechanism *_IM = nullptr;
  InstrumentationPolicy *_IP = nullptr;
  WitnessStrategy *_WS = nullptr;

  Config::MIMode _MIMode = Config::MIMode::DEFAULT;

  bool _UseFilters = false;
  bool _UseExternalChecks = false;
  bool _PrintWitnessGraph = false;
  bool _SimplifyWitnessGraph = false;
  bool _InstrumentVerbose = false;
};

}
#endif
