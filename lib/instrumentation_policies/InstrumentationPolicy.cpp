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

enum InstrumentationPolicyKind {
  IP_beforeOutflow,
};

cl::opt<InstrumentationPolicyKind> InstrumentationPolicyOpt(
    "mi-ipolicy",
    cl::desc("Choose InstructionPolicy: (default: before-outflow)"),
    cl::values(clEnumValN(IP_beforeOutflow, "before-outflow",
                          "only insert dummy calls for instrumentation")),
    cl::init(IP_beforeOutflow) // default
);

std::unique_ptr<InstrumentationPolicy> GlobalIM(nullptr);
} // namespace

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
