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

size_t BeforeOutflowPolicy::getPointerAccessSize(llvm::Value *v) {
  auto *type = v->getType();
  assert(type->isPointerTy() && "Only pointer types allowed!");

  auto *pointee_type = type->getPointerElementType();

  size_t sz = this->datalayout.getTypeStoreSize(pointee_type);

  return sz;
}

void BeforeOutflowPolicy::classifyTargets(std::vector<ITarget> &dest,
                                         llvm::Instruction *loc) {
  switch (loc->getOpcode()) {
  case Instruction::Ret: {
    llvm::ReturnInst *inst = llvm::cast<llvm::ReturnInst>(loc);

    auto *operand = inst->getReturnValue();
    if (!operand || !operand->getType()->isPointerTy()) {
      return;
    }

    dest.emplace_back(operand, loc, getPointerAccessSize(operand));
    break;
  }
  case Instruction::Call: {
    llvm::CallInst *inst = llvm::cast<llvm::CallInst>(loc);

    if (!inst->getCalledFunction()) { // call via function pointer
      // TODO implement
      DEBUG(dbgs() << "skipping indirect function call " << loc->getName()
                   << "\n";);
    }

    for (unsigned i = 0; i < inst->getNumArgOperands(); ++i) {
      auto *operand = inst->getArgOperand(i);
      if (!operand->getType()->isPointerTy()) {
        continue;
      }

      dest.emplace_back(operand, loc, getPointerAccessSize(operand));
    }

    break;
  }
  case Instruction::Load: {
    llvm::LoadInst *inst = llvm::cast<llvm::LoadInst>(loc);
    auto *ptr_operand = inst->getPointerOperand();
    dest.emplace_back(ptr_operand, loc, getPointerAccessSize(ptr_operand));
    break;
  }
  case Instruction::Store: {
    llvm::StoreInst *inst = llvm::cast<llvm::StoreInst>(loc);
    auto *ptr_operand = inst->getPointerOperand();
    dest.emplace_back(ptr_operand, loc, getPointerAccessSize(ptr_operand));

    auto *store_operand = inst->getValueOperand();
    if (!store_operand->getType()->isPointerTy()) {
      return;
    }

    dest.emplace_back(store_operand, loc, getPointerAccessSize(store_operand));

    break;
  }

  case Instruction::Invoke:
  case Instruction::Resume:
  case Instruction::IndirectBr:
  case Instruction::AtomicCmpXchg:
  case Instruction::AtomicRMW:
    // TODO implement
    DEBUG(dbgs() << "skipping unimplemented instruction " << loc->getName()
                 << "\n";);
    break;

  default:
    break;
  }
}

Value *BeforeOutflowPolicy::createWitness(Value *instrumentee) {

  if (auto *inst = dyn_cast<Instruction>(instrumentee)) {
    IRBuilder<> builder(inst->getNextNode());
    // insert code after the source instruction
    switch (inst->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::Load:
      // the assumption is that the validity of these has already been checked
      return insertGetBoundWitness(builder, inst);

    case Instruction::PHI: {
      auto *ptr_phi = cast<PHINode>(inst);
      unsigned num_ops = ptr_phi->getNumIncomingValues();
      auto *new_type = getWitnessType();
      auto *new_phi = builder.CreatePHI(new_type, num_ops); // TODO give name
      for (unsigned i = 0; i < num_ops; ++i) {
        auto *inval = ptr_phi->getIncomingValue(i);
        auto *inbb = ptr_phi->getIncomingBlock(i);
        auto *new_inval = createWitness(inval);
        new_phi->addIncoming(new_inval, inbb);
      }
      return new_phi;
    }

    case Instruction::Select: {
      auto *ptr_select = cast<SelectInst>(inst);
      auto *cond = ptr_select->getCondition();

      auto *true_val = createWitness(ptr_select->getTrueValue());
      auto *false_val = createWitness(ptr_select->getFalseValue());

      builder.CreateSelect(cond, true_val, false_val); // TODO give name
    }

    case Instruction::GetElementPtr: {
      // pointer arithmetic doesn't change the pointer bounds
      auto *op = cast<GetElementPtrInst>(inst);
      return createWitness(op->getPointerOperand());
    }

    case Instruction::IntToPtr: {
      // FIXME is this what we want?
      return insertGetBoundWitness(builder, inst);
    }

    case Instruction::BitCast: {
      // bit casts don't change the pointer bounds
      auto *op = cast<BitCastInst>(inst);
      return createWitness(op->getOperand(0));
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(instrumentee)) {
    // should have already been checked at the call site
    return insertGetBoundWitness(builder, instrumentee);
  } else if (isa<GlobalValue>(instrumentee)) {
    llvm_unreachable("Global Values are not yet supported!"); // FIXME
    // return insertGetBoundWitness(builder, instrumentee);
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }
}

void BeforeOutflowPolicy::instrumentFunction(llvm::Function &func,
                                             std::vector<ITarget> &targets) {
  for (auto &bb : func) {
    for (auto &inst : bb) {
      if (auto *alloca_inst = dyn_cast<AllocaInst>(&inst)) {
        handleAlloca(alloca_inst);
      }
    }
  }
  // TODO when to change allocas?

  for (const auto &it : targets) {
    if (it.checkTemporal)
      llvm_unreachable("Temporal checks are not supported by this policy!");

    if (!(it.checkLowerBound || it.checkUpperBound))
      continue; // nothing to do!

    auto *witness = createWitness(it.instrumentee);
    IRBuilder<> builder(it.location);

    if (it.checkLowerBound && it.checkUpperBound) {
      insertCheckBoundWitness(builder, it.instrumentee, witness);
    } else if (it.checkUpperBound) {
      insertCheckBoundWitnessUpper(builder, it.instrumentee, witness);
    } else {
      insertCheckBoundWitnessLower(builder, it.instrumentee, witness);
    }
  }
}
