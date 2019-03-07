//===------------- pass/Util.cpp -- MemSafety Instrumentation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Metadata.h"

#include "meminstrument/Config.h"

using namespace llvm;
using namespace meminstrument;

#define MEMINSTRUMENT_MD "meminstrument"
#define NOINSTRUMENT_MD "no_instrument"
#define BYVAL_HANDLING_MD "byval_handling"

namespace {
bool hasMDStrImpl(const char* ref, MDNode *N) {
  if (!N || N->getNumOperands() < 1) {
    return false;
  }

  if (auto *Str = dyn_cast<MDString>(N->getOperand(0))) {
    if (Str->getString().equals(ref)) {
      return true;
    }
  }

  return false;
}
} // namespace

namespace meminstrument {

void setNoInstrument(GlobalObject *O) {
  auto &Ctx = O->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, NOINSTRUMENT_MD));
  O->setMetadata(MEMINSTRUMENT_MD, N);
}

void setNoInstrument(Instruction *I) {
  auto &Ctx = I->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, NOINSTRUMENT_MD));
  I->setMetadata(MEMINSTRUMENT_MD, N);
}

void setByvalHandling(Instruction *I) {
  auto &Ctx = I->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, BYVAL_HANDLING_MD));
  I->setMetadata(BYVAL_HANDLING_MD, N);
}

void setNoInstrument(Constant *C) {
  auto O = cast<GlobalObject>(C);
  setNoInstrument(O);
}

bool hasNoInstrument(GlobalObject *O) {
  return hasMDStrImpl(NOINSTRUMENT_MD, O->getMetadata(MEMINSTRUMENT_MD));
}

bool hasNoInstrument(Instruction *O) {
  return hasMDStrImpl(NOINSTRUMENT_MD, O->getMetadata(MEMINSTRUMENT_MD));
}

bool hasByvalHandling(Instruction *O) {
  return hasMDStrImpl(BYVAL_HANDLING_MD, O->getMetadata(BYVAL_HANDLING_MD));
}

bool hasPointerAccessSize(llvm::Value *V) {
  auto *Ty = V->getType();
  if (!Ty->isPointerTy()) {
    return false;
  }

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return true;
  }

  if (!PointeeType->isSized()) {
    return false;
  }

  return true;
}

size_t getPointerAccessSize(const llvm::DataLayout &DL, llvm::Value *V) {
  assert(hasPointerAccessSize(V));

  auto *Ty = V->getType();

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return 0;
  }

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}
} // namespace meminstrument
