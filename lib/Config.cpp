//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Config.h"

#include "meminstrument/instrumentation_mechanisms/DummyMechanism.h"
#include "meminstrument/instrumentation_mechanisms/LowfatMechanism.h"
#include "meminstrument/instrumentation_mechanisms/NoopMechanism.h"
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

#define DEFAULT_CONFIG SplayConfig

using namespace llvm;
using namespace meminstrument;

namespace {

cl::OptionCategory
    MemInstrumentCat("MemInstrument Options",
                     "configure the memory safety instrumentation");

enum class ConfigKind {
  splay,
  rt_stat,
  external_only,
  noop,
  lowfat,
  default_val,
};

cl::opt<ConfigKind> ConfigKindOpt(
    "mi-config", cl::desc("Choose base configuration"),
    cl::values(clEnumValN(ConfigKind::splay, "splay",
                          "splay-tree-based instrumentation")),
    cl::values(clEnumValN(ConfigKind::external_only, "external_only",
                          "instrumentation that inserts only external checks")),
    cl::values(
        clEnumValN(ConfigKind::rt_stat, "rt_stat",
                   "instrumentation for collection run-time statistics only")),
    cl::values(
        clEnumValN(ConfigKind::noop, "noop",
                   "noop instrumentation that just adds runtime overheads")),
    cl::values(clEnumValN(ConfigKind::lowfat, "lowfat",
                          "low fat pointer based instrumentation")),
    cl::cat(MemInstrumentCat), cl::init(ConfigKind::default_val));

enum class IMKind {
  dummy,
  splay,
  rt_stat,
  noop,
  lowfat,
  default_val,
};

cl::opt<IMKind>
    IMOpt("mi-imechanism", cl::desc("Override InstructionMechanism:"),
          cl::values(clEnumValN(IMKind::dummy, "dummy",
                                "only insert dummy calls for instrumentation")),
          cl::values(clEnumValN(IMKind::splay, "splay",
                                "use splay tree for instrumentation")),
          cl::values(clEnumValN(
              IMKind::noop, "noop",
              "use noop instrumentation that just adds performance overhead")),
          cl::values(
              clEnumValN(IMKind::rt_stat, "rt_stat",
                         "only instrument for collecting run-time statistics")),
          cl::values(clEnumValN(IMKind::lowfat, "lowfat",
                                "use low fat pointers for instrumentation")),
          cl::cat(MemInstrumentCat), cl::init(IMKind::default_val));

InstrumentationMechanism *createInstrumentationMechanism(GlobalConfig &cfg,
                                                         IMKind k) {
  switch (k) {
  case IMKind::dummy:
    return new DummyMechanism(cfg);
  case IMKind::splay:
    return new SplayMechanism(cfg);
  case IMKind::rt_stat:
    return new RuntimeStatMechanism(cfg);
  case IMKind::noop:
    return new NoopMechanism(cfg);
  case IMKind::lowfat:
      return new LowfatMechanism(cfg);
  case IMKind::default_val:
    return nullptr;
  }
  llvm_unreachable("Invalid InstrumentationMechanism!");
}

enum class IPKind {
  beforeOutflow,
  accessOnly,
  none,
  default_val,
};

cl::opt<IPKind> IPOpt(
    "mi-ipolicy", cl::desc("Override InstructionPolicy:"),
    cl::values(clEnumValN(IPKind::beforeOutflow, "before-outflow",
                          "check for dereference at loads/stores and for being"
                          " inbounds when pointers flow out of functions")),
    cl::values(clEnumValN(IPKind::accessOnly, "access-only",
                          "check only at loads/stores for dereference")),
    cl::values(
        clEnumValN(IPKind::none, "none",
                   "do not check anything (except for external checks)")),
    cl::cat(MemInstrumentCat), cl::init(IPKind::default_val));

InstrumentationPolicy *createInstrumentationPolicy(GlobalConfig &cfg, IPKind k,
                                                   const DataLayout &DL) {
  switch (k) {
  case IPKind::beforeOutflow:
    return new BeforeOutflowPolicy(cfg, DL);
  case IPKind::accessOnly:
    return new AccessOnlyPolicy(cfg, DL);
  case IPKind::none:
    return new NonePolicy(cfg, DL);
  case IPKind::default_val:
    return nullptr;
  }
  llvm_unreachable("Invalid InstrumentationPolicy!");
}

enum class WSKind {
  after_inflow,
  none,
  default_val,
};

cl::opt<WSKind> WSOpt(
    "mi-wstrategy", cl::desc("Override WitnessStrategy:"),
    cl::values(clEnumValN(WSKind::after_inflow, "after-inflow",
                          "place witnesses after inflow of the base value")),
    cl::values(clEnumValN(WSKind::none, "none", "place no witnesses")),
    cl::cat(MemInstrumentCat), cl::init(WSKind::default_val));

WitnessStrategy *createWitnessStrategy(GlobalConfig &cfg, WSKind k) {
  switch (k) {
  case WSKind::after_inflow:
    return new AfterInflowStrategy(cfg);
  case WSKind::none:
    return new NoneStrategy(cfg);
  case WSKind::default_val:
    return nullptr;
  }
  llvm_unreachable("Invalid WitnessStrategy!");
}

const char *getModeName(MIMode M) {
  switch (M) {
  case MIMode::SETUP:
    return "Setup";
  case MIMode::GATHER_ITARGETS:
    return "GatherITargets";
  case MIMode::FILTER_ITARGETS:
    return "FilterITargets";
  case MIMode::GENERATE_WITNESSES:
    return "GenerateWitnesses";
  case MIMode::GENERATE_EXTERNAL_CHECKS:
    return "GenerateExternalChecks";
  case MIMode::GENERATE_CHECKS:
    return "GenerateChecks";
  default:
    return "[Unexpected Mode]";
  }
}

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
    cl::cat(MemInstrumentCat), cl::init(MIMode::DEFAULT));

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


