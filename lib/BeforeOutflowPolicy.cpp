//===------- BeforeOutflowPolicy.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/BeforeOutflowPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/Util.h"

STATISTIC(NumITargetsGathered,
          "The # of instrumentation targets initially gathered");

STATISTIC(NumIntrinsicsHandled, "The # of intrinsics that could be handled");
STATISTIC(NumIntrinsicsNotHandled,
          "The # of intrinsics that could not be handled");

using namespace meminstrument;
using namespace llvm;

size_t BeforeOutflowPolicy::getPointerAccessSize(llvm::Value *V) {
  auto *Ty = V->getType();
  assert(Ty->isPointerTy() && "Only pointer types allowed!");

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return 0;
  }

  if (!PointeeType->isSized()) {
    errs() << "Found pointer to unsized type `" << *PointeeType << "'!\n";
    llvm_unreachable("Only pointers to sized types allowed!");
  }

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}

bool handleInstrinsicInst(std::vector<std::shared_ptr<ITarget>> &Dest,
                          llvm::IntrinsicInst *II) {
  switch (II->getIntrinsicID()) {
  case Intrinsic::memcpy:
  case Intrinsic::memmove: {
    auto *MT = cast<MemTransferInst>(II);
    auto *Len = MT->getLength();
    auto *Src = MT->getSource();
    Dest.push_back(std::make_shared<ITarget>(
        Src, II, Len,
        /*CheckUpper*/ true, /*CheckLower*/ true, /*ExplicitBounds*/ false));

    auto *Dst = MT->getDest();
    Dest.push_back(std::make_shared<ITarget>(
        Dst, II, Len,
        /*CheckUpper*/ true, /*CheckLower*/ true, /*ExplicitBounds*/ false));
    return true;
  }
  case Intrinsic::memset: {
    auto *MS = cast<MemSetInst>(II);
    auto *Len = MS->getLength();
    auto *Dst = MS->getDest();
    Dest.push_back(std::make_shared<ITarget>(
        Dst, II, Len,
        /*CheckUpper*/ true, /*CheckLower*/ true, /*ExplicitBounds*/ false));
    return true;
  }
  default:
    return false;
  }
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

    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (handleInstrinsicInst(Destination, II)) {
        ++NumIntrinsicsHandled;
        break;
      }
      ++NumIntrinsicsNotHandled;
    }

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
