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

cl::opt<WitnessStrategyKind> WitnessStrategyOpt("memsafety-wstrategy",
    cl::desc("Choose WitnessStrategy: (default: simple)"),
    cl::values(
      clEnumValN(WS_simple, "simple", "simple strategy for witness placement")
    ),
    cl::init(WS_simple) // default
  );

std::unique_ptr<WitnessStrategy> GlobalWS(nullptr);

}

const WitnessStrategy &WitnessStrategy::get(void) {
  auto* Res = GlobalWS.get();
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

WitnessGraphNode *SimpleStrategy::constructWitnessGraph(
    WitnessGraph &WG, std::shared_ptr<ITarget> Target) const {
  auto *Node = WG.getNodeForOrNull(Target);

  if (Node != nullptr) {
    return Node;
  }

  Node = WG.createNewNodeFor(Target);

  if (auto *I = dyn_cast<Instruction>(Target->Instrumentee)) {
    switch (I->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::Load:
    case Instruction::IntToPtr: {
      // Assume that what we get from these is valid. Mark these to materialize
      // new witnesses.
      auto *NewNode = WG.getNodeFor(
          mkTempTarget(Target->Instrumentee, I->getNextNode(), *Target));
      NewNode->ToMaterialize = true;
      Node->Requirements.push_back(NewNode);
      break;
    }

    case Instruction::PHI: {
      // We might have to introduce a PHI for the corresponding witnesses.
      auto *PtrPhi = cast<PHINode>(I);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);

        auto NewTarget = mkTempTarget(InVal, &InBB->back(), *Target);
        Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      }
      break;
    }

    case Instruction::Select: {
      // We might have to introduce a Select for the corresponding witnesses.
      auto *PtrSelect = cast<SelectInst>(I);

      auto TrueTarget = mkTempTarget(PtrSelect->getTrueValue(), I, *Target);
      Node->Requirements.push_back(constructWitnessGraph(WG, TrueTarget));

      auto FalseTarget = mkTempTarget(PtrSelect->getFalseValue(), I, *Target);
      Node->Requirements.push_back(constructWitnessGraph(WG, FalseTarget));
      break;
    }

    case Instruction::GetElementPtr: {
      auto *Operand = cast<GetElementPtrInst>(I)->getPointerOperand();
      auto NewTarget = mkTempTarget(Operand, I, *Target);
      Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      break;
    }

    case Instruction::BitCast: {
      auto *Operand = cast<BitCastInst>(I)->getOperand(0);
      auto NewTarget = mkTempTarget(Operand, I, *Target);
      Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      break;
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(Target->Instrumentee)) {
    // Generate witnesses for arguments right when we need them. Other
    // approaches might also be desirable.
    Node->ToMaterialize = true;
  } else if (isa<GlobalValue>(Target->Instrumentee)) {
    // Generate witnesses for globals right when we need them. Other approaches
    // might also be desirable.
    Node->ToMaterialize = true;
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }

  return Node;
}

void SimpleStrategy::createWitness(InstrumentationMechanism &IM, WitnessGraphNode *Node) const {
  if (Node->Target->hasWitness()) {
    return;
  }
  if (Node->ToMaterialize) {
    IM.insertWitness(*(Node->Target));
    return;
  }
  auto *Instrumentee = Node->Target->Instrumentee;
  if (auto *Phi = dyn_cast_or_null<PHINode>(Instrumentee)) {
    assert(Node->Requirements.size() == Phi->getNumIncomingValues());

    auto PhiWitness = IM.insertWitnessPhi(*(Node->Target));

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
    assert(Node->Requirements.size() == 2);

    for (auto *ReqNode : Node->Requirements) {
      createWitness(IM, ReqNode);
    }

    IM.insertWitnessSelect(*(Node->Target),
                           Node->Requirements[0]->Target->BoundWitness,
                           Node->Requirements[1]->Target->BoundWitness);

    return;
  }
  assert(Node->Requirements.size() == 1);
  Node->Target->BoundWitness = Node->Requirements[0]->Target->BoundWitness;
}
