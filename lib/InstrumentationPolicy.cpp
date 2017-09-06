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

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

#include "meminstrument/Util.h"

STATISTIC(NumITargetsGathered,
          "The # of instrumentation targets initially gathered");

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
        Operand, Location, getPointerAccessSize(Operand), /*CheckUpper*/ false,
        /*CheckLower*/ false, /*ExplicitBounds*/ false));
    ++NumITargetsGathered;
    break;
  }
  case Instruction::Call: {
    llvm::CallInst *I = llvm::cast<llvm::CallInst>(Location);

    auto *Fun = I->getCalledFunction();
    if (!Fun) { // call via function pointer
      Destination.push_back(std::make_shared<ITarget>(
          I->getCalledValue(), Location, 1, /*CheckUpper*/ true,
          /*CheckLower*/ true, /*ExplicitBounds*/ false));
      ++NumITargetsGathered;
    }
    if (Fun && Fun->hasName() && Fun->getName().startswith("llvm.dbg.")) {
      // skip debug information pseudo-calls
      break;
    }

    bool FunIsNoVarArg = Fun && !Fun->isVarArg();
    // FIXME If we know the function, we can insert actual dereference checks
    // for byval arguments. Otherwise, we have to hope for the best.
    Function::arg_iterator ArgIt;
    if (FunIsNoVarArg) {
      ArgIt = Fun->arg_begin();
    }
    for (auto &Operand : I->arg_operands()) {
      if (!Operand->getType()->isPointerTy()) {
        if (FunIsNoVarArg) {
          ++ArgIt;
        }
        continue;
      }
      if (Operand->getType()->isMetadataTy()) {
        // skip metadata arguments
        if (FunIsNoVarArg) {
          ++ArgIt;
        }
        continue;
      }

      bool isByVal = FunIsNoVarArg && ArgIt->hasByValAttr();

      Destination.push_back(std::make_shared<ITarget>(
          Operand, Location, getPointerAccessSize(Operand),
          /*CheckUpper*/ isByVal, /*CheckLower*/ isByVal,
          /*ExplicitBounds*/ false));
      ++NumITargetsGathered;

      if (FunIsNoVarArg) {
        ++ArgIt;
      }
    }

    break;
  }
  case Instruction::Load: {
    llvm::LoadInst *I = llvm::cast<llvm::LoadInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(std::make_shared<ITarget>(
        PtrOperand, Location, getPointerAccessSize(PtrOperand),
        /*CheckUpper*/ true, /*CheckLower*/ true, /*ExplicitBounds*/ false));
    ++NumITargetsGathered;
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *I = llvm::cast<llvm::StoreInst>(Location);
    auto *PtrOperand = I->getPointerOperand();
    Destination.push_back(std::make_shared<ITarget>(
        PtrOperand, Location, getPointerAccessSize(PtrOperand),
        /*CheckUpper*/ true, /*CheckLower*/ true, /*ExplicitBounds*/ false));
    ++NumITargetsGathered;

    auto *StoreOperand = I->getValueOperand();
    if (!StoreOperand->getType()->isPointerTy()) {
      return;
    }

    Destination.push_back(std::make_shared<ITarget>(
        StoreOperand, Location, getPointerAccessSize(StoreOperand),
        /*CheckUpper*/ false, /*CheckLower*/ false, /*ExplicitBounds*/ false));
    ++NumITargetsGathered;

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
