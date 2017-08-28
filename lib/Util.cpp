//===------------------ Util.cpp -- MemSafety Instrumentation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/Util.h"

#include "llvm/IR/Constant.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;
using namespace meminstrument;

#define MEMINSTRUMENT_MD "meminstrument"
#define NOINSTRUMENT_MD "no_instrument"

namespace meminstrument {

void setNoInstrument(GlobalObject *O) {
  auto &Ctx = O->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, NOINSTRUMENT_MD));
  O->setMetadata(MEMINSTRUMENT_MD, N);
}

void setNoInstrument(Constant *C) {
  auto O = cast<GlobalObject>(C);
  setNoInstrument(O);
}

bool hasNoInstrument(GlobalObject *O) {
  auto *N = O->getMetadata(MEMINSTRUMENT_MD);

  if (!N || N->getNumOperands() < 1) {
    return false;
  }

  if (auto *Str = dyn_cast<MDString>(N->getOperand(0))) {
    if (Str->getString().equals(NOINSTRUMENT_MD)) {
      return true;
    }
  }

  return false;
}
}
