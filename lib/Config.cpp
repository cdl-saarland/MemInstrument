//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Config.h"

#include "meminstrument/instrumentation_mechanisms/DummyMechanism.h"
#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"
#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"
#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"
#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"
#include "meminstrument/instrumentation_policies/NonePolicy.h"
#include "meminstrument/witness_strategies/AfterInflowStrategy.h"
#include "meminstrument/witness_strategies/NoneStrategy.h"

#include "llvm/Support/CommandLine.h"

#include <cstdlib>

#include "meminstrument/pass/Util.h"

using namespace llvm;
using namespace meminstrument;

// TODO The overhead for inserting new options could be reduced by using some
// preprocessor magic...

/// Name of the environment variable to check for configuration input
#define MI_CONFIG_ENV_VAR "MI_CONFIG"

namespace {

cl::OptionCategory
    MemInstrumentCat("MemInstrument Options",
                     "configure the memory safety instrumentation");

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
    cl::values(
        clEnumValN(IM_rt_stat, "rt_stat",
                   "only instrument for collecting run-time statistics")),
    cl::cat(MemInstrumentCat),
    cl::init(IM_default) // default
);

enum InstrumentationPolicyKind {
  IP_beforeOutflow,
  IP_accessOnly,
  IP_none,
  IP_default,
};

cl::opt<InstrumentationPolicyKind> InstrumentationPolicyOpt(
    "mi-ipolicy", cl::desc("Override InstructionPolicy:"),
    cl::values(clEnumValN(IP_beforeOutflow, "before-outflow",
                          "check for dereference at loads/stores and for being"
                          " inbounds when pointers flow out of functions")),
    cl::values(clEnumValN(IP_accessOnly, "access-only",
                          "check only at loads/stores for dereference")),
    cl::values(clEnumValN(IP_none, "none",
                          "do not check anything (except for external checks)")),
    cl::cat(MemInstrumentCat), cl::init(IP_default));

enum WitnessStrategyKind {
  WS_after_inflow,
  WS_none,
  WS_default,
};

cl::opt<WitnessStrategyKind> WitnessStrategyOpt(
    "mi-wstrategy", cl::desc("Override WitnessStrategy:"),
    cl::values(clEnumValN(WS_after_inflow, "after-inflow",
                          "place witnesses after inflow of the base value")),
    cl::values(clEnumValN(WS_none, "none", "place no witnesses")),
    cl::cat(MemInstrumentCat), cl::init(WS_default));

cl::opt<Config::MIMode> MIModeOpt(
    "mi-mode",
    cl::desc("Override until which stage instrumentation should be performed:"),
    cl::values(clEnumValN(Config::MIMode::SETUP, "setup",
                          "only until setup is done")),
    cl::values(clEnumValN(Config::MIMode::GATHER_ITARGETS, "gatheritargets",
                          "only until ITarget gathering is done")),
    cl::values(clEnumValN(Config::MIMode::FILTER_ITARGETS, "filteritargets",
                          "only until ITarget filtering is done")),
    cl::values(clEnumValN(Config::MIMode::GENERATE_WITNESSES, "genwitnesses",
                          "only until witness generation is done")),
    cl::values(clEnumValN(Config::MIMode::GENERATE_EXTERNAL_CHECKS,
                          "genextchecks",
                          "only until external check generation is done")),
    cl::values(clEnumValN(Config::MIMode::GENERATE_CHECKS, "genchecks",
                          "the full pipeline")),
    cl::cat(MemInstrumentCat), cl::init(Config::MIMode::DEFAULT));

enum ConfigKind {
  CK_splay,
  CK_rt_stat,
  CK_external_only,
  CK_default,
};

cl::opt<ConfigKind> ConfigKindOpt(
    "mi-config", cl::desc("Choose base configuration"),
    cl::values(clEnumValN(CK_splay, "splay",
                          "splay-tree-based instrumentation")),
    cl::values(clEnumValN(CK_external_only, "external_only",
                          "instrumentation that inserts only external checks")),
    cl::values(
        clEnumValN(CK_rt_stat, "rt_stat",
                   "instrumentation for collection run-time statistics only")),
    cl::cat(MemInstrumentCat), cl::init(CK_default));

cl::opt<cl::boolOrDefault>
    UseFiltersOpt("mi-use-filters",
                  cl::desc("Enable memsafety instrumentation target filters"),
                  cl::cat(MemInstrumentCat));

cl::opt<cl::boolOrDefault>
    UseExternalChecksOpt("mi-use-extchecks",
                         cl::desc("Enable generation of external checks"),
                         cl::cat(MemInstrumentCat));

