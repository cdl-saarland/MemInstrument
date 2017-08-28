//===---- InstrumentationMechanism.cpp -- MemSafety Instrumentation -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/InstrumentationMechanism.h"

#include "meminstrument/DummyMechanism.h"
#include "meminstrument/SplayMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/Util.h"

using namespace llvm;
using namespace meminstrument;

namespace {

enum InstrumentationMechanismKind {
  IM_dummy,
  IM_splay,
};

cl::opt<InstrumentationMechanismKind> InstrumentationMechanismOpt(
    "memsafety-imechanism",
    cl::desc("Choose InstructionMechanism: (default: dummy)"),
    cl::values(clEnumValN(IM_dummy, "dummy",
                          "only insert dummy calls for instrumentation")),
    cl::values(clEnumValN(IM_splay, "splay",
                          "use splay tree for instrumentation")),
    cl::init(IM_dummy) // default
    );

std::unique_ptr<InstrumentationMechanism> GlobalIM(nullptr);
}

InstrumentationMechanism &InstrumentationMechanism::get(void) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationMechanismOpt) {
    case IM_dummy:
      GlobalIM.reset(new DummyMechanism());
      break;

    case IM_splay:
      GlobalIM.reset(new SplayMechanism());
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}