cl::opt<int> DefaultTime("mi-noop-time-default",
                            cl::desc("default time in microseconds that operations should take as a noop if not overwritten"),
                            cl::cat(MemInstrumentCat),
                            cl::init(0) // default
);


cl::opt<int> GenBoundsTime("mi-noop-time-gen-bounds",
                            cl::desc("time in microseconds that generating bounds should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> DerefCheckTime("mi-noop-time-deref-check",
                            cl::desc("time in microseconds that a dereference check should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> InvarCheckTime("mi-noop-time-invar-check",
                            cl::desc("time in microseconds that an inbounds check should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> StackAllocTime("mi-noop-time-stack-alloc",
                            cl::desc("time in microseconds that a stack allocation should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> HeapAllocTime("mi-noop-time-heap-alloc",
                            cl::desc("time in microseconds that a heap allocation should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> GlobalAllocTime("mi-noop-time-global-alloc",
                            cl::desc("time in microseconds that a global allocation should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

cl::opt<int> HeapFreeTime("mi-noop-time-heap-free",
                            cl::desc("time in microseconds that a heap deallocation should take as a noop"),
                            cl::cat(MemInstrumentCat),
                            cl::init(-1) // default
);

uint32_t getNoopValOrDefault(int32_t v) {
  if (v != -1) {
    return v;
  } else {
    return DefaultTime;
  }
}

} // namespace

namespace meminstrument {

/// The base class for configurations
class Config {
public:
  virtual ~Config(void) {}

  virtual IPKind getInstrumentationPolicy(void) const = 0;
  virtual IMKind getInstrumentationMechanism(void) const = 0;
  virtual WSKind getWitnessStrategy(void) const = 0;
  virtual MIMode getMIMode(void) const = 0;
  virtual bool hasUseFilters(void) const = 0;
  virtual bool hasUseExternalChecks(void) const = 0;
  virtual bool hasPrintWitnessGraph(void) const = 0;
  virtual bool hasSimplifyWitnessGraph(void) const = 0;
  virtual bool hasInstrumentVerbose(void) const = 0;
  virtual const char *getName(void) const = 0;

  static Config *create(ConfigKind k);
};

/// A configuration to perform splay-tree-based instrumentation.
/// Includes all internal filters and simplifications.
class SplayConfig : public Config {
public:
  virtual ~SplayConfig(void) {}

  virtual IPKind getInstrumentationPolicy(void) const override {
    return IPKind::beforeOutflow;
  }
  virtual IMKind getInstrumentationMechanism(void) const override {
    return IMKind::splay;
  }
  virtual WSKind getWitnessStrategy(void) const override {
    return WSKind::after_inflow;
  }
  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_CHECKS;
  }
  virtual bool hasUseFilters(void) const override { return true; }
  virtual bool hasUseExternalChecks(void) const override { return false; }
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return true; }
  virtual bool hasInstrumentVerbose(void) const override { return false; }
  virtual const char *getName(void) const override { return "Splay"; }
};

/// A configuration to perform only instrumentation for external checks.
class ExternalOnlyConfig : public SplayConfig {
public:
  virtual ~ExternalOnlyConfig(void) {}

  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_EXTERNAL_CHECKS;
  }
  virtual bool hasUseFilters(void) const override { return false; }
  virtual bool hasUseExternalChecks(void) const override { return true; }
  virtual const char *getName(void) const override { return "ExternalOnly"; }
};

/// A configuration to perform instrumentation for collecting run-time
/// statistics.
class RTStatConfig : public Config {
public:
  virtual ~RTStatConfig(void) {}

  virtual IPKind getInstrumentationPolicy(void) const override {
    return IPKind::accessOnly;
  }
  virtual IMKind getInstrumentationMechanism(void) const override {
    return IMKind::rt_stat;
  }
  virtual WSKind getWitnessStrategy(void) const override {
    return WSKind::none;
  }
  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_CHECKS;
  }
  virtual bool hasUseFilters(void) const override { return false; }
  virtual bool hasUseExternalChecks(void) const override { return false; }
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return false; }
  virtual bool hasInstrumentVerbose(void) const override { return true; }
  virtual const char *getName(void) const override { return "RTStat"; }
};

/// A configuration to perform noop instrumentation that just adds performance
/// overheads.
class NoopConfig : public Config {
public:
  virtual ~NoopConfig(void) {}

  virtual IPKind getInstrumentationPolicy(void) const override {
    return IPKind::accessOnly;
  }
  virtual IMKind getInstrumentationMechanism(void) const override {
    return IMKind::noop;
  }
  virtual WSKind getWitnessStrategy(void) const override {
    return WSKind::none;
  }
  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_CHECKS;
  }
  virtual bool hasUseFilters(void) const override { return false; }
  virtual bool hasUseExternalChecks(void) const override { return false; }
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return false; }
  virtual bool hasInstrumentVerbose(void) const override { return false; }
  virtual const char *getName(void) const override { return "Noop"; }
};

