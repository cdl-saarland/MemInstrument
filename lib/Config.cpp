//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Config.h"

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"
#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"
#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"
#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"
#include "meminstrument/witness_strategies/AfterInflowStrategy.h"
#include "meminstrument/witness_strategies/NoneStrategy.h"

using namespace llvm;
using namespace meminstrument;

namespace {

cl::OptionCategory MemInstrumentCat("MemInstrument Options", "configure the memory safety instrumentation");

enum InstrumentationMechanismKind {
  IM_dummy,
  IM_splay,
  IM_rt_stat,
  IM_default,
};

cl::opt<InstrumentationMechanismKind> InstrumentationMechanismOpt(
    "mi-imechanism", cl::desc("Override InstructionMechanism:"),
    cl::values(clEnumValN(IM_dummy, "dummy",
                          "only insert dummy calls for instrumentation")),
    cl::values(clEnumValN(IM_splay, "splay",
                          "use splay tree for instrumentation")),
    cl::values(clEnumValN(IM_rt_stat, "rt_stat",
                          "only instrument for collecting run-time statistics")),
    cl::cat(MemInstrumentCat),
    cl::init(IM_default) // default
);

enum InstrumentationPolicyKind {
  IP_beforeOutflow,
  IP_accessOnly,
  IP_default,
};

cl::opt<InstrumentationPolicyKind> InstrumentationPolicyOpt(
    "mi-ipolicy",
    cl::desc("Override InstructionPolicy:"),
    cl::values(clEnumValN(IP_beforeOutflow, "before-outflow",
                          "check for dereference at loads/stores and for being"
                          " inbounds when pointers flow out of functions")),
    cl::values(clEnumValN(IP_accessOnly, "access-only",
                          "check only at loads/stores for dereference")),
    cl::cat(MemInstrumentCat),
    cl::init(IP_default)
);

enum WitnessStrategyKind {
  WS_after_inflow,
  WS_none,
  WS_default,
};

cl::opt<WitnessStrategyKind> WitnessStrategyOpt(
    "mi-wstrategy", cl::desc("Override WitnessStrategy:"),
    cl::values(clEnumValN(WS_after_inflow, "after-inflow",
                          "place witnesses after inflow of the base value")),
    cl::values(clEnumValN(WS_none, "none",
                          "place no witnesses")),
    cl::cat(MemInstrumentCat),
    cl::init(WS_default)
);

cl::opt<MIMode> MIModeOpt(
    "mi-mode",
    cl::desc("Override until which stage instrumentation should be performed:"),
    cl::values(clEnumValN(MIMode::SETUP, "setup", "only until setup is done")),
    cl::values(clEnumValN(MIMode::GATHER_ITARGETS, "gatheritargets",
                          "only until ITarget gathering is done")),
    cl::values(clEnumValN(MIMode::FILTER_ITARGETS, "filteritargets",
                          "only until ITarget filtering is done")),
    cl::values(clEnumValN(MIMode::GENERATE_WITNESSES, "genwitnesses",
                          "only until witness generation is done")),
    cl::values(clEnumValN(MIMode::GENERATE_EXTERNAL_CHECKS, "genextchecks",
                          "only until external check generation is done")),
    cl::values(clEnumValN(MIMode::GENERATE_CHECKS, "genchecks",
                          "the full pipeline")),
    cl::cat(MemInstrumentCat),
    cl::init(MIMode::DEFAULT)
);

enum ConfigKind {
  CK_splay,
  CK_rt_stat,
};

cl::opt<ConfigKind> ConfigKindOpt(
    "mi-config", cl::desc("Choose configuration"),
    cl::values(clEnumValN(CK_splay, "splay",
                          "splay-tree-based instrumentation")),
    cl::values(clEnumValN(CK_rt_stat, "rt_stat",
                          "instrumentation for collection run-time statistics only")),
    cl::cat(MemInstrumentCat),
    cl::init(CK_rt_stat)
);

cl::opt<boolOrDefault> UseFiltersOpt(
    "mi-use-filters",
    cl::desc("Enable memsafety instrumentation target filters"),
    cl::cat(MemInstrumentCat),
    );

cl::opt<boolOrDefault>
    UseExternalChecksOpt("mi-use-extchecks",
                           cl::desc("Enable generation of external checks"),
                           cl::cat(MemInstrumentCat),
    );

cl::opt<boolOrDefault> PrintWitnessGraphOpt("mi-print-witnessgraph",
                                   cl::desc("Print the WitnessGraph"),
                                   cl::cat(MemInstrumentCat),
);

cl::opt<boolOrDefault>
    NoSimplifyWitnessGraphOpt("mi-no-simplify-witnessgraph",
                              cl::desc("Disable witness graph simplifications"),
                              cl::cat(MemInstrumentCat),
    );

cl::opt<boolOrDefault> InstrumentVerboseOpt("mi-verbose",
                           cl::desc("Use verbose check functions"),
                              cl::cat(MemInstrumentCat),
                           );

WitenssStrategy *createConfigCLI(void) {
  switch (ConfigKindOpt) {
  case CK_splay:
    return new SplayConfig();
  case CK_rt_stat:
    return new RTStatConfig();
  }
}

InstrumentationMechanism *createInstrumentationMechanismCLI(void) {
  switch (InstrumentationMechanismOpt) {
  case IM_dummy:
    return new DummyMechanism();
  case IM_splay:
    return new SplayMechanism();
  case IM_rt_stat:
    return new RuntimeStatMechanism();
  case IM_default:
    return nullptr;
  }
}

InstrumentationPolicy *createInstrumentationPolicyCLI(const DataLayout &DL) {
  switch (InstrumentationPolicyOpt) {
  case IP_beforeOutflow:
    return new BeforeOutflowPolicy(DL);
  case IP_accessOnly:
    return new AccessOnlyPolicy(DL);
  case IP_default:
    return nullptr;
  }
}

WitenssStrategy *createWitnessStrategyCLI(void) {
  switch (WitnessStrategyOpt) {
  case WS_after_inflow:
    return new AfterInflowStrategy();
  case WS_none:
    return new NoneStrategy();
  case WS_default:
    return nullptr;
  }
}

bool getValOrDefault(boolOrDefault& val, bool defaultVal) {
  switch (val) {
    case BOU_UNSET:
      return defaultVal;
    case BOU_TRUE:
      return true;
    case BOU_FALSE:
      return false;
  }
}

std::unique_ptr<GlobalConfig> GlobalCfg(nullptr);
} // namespace


