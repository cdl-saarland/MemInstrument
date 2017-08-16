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

#include "meminstrument/InstrumentationMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

using namespace meminstrument;
using namespace llvm;

namespace {

std::shared_ptr<ITarget> mkTempTarget(llvm::Value *Instrumentee,
                                      llvm::Instruction *Location,
                                      ITarget &Target) {
  return std::make_shared<ITarget>(
      Instrumentee, Location, Target.AccessSize, Target.CheckUpperBoundFlag,
      Target.CheckLowerBoundFlag, Target.CheckTemporalFlag,
      Target.RequiresExplicitBounds);
}

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
      auto NewTarget =
          mkTempTarget(Target->Instrumentee, I->getNextNode(), *Target);
      auto *NewNode = WG.getInternalNode(NewTarget);
      if (NewNode != Node) // FIXME
        Node->Requirements.push_back(NewNode);
      break;
    }

    case Instruction::PHI: {
      // Introduce a target for the phi. This breaks loops.
      auto PhiTarget = mkTempTarget(I, I->getNextNode(), *Target);
      auto *PhiNode = WG.getInternalNode(PhiTarget);
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

        auto NewTarget = mkTempTarget(InVal, &InBB->back(), *Target);
        auto *NewNode = WG.getInternalNode(NewTarget);
        addRequired(NewNode);
        PhiNode->Requirements.push_back(NewNode);
      }
      break;
    }

    case Instruction::Select: {
      // A select target needs witnesses of both its arguments.
      auto *PtrSelect = cast<SelectInst>(I);

      auto TrueTarget = mkTempTarget(PtrSelect->getTrueValue(), I, *Target);
      auto *TrueNode = WG.getInternalNode(TrueTarget);
      addRequired(TrueNode);
      Node->Requirements.push_back(TrueNode);

      auto FalseTarget = mkTempTarget(PtrSelect->getFalseValue(), I, *Target);
      auto *FalseNode = WG.getInternalNode(FalseTarget);
      addRequired(FalseNode);
      Node->Requirements.push_back(FalseNode);
      break;
    }

    case Instruction::GetElementPtr: {
      // A GEP target requires only the witness of its argument.
      auto *Operand = cast<GetElementPtrInst>(I)->getPointerOperand();
      auto NewTarget = mkTempTarget(Operand, I, *Target);
      auto *NewNode = WG.getInternalNode(NewTarget);
      addRequired(NewNode);
      Node->Requirements.push_back(NewNode);
      break;
    }

    case Instruction::BitCast: {
      // A bitcast target requires only the witness of its argument.
      auto *Operand = cast<BitCastInst>(I)->getOperand(0);
      auto NewTarget = mkTempTarget(Operand, I, *Target);
      auto *NewNode = WG.getInternalNode(NewTarget);
      addRequired(NewNode);
      Node->Requirements.push_back(NewNode);
      break;
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(Target->Instrumentee)) {
    // Generate witnesses for arguments right when we need them. This might be
    // inefficient.
  } else if (isa<GlobalValue>(Target->Instrumentee)) {
    // Generate witnesses for globals right when we need them. This might be
    // inefficient.
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }
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
