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

namespace meminstrument {

/// The base class for configurations
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

  virtual ~Config(void) {}

  virtual InstrumentationPolicy *
  createInstrumentationPolicy(const llvm::DataLayout &DL) = 0;

  virtual InstrumentationMechanism *createInstrumentationMechanism(void) = 0;

  virtual WitnessStrategy *createWitnessStrategy(void) = 0;

  virtual MIMode getMIMode(void) = 0;

  virtual bool hasUseFilters(void) = 0;

  virtual bool hasUseExternalChecks(void) = 0;

  virtual bool hasPrintWitnessGraph(void) = 0;

  virtual bool hasSimplifyWitnessGraph(void) = 0;

  virtual bool hasInstrumentVerbose(void) = 0;

  virtual const char *getName(void) const = 0;
};

/// A configuration to perform splay-tree-based instrumentation.
/// Includes all internal filters and simplifications.
class SplayConfig : public Config {
public:
  virtual ~SplayConfig(void) {}

  virtual InstrumentationPolicy *
  createInstrumentationPolicy(const llvm::DataLayout &D) override;
  virtual InstrumentationMechanism *
  createInstrumentationMechanism(void) override;
  virtual WitnessStrategy *createWitnessStrategy(void) override;
  virtual MIMode getMIMode(void) override;
  virtual bool hasUseFilters(void) override;
  virtual bool hasUseExternalChecks(void) override;
  virtual bool hasPrintWitnessGraph(void) override;
  virtual bool hasSimplifyWitnessGraph(void) override;
  virtual bool hasInstrumentVerbose(void) override;
  virtual const char *getName(void) const override;
};

/// A configuration to perform instrumentation for collecting run-time
/// statistics.
class RTStatConfig : public Config {
public:
  virtual ~RTStatConfig(void) {}

  virtual InstrumentationPolicy *
  createInstrumentationPolicy(const llvm::DataLayout &D) override;
  virtual InstrumentationMechanism *
  createInstrumentationMechanism(void) override;
  virtual WitnessStrategy *createWitnessStrategy(void) override;
  virtual MIMode getMIMode(void) override;
  virtual bool hasUseFilters(void) override;
  virtual bool hasUseExternalChecks(void) override;
  virtual bool hasPrintWitnessGraph(void) override;
  virtual bool hasSimplifyWitnessGraph(void) override;
  virtual bool hasInstrumentVerbose(void) override;
  virtual const char *getName(void) const override;
};

/// Class for the actual configuration items in use.
/// On a normal run, only one of these should be created in the first call of
/// the static GlobalConfig::get() method, following calls just provide the
/// same GlobalConfig.
///
/// Configurations are always based on one of the Classes that derive from
/// `Config`. This `Config` instance can be chosen via the -mi-config=...
/// command line flag or via the MI_CONFIG environment variable (the former
/// takes precedence if both are specified).
/// The chosen `Config` instance can be adjusted via further command line flags.
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

  Config::MIMode getMIMode(void) { return _MIMode; }

  bool hasUseFilters(void) { return _UseFilters; }

  bool hasUseExternalChecks(void) { return _UseExternalChecks; }

  bool hasPrintWitnessGraph(void) { return _PrintWitnessGraph; }

  bool hasSimplifyWitnessGraph(void) { return _SimplifyWitnessGraph; }

  bool hasInstrumentVerbose(void) { return _InstrumentVerbose; }

  static GlobalConfig &get(const llvm::Module &M);

  static void release(void);

  void dump(llvm::raw_ostream &Stream) const;

  ~GlobalConfig(void) {
    delete _IM;
    delete _IP;
    delete _WS;
  }

private:
  GlobalConfig(Config *Cfg, const llvm::Module &M);

  InstrumentationMechanism *_IM = nullptr;
  InstrumentationPolicy *_IP = nullptr;
  WitnessStrategy *_WS = nullptr;

  Config::MIMode _MIMode = Config::MIMode::DEFAULT;

  bool _UseFilters = false;
  bool _UseExternalChecks = false;
  bool _PrintWitnessGraph = false;
  bool _SimplifyWitnessGraph = false;
  bool _InstrumentVerbose = false;

  const char *_ConfigName;
};

} // namespace meminstrument
#endif
