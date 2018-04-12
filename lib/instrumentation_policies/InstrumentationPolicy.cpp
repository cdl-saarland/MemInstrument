//==- instrumentation_policies/InstrumentationPolicy.cpp - MemSafety Instr. -=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"

#include "meminstrument/instrumentation_policies/AccessOnlyPolicy.h"
#include "meminstrument/instrumentation_policies/BeforeOutflowPolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

#include "meminstrument/pass/Util.h"

STATISTIC(NumUnsizedTypes, "modules discarded because of unsized types");

using namespace meminstrument;
using namespace llvm;

size_t InstrumentationPolicy::getPointerAccessSize(const llvm::DataLayout &DL,
                                                   llvm::Value *V) {
  auto *Ty = V->getType();
  assert(Ty->isPointerTy() && "Only pointer types allowed!");

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return 0;
  }

  if (!PointeeType->isSized()) {
    ++NumUnsizedTypes;
    DEBUG(dbgs() << "Found pointer to unsized type `" << *PointeeType << "'!\n";);
    _CFG.noteError();
    return 1;
  }

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}
