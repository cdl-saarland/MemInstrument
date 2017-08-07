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

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

using namespace meminstrument;
using namespace llvm;

WitnessGraphNode *TodoBetterNameStrategy::constructWitnessGraph(
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
    case Instruction::IntToPtr: // FIXME is this what we want?
      break;

    case Instruction::PHI: {
      auto *PtrPhi = cast<PHINode>(I);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);

        auto NewTarget =
            std::make_shared<ITarget>(InVal, &InBB->back(), Target->AccessSize);
        Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      }
      // TODO
      break;
    }

    case Instruction::Select: {
      auto *PtrSelect = cast<SelectInst>(I);
      auto *TrueVal = PtrSelect->getTrueValue();
      auto *FalseVal = PtrSelect->getFalseValue();

      auto TrueTarget =
          std::make_shared<ITarget>(TrueVal, I, Target->AccessSize);
      Node->Requirements.push_back(constructWitnessGraph(WG, TrueTarget));

      auto FalseTarget =
          std::make_shared<ITarget>(FalseVal, I, Target->AccessSize);
      Node->Requirements.push_back(constructWitnessGraph(WG, FalseTarget));
      break;
    }

    case Instruction::GetElementPtr: {
      auto *Operand = cast<GetElementPtrInst>(I)->getPointerOperand();
      auto NewTarget =
          std::make_shared<ITarget>(Operand, I, Target->AccessSize);
      Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      break;
    }

    case Instruction::BitCast: {
      // bit casts don't change the pointer bounds
      auto *Operand = cast<BitCastInst>(I)->getOperand(0);
      auto NewTarget =
          std::make_shared<ITarget>(Operand, I, Target->AccessSize);
      Node->Requirements.push_back(constructWitnessGraph(WG, NewTarget));
      break;
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(Target->Instrumentee)) {
    // return;
  } else if (isa<GlobalValue>(Target->Instrumentee)) {
    // llvm_unreachable("Global Values are not yet supported!"); // FIXME
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }

  return Node;
}

/*
Value *BeforeOutflowPolicy::createWitness(Value *Instrumentee) {
  if (auto *I = dyn_cast<Instruction>(Instrumentee)) {
    IRBuilder<> Builder(I->getNextNode());
    // insert code after the source instruction
    switch (I->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::Load:
      // the assumption is that the validity of these has already been checked
      return insertGetBoundWitness(Builder, I);

    case Instruction::PHI: {
      auto *PtrPhi = cast<PHINode>(I);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      auto *NewType = getWitnessType();
      auto *NewPhi = Builder.CreatePHI(NewType, NumOperands); // TODO give name
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);
        auto *NewInVal = createWitness(InVal);
        NewPhi->addIncoming(NewInVal, InBB);
      }
      return NewPhi;
    }

    case Instruction::Select: {
      auto *PtrSelect = cast<SelectInst>(I);
      auto *Condition = PtrSelect->getCondition();

      auto *TrueVal = createWitness(PtrSelect->getTrueValue());
      auto *FalseVal = createWitness(PtrSelect->getFalseValue());

      Builder.CreateSelect(Condition, TrueVal, FalseVal); // TODO give name
    }

    case Instruction::GetElementPtr: {
      // pointer arithmetic doesn't change the pointer bounds
      auto *Operand = cast<GetElementPtrInst>(I);
      return createWitness(Operand->getPointerOperand());
    }

    case Instruction::IntToPtr: {
      // FIXME is this what we want?
      return insertGetBoundWitness(Builder, I);
    }

    case Instruction::BitCast: {
      // bit casts don't change the pointer bounds
      auto *Operand = cast<BitCastInst>(I);
      return createWitness(Operand->getOperand(0));
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(Instrumentee)) {
    // should have already been checked at the call site
    return insertGetBoundWitness(Builder, Instrumentee);
  } else if (isa<GlobalValue>(Instrumentee)) {
    llvm_unreachable("Global Values are not yet supported!"); // FIXME
    // return insertGetBoundWitness(Builder, Instrumentee);
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }
}

void BeforeOutflowPolicy::instrumentFunction(llvm::Function &Func,
                                             std::vector<ITarget> &Targets) {
  for (auto &BB : Func) {
    for (auto &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        handleAlloca(AI);
      }
    }
  }
  // TODO when to change allocas?

  for (const auto &IT : Targets) {
    if (IT.CheckTemporalFlag)
      llvm_unreachable("Temporal checks are not supported by this policy!");

    if (!(IT.CheckLowerBoundFlag || IT.CheckUpperBoundFlag))
      continue; // nothing to do!

    auto *Witness = createWitness(IT.Instrumentee);
    IRBuilder<> Builder(IT.Location);

    if (IT.CheckLowerBoundFlag && IT.CheckUpperBoundFlag) {
      insertCheckBoundWitness(Builder, IT.Instrumentee, Witness);
    } else if (IT.CheckUpperBoundFlag) {
      insertCheckBoundWitnessUpper(Builder, IT.Instrumentee, Witness);
    } else {
      insertCheckBoundWitnessLower(Builder, IT.Instrumentee, Witness);
    }
  }
}
*/
