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

#include "llvm/Support/Debug.h"
#include "llvm/IR/Instructions.h"

#define DEBUG_TYPE "meminstrument"


using namespace meminstrument;
using namespace llvm;

size_t BeforeOutflowPolicy::getPointerAccessSize(llvm::Value* v) {
      auto* type = v->getType();
      assert(type->isPointerTy() && "Only pointer types allowed!");

      auto* pointee_type = type->getPointerElementType();

      size_t sz = this->datalayout.getTypeStoreSize(pointee_type);

      return sz;
}

void BeforeOutflowPolicy::classifyTarget(std::vector<ITarget>& dest,
    llvm::Instruction* loc) {
  switch (loc->getOpcode()) {
    case Instruction::Ret: {
      llvm::ReturnInst* inst = llvm::cast<llvm::ReturnInst>(loc);

      auto* operand = inst->getReturnValue();
      if (!operand || !operand->getType()->isPointerTy()) {
        return;
      }

      dest.emplace_back(operand, loc, getPointerAccessSize(operand));
    break; }
    case Instruction::Call: {
      llvm::CallInst* inst = llvm::cast<llvm::CallInst>(loc);

      if (!inst->getCalledFunction()) { // call via function pointer
        // TODO implement
        DEBUG(
          dbgs() << "skipping indirect function call " << loc->getName() << "\n";
        );
      }

      for (unsigned i = 0; i < inst->getNumArgOperands(); ++i) {
        auto* operand = inst->getArgOperand(i);
        if (!operand->getType()->isPointerTy()) {
          continue;
        }

        dest.emplace_back(operand, loc, getPointerAccessSize(operand));
      }

    break; }
    case Instruction::Load: {
      llvm::LoadInst* inst = llvm::cast<llvm::LoadInst>(loc);
      auto* ptr_operand = inst->getPointerOperand();
      dest.emplace_back(ptr_operand, loc, getPointerAccessSize(ptr_operand));
    break; }
    case Instruction::Store: {
      llvm::StoreInst* inst = llvm::cast<llvm::StoreInst>(loc);
      auto* ptr_operand = inst->getPointerOperand();
      dest.emplace_back(ptr_operand, loc, getPointerAccessSize(ptr_operand));

      auto* store_operand = inst->getValueOperand();
      if (!store_operand->getType()->isPointerTy()) {
        return;
      }

      dest.emplace_back(store_operand, loc, getPointerAccessSize(store_operand));

    break; }

    case Instruction::Invoke:
    case Instruction::Resume:
    case Instruction::IndirectBr:
    case Instruction::AtomicCmpXchg:
    case Instruction::AtomicRMW:
    // TODO implement
    DEBUG(
      dbgs() << "skipping unimplemented instruction " << loc->getName() << "\n";
    );
    break;

    default:
    break;
  }

}
