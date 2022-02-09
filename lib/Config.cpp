//===- Config.cpp - Instrumentation configuration -------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/Config.h"

#include "meminstrument/instrumentation_mechanisms/DummyMechanism.h"
#include "meminstrument/instrumentation_mechanisms/LowfatMechanism.h"
#include "meminstrument/instrumentation_mechanisms/NoopMechanism.h"
#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"
#include "meminstrument/instrumentation_mechanisms/SoftBoundMechanism.h"
#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"
#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"
#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"
#include "meminstrument/instrumentation_policies/NonePolicy.h"
#include "meminstrument/instrumentation_policies/PointerBoundsPolicy.h"
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
  optimization_checks_only,
  noop,
  lowfat,
  softbound,
  default_val,
};

cl::opt<ConfigKind> ConfigKindOpt(
    "mi-config", cl::desc("Choose base configuration"),
    cl::values(
        clEnumValN(ConfigKind::splay, "splay",
                   "splay-tree-based instrumentation"),
        clEnumValN(ConfigKind::optimization_checks_only,
                   "optimization-checks-only",
                   "instrumentation that inserts only external checks"),
        clEnumValN(ConfigKind::rt_stat, "rt_stat",
                   "instrumentation for collection run-time statistics only"),
        clEnumValN(ConfigKind::noop, "noop",
                   "noop instrumentation that just adds runtime overheads"),
        clEnumValN(ConfigKind::lowfat, "lowfat",
                   "low fat pointer based instrumentation"),
        clEnumValN(ConfigKind::softbound, "softbound",
                   "SoftBound instrumentation")),
    cl::cat(MemInstrumentCat), cl::init(ConfigKind::default_val));

enum class IMKind {
  dummy,
  splay,
  rt_stat,
  noop,
  lowfat,
  softbound,
  default_val,
};

cl::opt<IMKind> IMOpt(
    "mi-imechanism", cl::desc("Override InstructionMechanism:"),
    cl::values(
        clEnumValN(IMKind::dummy, "dummy",
                   "only insert dummy calls for instrumentation"),
        clEnumValN(IMKind::splay, "splay",
                   "use splay tree for instrumentation"),
        clEnumValN(
            IMKind::noop, "noop",
            "use noop instrumentation that just adds performance overhead"),

        clEnumValN(IMKind::rt_stat, "rt_stat",
                   "only instrument for collecting run-time statistics"),
        clEnumValN(IMKind::lowfat, "lowfat",
                   "use low fat pointers for instrumentation"),
        clEnumValN(IMKind::softbound, "softbound",
                   "use SoftBound for instrumentation")),
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
  case IMKind::softbound:
    return new SoftBoundMechanism(cfg);
  case IMKind::default_val:
    return nullptr;
  }
  llvm_unreachable("Invalid InstrumentationMechanism!");
}

enum class IPKind {
  beforeOutflow,
  accessOnly,
  none,
  pointerBounds,
  default_val,
};

cl::opt<IPKind> IPOpt(
    "mi-ipolicy", cl::desc("Override InstructionPolicy:"),
    cl::values(clEnumValN(IPKind::beforeOutflow, "before-outflow",
                          "check for dereference at loads/stores and for being"
                          " inbounds when pointers flow out of functions"),
               clEnumValN(IPKind::accessOnly, "access-only",
                          "check only at loads/stores for dereference"),
               clEnumValN(IPKind::none, "none",
                          "do not check anything (except for external checks)"),
               clEnumValN(IPKind::pointerBounds, "pointer-bounds",
                          "pointer bounds strategy based targets (loads, "
                          "stores, calls, function arguments)")),
    cl::cat(MemInstrumentCat), cl::init(IPKind::default_val));

InstrumentationPolicy *createInstrumentationPolicy(IPKind k) {
  switch (k) {
  case IPKind::beforeOutflow:
    return new BeforeOutflowPolicy();
  case IPKind::accessOnly:
    return new AccessOnlyPolicy();
  case IPKind::none:
    return new NonePolicy();
  case IPKind::pointerBounds:
    return new PointerBoundsPolicy();
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
                          "place witnesses after inflow of the base value"),
               clEnumValN(WSKind::none, "none", "place no witnesses")),
    cl::cat(MemInstrumentCat), cl::init(WSKind::default_val));

