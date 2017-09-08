//===---------- WitnessStrategy.cpp -- MemSafety Instrumentation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/WitnessStrategy.h"

#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

#include <sstream>

using namespace meminstrument;
using namespace llvm;

namespace {

enum WitnessStrategyKind {
  WS_simple,
};

cl::opt<WitnessStrategyKind> WitnessStrategyOpt(
    "memsafety-wstrategy",
    cl::desc("Choose WitnessStrategy: (default: simple)"),
    cl::values(clEnumValN(WS_simple, "simple",
                          "simple strategy for witness placement")),
    cl::init(WS_simple) // default
    );

std::unique_ptr<WitnessStrategy> GlobalWS(nullptr);

WitnessGraphNode *getInternalNode(WitnessGraph &WG, llvm::Value *Instrumentee,
                                  llvm::Instruction *Location) {
  // Flags do not matter here as they are propagated later in propagateFlags()
  auto NewTarget = std::make_shared<ITarget>(Instrumentee, Location, 0, false,
                                             false, false, false);
  return WG.getInternalNode(NewTarget);
}

void getPointerOperands(std::vector<Value *> &Results, llvm::Constant *C) {
  if (!C->getType()->isPointerTy()) {
    llvm_unreachable("getPointerOperands() called for non-pointer constant!");
  }

  if (auto *GV = dyn_cast<GlobalValue>(C)) {
    Results.push_back(GV);
    return;
  }

  if (auto *CD = dyn_cast<ConstantData>(C)) {
    Results.push_back(CD);
    return;
  }

  if (auto *CE = dyn_cast<ConstantExpr>(C)) {
    switch (CE->getOpcode()) {
    case Instruction::GetElementPtr:
      getPointerOperands(Results, CE->getOperand(0)); // pointer argument
      break;
    case Instruction::Select:
      getPointerOperands(Results, CE->getOperand(1)); // true operand
      getPointerOperands(Results, CE->getOperand(2)); // false operand
      llvm_unreachable("Select constant expression!");
      break;
    case Instruction::BitCast:
      getPointerOperands(Results, CE->getOperand(0)); // pointer argument
      break;
    default:
      llvm_unreachable("Unsupported constant expression!");
    }

    return;
  }

  llvm_unreachable("Unsupported constant value!");
}
}