cl::opt<cl::boolOrDefault>
    PrintWitnessGraphOpt("mi-print-witnessgraph",
                         cl::desc("Print the WitnessGraph"),
                         cl::cat(MemInstrumentCat));

cl::opt<cl::boolOrDefault>
    SimplifyWitnessGraphOpt("mi-simplify-witnessgraph",
                            cl::desc("Enable witness graph simplifications"),
                            cl::cat(MemInstrumentCat));

cl::opt<cl::boolOrDefault>
    InstrumentVerboseOpt("mi-verbose", cl::desc("Use verbose check functions"),
                         cl::cat(MemInstrumentCat));

Config *createConfigCLI(void) {
  switch (ConfigKindOpt) {
  case CK_splay:
    return new SplayConfig();
  case CK_rt_stat:
    return new RTStatConfig();
  case CK_external_only:
    return new ExternalOnlyConfig();
  case CK_default: {
      const char *EnvStr = std::getenv(MI_CONFIG_ENV_VAR);

      if (EnvStr == nullptr) {
        return new SplayConfig(); // default Config HERE
      }

      if (0 == strcmp(EnvStr, "splay")) {
        return new SplayConfig();
      } else if (0 == strcmp(EnvStr, "rt_stat")) {
        return new RTStatConfig();
      } else if (0 == strcmp(EnvStr, "external_only")) {
        return new ExternalOnlyConfig();
      } else {
        errs() << "Unknown meminstrument config name: `" << EnvStr << "'\n";
        return new SplayConfig();
      }
    }
  }
  llvm_unreachable("Invalid ConfigKind!");
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
  llvm_unreachable("Invalid InstrumentationMechanism!");
}

InstrumentationPolicy *createInstrumentationPolicyCLI(const DataLayout &DL) {
  switch (InstrumentationPolicyOpt) {
  case IP_beforeOutflow:
    return new BeforeOutflowPolicy(DL);
  case IP_accessOnly:
    return new AccessOnlyPolicy(DL);
  case IP_none:
    return new NonePolicy(DL);
  case IP_default:
    return nullptr;
  }
  llvm_unreachable("Invalid InstrumentationPolicy!");
}

WitnessStrategy *createWitnessStrategyCLI(void) {
  switch (WitnessStrategyOpt) {
  case WS_after_inflow:
    return new AfterInflowStrategy();
  case WS_none:
    return new NoneStrategy();
  case WS_default:
    return nullptr;
  }
  llvm_unreachable("Invalid WitnessStrategy!");
}

bool getValOrDefault(cl::boolOrDefault val, bool defaultVal) {
  switch (val) {
  case cl::BOU_UNSET:
    return defaultVal;
  case cl::BOU_TRUE:
    return true;
  case cl::BOU_FALSE:
    return false;
  }
  llvm_unreachable("Invalid BOU value!");
}

GlobalConfig* GlobalCfg = nullptr;

const char *getModeName(Config::MIMode M) {
  switch (M) {
    case Config::MIMode::SETUP:
      return "Setup";
    case Config::MIMode::GATHER_ITARGETS:
      return "GatherITargets";
    case Config::MIMode::FILTER_ITARGETS:
      return "FilterITargets";
    case Config::MIMode::GENERATE_WITNESSES:
      return "GenerateWitnesses";
    case Config::MIMode::GENERATE_EXTERNAL_CHECKS:
      return "GenerateExternalChecks";
    case Config::MIMode::GENERATE_CHECKS:
      return "GenerateChecks";
    default:
      return "[Unexpected Mode]";
  }
}

} // namespace

GlobalConfig::GlobalConfig(Config *Cfg, const llvm::Module &M) {

  InstrumentationMechanism *IM = createInstrumentationMechanismCLI();
  _IM = IM ? IM : Cfg->createInstrumentationMechanism();

  const DataLayout &DL = M.getDataLayout();
  InstrumentationPolicy *IP = createInstrumentationPolicyCLI(DL);
  _IP = IP ? IP : Cfg->createInstrumentationPolicy(DL);

  WitnessStrategy *WS = createWitnessStrategyCLI();
  _WS = WS ? WS : Cfg->createWitnessStrategy();

  if (MIModeOpt != Config::MIMode::DEFAULT) {
    _MIMode = MIModeOpt;
  } else {
    _MIMode = Cfg->getMIMode();
  }

#define ASSIGNBOOLOPT(x) _##x = getValOrDefault(x##Opt, Cfg->has##x());

  ASSIGNBOOLOPT(UseFilters)
  ASSIGNBOOLOPT(UseExternalChecks)
  ASSIGNBOOLOPT(PrintWitnessGraph)
  ASSIGNBOOLOPT(SimplifyWitnessGraph)
  ASSIGNBOOLOPT(InstrumentVerbose)

#undef ASSIGNBOOLOPT

  _ConfigName = Cfg->getName();

  delete Cfg;
}

