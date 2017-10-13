//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/WitnessStrategy.h"

#include "meminstrument/witness_strategies/AfterInflowStrategy.h"
#include "meminstrument/witness_strategies/NoneStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {

enum WitnessStrategyKind {
  WS_after_inflow,
  WS_none,
};

cl::opt<WitnessStrategyKind> WitnessStrategyOpt(
    "mi-wstrategy", cl::desc("Choose WitnessStrategy: (default: after-inflow)"),
    cl::values(clEnumValN(WS_after_inflow, "after-inflow",
                          "place witnesses after inflow of the base value")),
    cl::values(clEnumValN(WS_none, "none",
                          "place no witnesses")),
    cl::init(WS_after_inflow) // default
);

std::unique_ptr<WitnessStrategy> GlobalWS(nullptr);
} // namespace

WitnessGraphNode *WitnessStrategy::getInternalNode(WitnessGraph &WG, llvm::Value *Instrumentee,
                                  llvm::Instruction *Location) {
  // Flags do not matter here as they are propagated later in propagateFlags()
  auto NewTarget = std::make_shared<ITarget>(Instrumentee, Location, 0, false,
                                             false, false, false);
  return WG.getInternalNode(NewTarget);
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

void WitnessStrategy::requireRecursively(WitnessGraphNode *Node, Value *Req,
                                         Instruction *Loc) const {
  auto &WG = Node->Graph;

  auto *NewNode = getInternalNode(WG, Req, Loc);
  addRequired(NewNode);
  Node->addRequirement(NewNode);
}

void WitnessStrategy::requireSource(WitnessGraphNode *Node, Value *Req,
                                    Instruction *Loc) const {
  auto &WG = Node->Graph;
  auto *NewNode = getInternalNode(WG, Req, Loc);
  NewNode->HasAllRequirements = true;
  if (NewNode != Node) // FIXME?
    Node->addRequirement(NewNode);
}