WitnessStrategy *createWitnessStrategy(WSKind k) {
  switch (k) {
  case WSKind::after_inflow:
    return new AfterInflowStrategy();
  case WSKind::none:
    return new NoneStrategy();
  case WSKind::default_val:
    return nullptr;
  }
  llvm_unreachable("Invalid WitnessStrategy!");
}

const char *getModeName(MIMode M) {
  switch (M) {
  case MIMode::NOTHING:
    return "Nothing";
  case MIMode::SETUP:
    return "Setup";
  case MIMode::GATHER_ITARGETS:
    return "GatherITargets";
  case MIMode::FILTER_ITARGETS:
    return "FilterITargets";
  case MIMode::GENERATE_WITNESSES:
    return "GenerateWitnesses";
  case MIMode::GENERATE_INVARIANTS:
    return "GenerateInvariants";
  case MIMode::GENERATE_OPTIMIZATION_CHECKS:
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
    cl::values(clEnumValN(MIMode::NOTHING, "nothing",
                          "don't do anything for instrumentation, just have "
                          "the required passes run"),
               clEnumValN(MIMode::SETUP, "setup", "only until setup is done"),
               clEnumValN(MIMode::GATHER_ITARGETS, "gatheritargets",
                          "only until ITarget gathering is done"),
               clEnumValN(MIMode::FILTER_ITARGETS, "filteritargets",
                          "only until ITarget filtering is done"),
               clEnumValN(MIMode::GENERATE_WITNESSES, "genwitnesses",
                          "only until witness generation is done"),
               clEnumValN(MIMode::GENERATE_INVARIANTS, "geninvariants",
                          "only until invariant generation is done"),
               clEnumValN(MIMode::GENERATE_OPTIMIZATION_CHECKS, "genextchecks",
                          "only until external check generation is done"),
               clEnumValN(MIMode::GENERATE_CHECKS, "genchecks",
                          "the full pipeline")),
    cl::cat(MemInstrumentCat), cl::init(MIMode::DEFAULT));

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
                         cl::desc("default time that operations should take as "
                                  "a noop if not overwritten"),
                         cl::cat(MemInstrumentCat),
                         cl::init(0) // default
);

cl::opt<int>
    GenBoundsTime("mi-noop-time-gen-bounds",
                  cl::desc("time that generating bounds should take as a noop"),
                  cl::cat(MemInstrumentCat),
                  cl::init(-1) // default
    );

cl::opt<int> DerefCheckTime(
    "mi-noop-time-deref-check",
    cl::desc("time that a dereference check should take as a noop"),
    cl::cat(MemInstrumentCat),
    cl::init(-1) // default
);

cl::opt<int> InvarCheckTime(
    "mi-noop-time-invar-check",
    cl::desc("time that an inbounds check should take as a noop"),
    cl::cat(MemInstrumentCat),
    cl::init(-1) // default
);

cl::opt<int> StackAllocTime(
    "mi-noop-time-stack-alloc",
    cl::desc("time that a stack allocation should take as a noop"),
    cl::cat(MemInstrumentCat),
    cl::init(-1) // default
);

cl::opt<int>
    HeapAllocTime("mi-noop-time-heap-alloc",
                  cl::desc("time that a heap allocation should take as a noop"),
                  cl::cat(MemInstrumentCat),
                  cl::init(-1) // default
    );

cl::opt<int> GlobalAllocTime(
    "mi-noop-time-global-alloc",
    cl::desc("time that a global allocation should take as a noop"),
    cl::cat(MemInstrumentCat),
    cl::init(-1) // default
);

cl::opt<int> HeapFreeTime(
    "mi-noop-time-heap-free",
    cl::desc("time that a heap deallocation should take as a noop"),
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
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return true; }
  virtual bool hasInstrumentVerbose(void) const override { return false; }
  virtual const char *getName(void) const override { return "Splay"; }
};

/// A configuration to perform only instrumentation for external checks.
class OptimizationChecksOnlyConfig : public SplayConfig {
public:
  virtual ~OptimizationChecksOnlyConfig(void) {}

  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_OPTIMIZATION_CHECKS;
  }
  virtual const char *getName(void) const override {
    return "OptimizationChecksOnly";
  }
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
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return true; }
  virtual bool hasInstrumentVerbose(void) const override { return false; }
  virtual const char *getName(void) const override { return "Lowfat"; }
};

