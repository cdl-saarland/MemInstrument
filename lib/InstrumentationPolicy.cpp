//===----- InstrumentationPolicy.cpp -- Memory Instrumentation ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/InstrumentationPolicy.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

size_t BeforeOutflowPolicy::getPointerAccessSize(llvm::Value *V) {
  auto *Ty = V->getType();
  assert(Ty->isPointerTy() && "Only pointer types allowed!");

  auto *PointeeType = Ty->getPointerElementType();

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}

void BeforeOutflowPolicy::classifyTargets(std::vector<ITarget> &Destination,
                                          llvm::Instruction *Location) {
  switch (Location->getOpcode()) {
  case Instruction::Ret: {
    llvm::ReturnInst *I = llvm::cast<llvm::ReturnInst>(Location);

    auto *Operand = I->getReturnValue();
    if (!Operand || !Operand->getType()->isPointerTy()) {
      return;
    }

    Destination.emplace_back(Operand, Location, getPointerAccessSize(Operand));
    break;
  }
  case Instruction::Call: {
    llvm::CallInst *I = llvm::cast<llvm::CallInst>(Location);

    if (!I->getCalledFunction()) { // call via function pointer
      // TODO implement
      DEBUG(dbgs() << "skipping indirect function call " << Location->getName()
                   << "\n";);
    }

    for (unsigned i = 0; i < I->getNumArgOperands(); ++i) {
      auto *Operand = I->getArgOperand(i);
      if (!Operand->getType()->isPointerTy()) {
        continue;
      }

      Destination.emplace_back(Operand, Location,
                               getPointerAccessSize(Operand));
    }

    break;
  }
  case Instruction::Load: {
    llvm::LoadInst *I = llvm::cast<llvm::LoadInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.emplace_back(PtrOperand, Location,
                             getPointerAccessSize(PtrOperand));
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *I = llvm::cast<llvm::StoreInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.emplace_back(PtrOperand, Location,
                             getPointerAccessSize(PtrOperand));

    auto *StoreOperand = I->getValueOperand();
    if (!StoreOperand->getType()->isPointerTy()) {
      return;
    }

    Destination.emplace_back(StoreOperand, Location,
                             getPointerAccessSize(StoreOperand));

    break;
  }

  case Instruction::Invoke:
  case Instruction::Resume:
  case Instruction::IndirectBr:
  case Instruction::AtomicCmpXchg:
  case Instruction::AtomicRMW:
    // TODO implement
    DEBUG(dbgs() << "skipping unimplemented instruction " << Location->getName()
                 << "\n";);
    break;

  default:
    break;
  }
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
