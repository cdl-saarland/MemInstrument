//=== meminstrument/InstrumentationMechanism.h - MemSafety Instr -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) = 0;

  virtual void insertCheck(ITarget &Target) = 0;

  virtual void materializeBounds(ITarget &Target) = 0;

  virtual ~InstrumentationMechanism(void) {}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
