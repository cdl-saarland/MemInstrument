//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/Config.h"

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
    cl::values(clEnumValN(MIM_SETUP, "setup", "only until setup is done")),
    cl::values(clEnumValN(MIM_GATHER_ITARGETS, "gatheritargets",
                          "only until ITarget gathering is done")),
    cl::values(clEnumValN(MIM_FILTER_ITARGETS, "filteritargets",
                          "only until ITarget filtering is done")),
    cl::values(clEnumValN(MIM_GENERATE_WITNESSES, "genwitnesses",
                          "only until witness generation is done")),
    cl::values(clEnumValN(MIM_GENERATE_EXTERNAL_CHECKS, "genextchecks",
                          "only until external check generation is done")),
    cl::values(clEnumValN(MIM_GENERATE_CHECKS, "genchecks",
                          "the full pipeline")),
    cl::cat(MemInstrumentCat),
    cl::init(MIM_GENERATE_CHECKS)
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

cl::opt<boolOrDefault> NoFiltersOpt(
    "mi-no-filter",
    cl::desc("Disable all memsafety instrumentation target filters"),
    cl::cat(MemInstrumentCat),
    );

cl::opt<boolOrDefault>
    MIUseExternalChecksOpt("mi-use-extchecks",
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

} // namespace


InstrumentationMechanism &InstrumentationMechanism::get(void) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationMechanismOpt) {
    case IM_dummy:
      GlobalIM.reset(new DummyMechanism());
      break;

    case IM_splay:
      GlobalIM.reset(new SplayMechanism());
      break;

    case IM_rt_stat:
      GlobalIM.reset(new RuntimeStatMechanism());
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}

InstrumentationPolicy &InstrumentationPolicy::get(const DataLayout &DL) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationPolicyOpt) {
    case IP_beforeOutflow:
      GlobalIM.reset(new BeforeOutflowPolicy(DL));
      break;
    case IP_accessOnly:
      GlobalIM.reset(new AccessOnlyPolicy(DL));
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}


const WitnessStrategy &WitnessStrategy::get(void) {
  auto *Res = GlobalWS.get();
  if (Res == nullptr) {
    switch (WitnessStrategyOpt) {
    case WS_after_inflow:
      GlobalWS.reset(new AfterInflowStrategy());
      break;
    case WS_none:
      GlobalWS.reset(new NoneStrategy());
      break;
    }
    Res = GlobalWS.get();
  }
  return *Res;
}


static Config &Config::get(void) {

}