const WitnessStrategy &WitnessStrategy::get(void) {
  auto *Res = GlobalWS.get();
  if (Res == nullptr) {
    switch (WitnessStrategyOpt) {
    case WS_simple:
      GlobalWS.reset(new SimpleStrategy());
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

void SimpleStrategy::addRequired(WitnessGraphNode *Node) const {
  if (Node->HasAllRequirements) {
    return;
  }

  Node->HasAllRequirements = true;

  auto &WG = Node->Graph;

  auto &Target = Node->Target;
  if (auto *I = dyn_cast<Instruction>(Target->Instrumentee)) {
    switch (I->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::Load:
    case Instruction::IntToPtr: {
      // Introduce a target without requirements for these values. We assume
      // that these are valid pointers.
      requireSource(Node, I, I->getNextNode());
      return;
    }

    case Instruction::PHI: {
      // Introduce a target for the phi. This breaks loops consistently.
      // Setting the location to I (and not after I) makes sure that this
      // particular internal Node is only "gotten" when handling Phis here.
      auto *PhiNode = getInternalNode(WG, I, I);
      Node->addRequirement(PhiNode);
      if (PhiNode->HasAllRequirements) {
        return;
      }
      PhiNode->HasAllRequirements = true;

      // The phi target requires witnesses of its incoming values.
      auto *PtrPhi = cast<PHINode>(I);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);
        requireRecursively(PhiNode, InVal, &InBB->back());
      }
      return;
    }

    case Instruction::Select: {
      // A select target needs witnesses of both its arguments.
      auto *PtrSelect = cast<SelectInst>(I);
      requireRecursively(Node, PtrSelect->getTrueValue(), I);
      requireRecursively(Node, PtrSelect->getFalseValue(), I);
      return;
    }

    case Instruction::GetElementPtr: {
      // A GEP target requires only the witness of its argument.
      auto *Operand = cast<GetElementPtrInst>(I)->getPointerOperand();
      requireRecursively(Node, Operand, I);
      return;
    }

    case Instruction::BitCast: {
      // A bitcast target requires only the witness of its argument.
      auto *Operand = cast<BitCastInst>(I)->getOperand(0);
      requireRecursively(Node, Operand, I);
      return;
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }
  }

  if (isa<Argument>(Target->Instrumentee)) {
    // Generate witnesses for arguments right when we need them. This might be
    // inefficient.
    return;
  }

  if (auto *C = dyn_cast<Constant>(Target->Instrumentee)) {
    std::vector<Value *> Pointers;
    getPointerOperands(Pointers, C);
    for (auto *V : Pointers) {
      // Generate witnesses for globals and constants right when we need them.
      // This might be inefficient.
      requireSource(Node, V, Target->Location);
      // TODO this location might be sub-optimal
      // TODO do we really want to have witnesses for constant null pointers?
    }
    return;
  }

  llvm_unreachable("Unsupported value operand!");
}

void SimpleStrategy::createWitness(InstrumentationMechanism &IM,
                                   WitnessGraphNode *Node) const {
  if (Node->Target->hasWitness()) {
    // We already handled this node.
    return;
  }

  if (Node->getRequiredNodes().size() == 0) {
    // We assume that this Node corresponds to a valid pointer, so we create a
    // new witness for it.
    IM.insertWitness(*(Node->Target));
    return;
  }

  if (Node->getRequiredNodes().size() == 1) {
    // We just use the witness of the single requirement, nothing to combine.
    auto *Requirement = Node->getRequiredNodes()[0];
    createWitness(IM, Requirement);
    Node->Target->BoundWitness = Requirement->Target->BoundWitness;
    return;
  }

  auto *Instrumentee = Node->Target->Instrumentee;
  if (auto *Phi = dyn_cast<PHINode>(Instrumentee)) {
    // Insert new phis that use the witnesses of the operands of the
    // instrumentee. This has to happen in two separate steps to break loops.
    assert(Node->getRequiredNodes().size() == Phi->getNumIncomingValues());

    // First insert phis with no arguments.
    auto PhiWitness = IM.insertWitnessPhi(*(Node->Target));

    // Now, add the corresponding incoming values for the operands.
    unsigned int i = 0;
    for (auto *ReqNode : Node->getRequiredNodes()) {
      createWitness(IM, ReqNode);
      auto *BB = Phi->getIncomingBlock(i);
      IM.addIncomingWitnessToPhi(PhiWitness, ReqNode->Target->BoundWitness, BB);
      i++;
    }
    return;
  }

  if (isa<SelectInst>(Instrumentee)) {
    // Insert new selects that use the witnesses of the operands of the
    // instrumentee.
    assert(Node->getRequiredNodes().size() == 2);

    for (auto *ReqNode : Node->getRequiredNodes()) {
      createWitness(IM, ReqNode);
    }

    IM.insertWitnessSelect(*(Node->Target),
                           Node->getRequiredNodes()[0]->Target->BoundWitness,
                           Node->getRequiredNodes()[1]->Target->BoundWitness);

    return;
  }

  // There should be no way to end up here.
  llvm_unreachable("Invalid Node");
}

// #define DEBUG_WG_SIMPLIFY 1

namespace {
void updateWitnessNode(std::map<WitnessGraphNode *, WitnessGraphNode *> &ReqMap,
                       WitnessGraphNode *N) {
  assert(N != nullptr);
  WitnessGraphNode *ValBefore = ReqMap.at(N);
  if (N->getRequiredNodes().size() == 0) {
    ReqMap.at(N) = N;
  } else {
    WitnessGraphNode *Reference = nullptr;
    bool AllSame = true;

    for (auto &N2 : N->getRequiredNodes()) {
      auto *Req = ReqMap.at(N2);
      if (Reference == nullptr) {
        Reference = Req;
      } else if (Req != nullptr && Reference != Req) {
        AllSame = false;
        break;
      }
    }
    if (AllSame) {
      ReqMap.at(N) = Reference;
    } else {
      ReqMap.at(N) = N;
    }
  }

#ifdef DEBUG_WG_SIMPLIFY
  static int cnt = 0;
  std::stringstream ss;
  ss << "simplifyStep." << ++cnt << ".dot";
  N->Graph.dumpDotGraph(ss.str(), ReqMap);
#endif

  if (ReqMap.at(N) != ValBefore) {
    for (auto *other : N->getRequiringNodes()) {
      updateWitnessNode(ReqMap, other);
    }
  }
}
}

void SimpleStrategy::simplifyWitnessGraph(WitnessGraph &WG) const {
  std::map<WitnessGraphNode *, WitnessGraphNode *> ReqMap;
  std::vector<WitnessGraphNode *> Roots;

  WG.map([&](WitnessGraphNode *N) {
    ReqMap.insert(std::make_pair(N, nullptr));
    if (N->getRequiredNodes().size() == 0) {
      Roots.push_back(N);
    }
  });

  for (auto *N : Roots) {
    updateWitnessNode(ReqMap, N);
  }

  WG.map([&](WitnessGraphNode *N) {
    auto *Req = ReqMap.at(N);
    assert(Req != nullptr);

    if (Req != N) {
      N->clearRequirements();
      N->addRequirement(Req);
    }

  });
  WG.removeDeadNodes();
}