/// A configuration to perform instrumentation using SoftBound
class SoftBoundConfig : public Config {
public:
  virtual ~SoftBoundConfig(void) {}

  virtual IPKind getInstrumentationPolicy(void) const override {
    return IPKind::pointerBounds;
  }
  virtual IMKind getInstrumentationMechanism(void) const override {
    return IMKind::softbound;
  }
  virtual WSKind getWitnessStrategy(void) const override {
    return WSKind::after_inflow;
  }
  virtual MIMode getMIMode(void) const override {
    return MIMode::GENERATE_CHECKS;
  }
  virtual bool hasPrintWitnessGraph(void) const override { return false; }
  virtual bool hasSimplifyWitnessGraph(void) const override { return false; }
  virtual bool hasInstrumentVerbose(void) const override { return false; }
  virtual const char *getName(void) const override { return "SoftBound"; }
};

Config *Config::create(ConfigKind k) {
  switch (k) {
  case ConfigKind::splay:
    return new SplayConfig();
  case ConfigKind::rt_stat:
    return new RTStatConfig();
  case ConfigKind::noop:
    return new NoopConfig();
  case ConfigKind::optimization_checks_only:
    return new OptimizationChecksOnlyConfig();
  case ConfigKind::lowfat:
    return new LowfatConfig();
  case ConfigKind::softbound:
    return new SoftBoundConfig();
  case ConfigKind::default_val:
    return new DEFAULT_CONFIG();
  }
  llvm_unreachable("Invalid ConfigKind!");
}

} // namespace meminstrument

GlobalConfig::GlobalConfig(Config *Cfg, const Module &M) {

#define X_OR_DEFAULT(TYPE, X, Y) (((X) != TYPE::default_val) ? (X) : (Y))

  IMKind imk = X_OR_DEFAULT(IMKind, IMOpt, Cfg->getInstrumentationMechanism());
  instrumentationMechanism = createInstrumentationMechanism(*this, imk);

  IPKind ipk = X_OR_DEFAULT(IPKind, IPOpt, Cfg->getInstrumentationPolicy());
  instrumentationPolicy = createInstrumentationPolicy(ipk);

  WSKind wsk = X_OR_DEFAULT(WSKind, WSOpt, Cfg->getWitnessStrategy());
  witnessStrategy = createWitnessStrategy(wsk);

#undef X_OR_DEFAULT

  if (MIModeOpt != MIMode::DEFAULT) {
    mode = MIModeOpt;
  } else {
    mode = Cfg->getMIMode();
  }

  printWitnessGraph =
      getValOrDefault(PrintWitnessGraphOpt, Cfg->hasPrintWitnessGraph());
  simplifyWitnessGraph =
      getValOrDefault(SimplifyWitnessGraphOpt, Cfg->hasSimplifyWitnessGraph());
  instrumentVerbose =
      getValOrDefault(InstrumentVerboseOpt, Cfg->hasInstrumentVerbose());

  configName = Cfg->getName();

  delete Cfg;
}

#define GET_NOOP_METHOD(x)                                                     \
  uint32_t GlobalConfig::getNoop##x##Time(void) {                              \
    return getNoopValOrDefault(x##Time);                                       \
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

std::unique_ptr<GlobalConfig> GlobalConfig::create(const Module &M) {
  auto GlobalCfg = std::unique_ptr<GlobalConfig>(
      new GlobalConfig(Config::create(ConfigKindOpt), M));
  LLVM_DEBUG(dbgs() << "Creating MemInstrument Config:\n";
             GlobalCfg->dump(dbgs()););
  return GlobalCfg;
}

void GlobalConfig::dump(raw_ostream &Stream) const {
  Stream << "{{{ Config: " << configName << "\n";
  Stream << "     InstrumentationPolicy: " << instrumentationPolicy->getName()
         << '\n';
  Stream << "           WitnessStrategy: " << witnessStrategy->getName()
         << '\n';
  Stream << "  InstrumentationMechanism: "
         << instrumentationMechanism->getName() << '\n';
  Stream << "                      Mode: " << getModeName(mode) << '\n';
  Stream << "         PrintWitnessGraph: " << printWitnessGraph << '\n';
  Stream << "      SimplifyWitnessGraph: " << simplifyWitnessGraph << '\n';
  Stream << "         InstrumentVerbose: " << instrumentVerbose << '\n';
  Stream << "}}}\n\n";
}
