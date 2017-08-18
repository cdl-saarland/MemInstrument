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

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

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
  Node->Requirements.push_back(NewNode);
}

void WitnessStrategy::requireSource(WitnessGraphNode *Node, Value *Req,
                                    Instruction *Loc) const {
  auto &WG = Node->Graph;
  auto *NewNode = getInternalNode(WG, Req, Loc);
  NewNode->HasAllRequirements = true;
  if (NewNode != Node) // FIXME?
    Node->Requirements.push_back(NewNode);
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
      auto *PhiNode = getInternalNode(WG, I, I->getNextNode());
      Node->Requirements.push_back(PhiNode);
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

  if (isa<GlobalValue>(Target->Instrumentee)) {
    // Generate witnesses for globals right when we need them. This might be
    // inefficient.
    return;
  }

  if (auto *E = dyn_cast<ConstantExpr>(Target->Instrumentee)) {
    auto *I = E->getAsInstruction();
    switch (E->getOpcode()) {
      case Instruction::GetElementPtr: {
          // GEP ConstantExpr's can only refer to global values, therefore we
          // can materialize bounds for them just like for global values
          // (and do not need to follow their operands, which is more ugly).
          // TODO verify this guess!
        break;
      }
      default: // TODO constant select, etc.?
        llvm_unreachable("Unsupported constant expression operand!");
    }
    delete (I);
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

  if (Node->Requirements.size() == 0) {
    // We assume that this Node corresponds to a valid pointer, so we create a
    // new witness for it.
    IM.insertWitness(*(Node->Target));
    return;
  }

  if (Node->Requirements.size() == 1) {
    // We just use the witness of the single requirement, nothing to combine.
    auto *Requirement = Node->Requirements[0];
    createWitness(IM, Requirement);
    Node->Target->BoundWitness = Requirement->Target->BoundWitness;
    return;
  }

  auto *Instrumentee = Node->Target->Instrumentee;
  if (auto *Phi = dyn_cast<PHINode>(Instrumentee)) {
    // Insert new phis that use the witnesses of the operands of the
    // instrumentee. This has to happen in two separate steps to break loops.
    assert(Node->Requirements.size() == Phi->getNumIncomingValues());

    // First insert phis with no arguments.
    auto PhiWitness = IM.insertWitnessPhi(*(Node->Target));

    // Now, add the corresponding incoming values for the operands.
    unsigned int i = 0;
    for (auto *ReqNode : Node->Requirements) {
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
    assert(Node->Requirements.size() == 2);

    for (auto *ReqNode : Node->Requirements) {
      createWitness(IM, ReqNode);
    }

    IM.insertWitnessSelect(*(Node->Target),
                           Node->Requirements[0]->Target->BoundWitness,
                           Node->Requirements[1]->Target->BoundWitness);

    return;
  }

  // There should be no way to end up here.
  llvm_unreachable("Invalid Node");
}
