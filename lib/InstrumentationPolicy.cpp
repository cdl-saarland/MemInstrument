//===----- InstrumentationPolicy.cpp -- MemSafety Instrumentation ---------===//
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

namespace {

enum InstrumentationPolicyKind {
  IP_beforeOutflow,
};

cl::opt<InstrumentationPolicyKind> InstrumentationPolicyOpt(
    "memsafety-ipolicy",
    cl::desc("Choose InstructionPolicy: (default: before-outflow)"),
    cl::values(clEnumValN(IP_beforeOutflow, "before-outflow",
                          "only insert dummy calls for instrumentation")),
    cl::init(IP_beforeOutflow) // default
    );

std::unique_ptr<InstrumentationPolicy> GlobalIM(nullptr);
}

InstrumentationPolicy &InstrumentationPolicy::get(const DataLayout &DL) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationPolicyOpt) {
    case IP_beforeOutflow:
      GlobalIM.reset(new BeforeOutflowPolicy(DL));
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}

size_t BeforeOutflowPolicy::getPointerAccessSize(llvm::Value *V) {
  auto *Ty = V->getType();
  assert(Ty->isPointerTy() && "Only pointer types allowed!");

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    // FIXME
    DEBUG(dbgs() << "treating function pointer with access size 0: `"
                 << V->getName() << "`\n";);
    return 0;
  }

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}

void BeforeOutflowPolicy::classifyTargets(
    std::vector<std::shared_ptr<ITarget>> &Destination,
    llvm::Instruction *Location) {
  switch (Location->getOpcode()) {
  case Instruction::Ret: {
    llvm::ReturnInst *I = llvm::cast<llvm::ReturnInst>(Location);

    auto *Operand = I->getReturnValue();
    if (!Operand || !Operand->getType()->isPointerTy()) {
      return;
    }

    Destination.push_back(std::make_shared<ITarget>(
        Operand, Location, getPointerAccessSize(Operand)));
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

      Destination.push_back(std::make_shared<ITarget>(
          Operand, Location, getPointerAccessSize(Operand)));
    }

    break;
  }
  case Instruction::Load: {
    llvm::LoadInst *I = llvm::cast<llvm::LoadInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(std::make_shared<ITarget>(
        PtrOperand, Location, getPointerAccessSize(PtrOperand)));
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *I = llvm::cast<llvm::StoreInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(std::make_shared<ITarget>(
        PtrOperand, Location, getPointerAccessSize(PtrOperand)));

    auto *StoreOperand = I->getValueOperand();
    if (!StoreOperand->getType()->isPointerTy()) {
      return;
    }

    Destination.push_back(std::make_shared<ITarget>(
        StoreOperand, Location, getPointerAccessSize(StoreOperand)));

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