GlobalConfig &GlobalConfig::get(const llvm::Module &M) {
  if (GlobalCfg == nullptr) {
    GlobalCfg = new GlobalConfig(createConfigCLI(), M);
    DEBUG(
      dbgs() << "Creating MemInstrument Config:\n";
      GlobalCfg->dump(dbgs());
    );
  }
  return *GlobalCfg;
}

void GlobalConfig::release(void) {
  delete GlobalCfg;
}

void GlobalConfig::dump(llvm::raw_ostream &Stream) const {
  Stream << "{{{ Config: " << _ConfigName << "\n";
  Stream << "     InstrumentationPolicy: " << _IP->getName() << '\n';
  Stream << "           WitnessStrategy: " << _WS->getName() << '\n';
  Stream << "  InstrumentationMechanism: " << _IM->getName() << '\n';
  Stream << "                      Mode: " << getModeName(_MIMode) << '\n';
  Stream << "                UseFilters: " << _UseFilters << '\n';
  Stream << "         UseExternalChecks: " << _UseExternalChecks << '\n';
  Stream << "         PrintWitnessGraph: " << _PrintWitnessGraph << '\n';
  Stream << "      SimplifyWitnessGraph: " << _SimplifyWitnessGraph << '\n';
  Stream << "         InstrumentVerbose: " << _InstrumentVerbose << '\n';
  Stream << "}}}\n\n";
}

// Implementation of SplayConfig

InstrumentationPolicy *
SplayConfig::createInstrumentationPolicy(const llvm::DataLayout &DL) {
  return new BeforeOutflowPolicy(DL);
}

InstrumentationMechanism *SplayConfig::createInstrumentationMechanism(void) {
  return new SplayMechanism();
}

WitnessStrategy *SplayConfig::createWitnessStrategy(void) {
  return new AfterInflowStrategy();
}

Config::MIMode SplayConfig::getMIMode(void) {
  return Config::MIMode::GENERATE_CHECKS;
}
bool SplayConfig::hasUseFilters(void) { return true; }
bool SplayConfig::hasUseExternalChecks(void) { return false; }
bool SplayConfig::hasPrintWitnessGraph(void) { return false; }
bool SplayConfig::hasSimplifyWitnessGraph(void) { return true; }
bool SplayConfig::hasInstrumentVerbose(void) { return false; }

const char *SplayConfig::getName(void) const {
  return "Splay";
}

// Implementation of ExternalOnlyConfig

InstrumentationPolicy *
ExternalOnlyConfig::createInstrumentationPolicy(const llvm::DataLayout &DL) {
  return new BeforeOutflowPolicy(DL);
}

InstrumentationMechanism *ExternalOnlyConfig::createInstrumentationMechanism(void) {
  return new SplayMechanism();
}

WitnessStrategy *ExternalOnlyConfig::createWitnessStrategy(void) {
  return new AfterInflowStrategy();
}

Config::MIMode ExternalOnlyConfig::getMIMode(void) {
  return Config::MIMode::GENERATE_EXTERNAL_CHECKS;
}
bool ExternalOnlyConfig::hasUseFilters(void) { return false; }
bool ExternalOnlyConfig::hasUseExternalChecks(void) { return true; }
bool ExternalOnlyConfig::hasPrintWitnessGraph(void) { return false; }
bool ExternalOnlyConfig::hasSimplifyWitnessGraph(void) { return true; }
bool ExternalOnlyConfig::hasInstrumentVerbose(void) { return false; }

const char *ExternalOnlyConfig::getName(void) const {
  return "ExternalOnly";
}


// Implementation of RTStatConfig
InstrumentationPolicy *
RTStatConfig::createInstrumentationPolicy(const llvm::DataLayout &DL) {
  return new AccessOnlyPolicy(DL);
}

InstrumentationMechanism *RTStatConfig::createInstrumentationMechanism(void) {
  return new RuntimeStatMechanism();
}

WitnessStrategy *RTStatConfig::createWitnessStrategy(void) {
  return new NoneStrategy();
}

Config::MIMode RTStatConfig::getMIMode(void) {
  return Config::MIMode::GENERATE_CHECKS;
}
bool RTStatConfig::hasUseFilters(void) { return false; }
bool RTStatConfig::hasUseExternalChecks(void) { return false; }
bool RTStatConfig::hasPrintWitnessGraph(void) { return false; }
bool RTStatConfig::hasSimplifyWitnessGraph(void) { return false; }
bool RTStatConfig::hasInstrumentVerbose(void) { return true; }

const char *RTStatConfig::getName(void) const {
  return "RTStat";
}