GlobalConfig::GlobalConfig(Config& Cfg, const llvm::Module& M) {

  InstrumentationMechanism* IM = createInstrumentationMechanismCLI();
  _IM = IM ? IM : Cfg.createInstrumentationMechanism();

  const DataLayout &DL = M.getDataLayout();
  InstrumentationPolicy* IP = createInstrumentationPolicyCLI(DL);
  _IP = IP ? IP : Cfg.createInstrumentationPolicy(DL);

  WitnessStrategy* WS = createWitnessStrategyCLI();
  _WS = WS ? WS : Cfg.createWitnessStrategy();

  if (MIModeOpt != MIMode::DEFAULT) {
    _MIMode = MIModeOpt;
  } else {
    _MIMode = Cfg.getMIMode();
  }

#define ASSIGNBOOLOPT(x) \
  _##x = getValOrDefault(x##Opt, Cfg.has##x);

  ASSIGNBOOLOPT(UseFilters)
  ASSIGNBOOLOPT(UseExternalChecks)
  ASSIGNBOOLOPT(PrintWitnessGraph)
  ASSIGNBOOLOPT(SimplifyWitnessGraph)
  ASSIGNBOOLOPT(InstrumentVerboe)

#undef ASSIGNBOOLOPT
}

static Config &Config::get(const llvm::Module& M) {
  auto *Res = GlobalCfg.get();
  if (Res == nullptr) {
    GlobalCfg.reset(new GlobalConfig(createConfigCLI(), M));
    Res = GlobalCfg.get();
  }
  return *Res;
}


InstrumentationPolicy *SplayConfig::createInstrumentationPolicy(const llvm::DataLayout& D) override {
  return new BeforeOutflowPolicy(DL);
}

InstrumentationMechanism *SplayConfig::createInstrumentationMechanism(void) override {
  return new SplayMechanism();
};

WitnessStrategy *SplayConfig::createWitnessStrategy(void) override {
  return new AfterInflowStrategy();
}

MIMode SplayConfig::getMIMode(void) override  { return MIMode::GENERATE_CHECKS; }
bool SplayConfig::hasUseFilters(void) { return true; }
bool SplayConfig::hasUseExternalChecks(void) { return false; }
bool SplayConfig::hasPrintWitnessGraph(void) { return false; }
bool SplayConfig::hasSimplifyWitnessGraph(void) { return true; }
bool SplayConfig::hasInstrumentVerbose(void) { return false; }

InstrumentationPolicy *RTStatConfig::createInstrumentationPolicy(const llvm::DataLayout& D) override {
  return new AccessOnlyPolicy(DL);
}

InstrumentationMechanism *RTStatConfig::createInstrumentationMechanism(void) override {
  return new RuntimeStatMechanism();
};

WitnessStrategy *RTStatConfig::createWitnessStrategy(void) override {
  return new NoneStrategy();
}

MIMode RTStatConfig::getMIMode(void) override  { return MIMode::GENERATE_CHECKS; }
bool RTStatConfig::hasUseFilters(void) { return false; }
bool RTStatConfig::hasUseExternalChecks(void) { return false; }
bool RTStatConfig::hasPrintWitnessGraph(void) { return false; }
bool RTStatConfig::hasSimplifyWitnessGraph(void) { return false; }
bool RTStatConfig::hasInstrumentVerbose(void) { return false; }