/// A configuration to perform instrumentation based on low fat pointers
class LowfatConfig : public Config {
public:
    virtual ~LowfatConfig(void) {}

    virtual IPKind getInstrumentationPolicy(void) const override {
        return IPKind::beforeOutflow;
    }
    virtual IMKind getInstrumentationMechanism(void) const override {
        return IMKind::lowfat;
    }
    virtual WSKind getWitnessStrategy(void) const override {
        return WSKind::after_inflow;
    }
    virtual MIMode getMIMode(void) const override {
        return MIMode::GENERATE_CHECKS;
    }
    virtual bool hasUseFilters(void) const override { return true; }
    virtual bool hasUseExternalChecks(void) const override { return false; }
    virtual bool hasPrintWitnessGraph(void) const override { return false; }
    virtual bool hasSimplifyWitnessGraph(void) const override { return true; }
    virtual bool hasInstrumentVerbose(void) const override { return false; }
    virtual const char *getName(void) const override { return "Lowfat"; }
};

Config *Config::create(ConfigKind k) {
  switch (k) {
  case ConfigKind::splay:
    return new SplayConfig();
  case ConfigKind::rt_stat:
    return new RTStatConfig();
  case ConfigKind::noop:
    return new NoopConfig();
  case ConfigKind::external_only:
    return new ExternalOnlyConfig();
  case ConfigKind::lowfat:
    return new LowfatConfig();
  case ConfigKind::default_val:
    return new DEFAULT_CONFIG();
  }
  llvm_unreachable("Invalid ConfigKind!");
}

} // namespace meminstrument

GlobalConfig::GlobalConfig(Config *Cfg, const llvm::Module &M) {

#define X_OR_DEFAULT(TYPE, X, Y) (((X) != TYPE::default_val) ? (X) : (Y))

  IMKind imk = X_OR_DEFAULT(IMKind, IMOpt, Cfg->getInstrumentationMechanism());
  _IM = createInstrumentationMechanism(*this, imk);

  const DataLayout &DL = M.getDataLayout();
  IPKind ipk = X_OR_DEFAULT(IPKind, IPOpt, Cfg->getInstrumentationPolicy());
  _IP = createInstrumentationPolicy(*this, ipk, DL);

  WSKind wsk = X_OR_DEFAULT(WSKind, WSOpt, Cfg->getWitnessStrategy());
  _WS = createWitnessStrategy(*this, wsk);

#undef X_OR_DEFAULT

  if (MIModeOpt != MIMode::DEFAULT) {
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

#define GET_NOOP_METHOD(x) \
uint32_t GlobalConfig::getNoop##x##Time(void) { \
  return getNoopValOrDefault(x##Time); \
}

GET_NOOP_METHOD(GenBounds)
GET_NOOP_METHOD(DerefCheck)
GET_NOOP_METHOD(InvarCheck)
GET_NOOP_METHOD(StackAlloc)
GET_NOOP_METHOD(HeapAlloc)
GET_NOOP_METHOD(GlobalAlloc)
GET_NOOP_METHOD(HeapFree)

bool GlobalConfig::hasUseNoop(void) {
  bool res = false;
  res = res || (GenBoundsTime != -1);
  res = res || (DerefCheckTime != -1);
  res = res || (InvarCheckTime != -1);
  res = res || (StackAllocTime != -1);
  res = res || (HeapAllocTime != -1);
  res = res || (GlobalAllocTime != -1);
  res = res || (HeapFreeTime != -1);
  return res;
}

std::unique_ptr<GlobalConfig> GlobalConfig::create(const llvm::Module &M) {
  auto GlobalCfg = std::unique_ptr<GlobalConfig>(
      new GlobalConfig(Config::create(ConfigKindOpt), M));
  DEBUG(dbgs() << "Creating MemInstrument Config:\n"; GlobalCfg->dump(dbgs()););
  return GlobalCfg;
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

void GlobalConfig::noteError(void) { ++_numErrors; }

bool GlobalConfig::hasErrors(void) const { return _numErrors != 0; }
