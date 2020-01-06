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

using namespace meminstrument;
using namespace llvm;

namespace {
STATISTIC(NumUnsizedTypes, "modules discarded because of unsized types");
}

InstrumentationPolicy::InstrumentationPolicy(GlobalConfig &cfg) : _CFG(cfg) {}

bool InstrumentationPolicy::validateSize(llvm::Value *Ptr) {
  if (!hasPointerAccessSize(Ptr)) {
    ++NumUnsizedTypes;
    LLVM_DEBUG(dbgs() << "Found pointer to unsized type: `" << *Ptr << "'!\n";);
    _CFG.noteError();
    return false;
  }
  return true;
}
