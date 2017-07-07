//===- meminstrument/InstrumentationMechanism.h -- TODO doku ---*- C++ -*--===//
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

#include "llvm/IR/Value.h"

namespace meminstrument {

class InstrumentationMechanism {
public:

virtual llvm::Value* insertGetBoundWitness(llvm::IRBuilder& builder, llvm::Value* toCheck) = 0;

virtual void insertCheckBoundWitnessLower(llvm::IRBuilder& builder, llvm::Value* toCheck, llvm::Value* witness) = 0;
virtual void insertCheckBoundWitnessUpper(llvm::IRBuilder& builder, llvm::Value* toCheck, llvm::Value* witness) = 0;
// TODO temporal checks?

virtual void insertCheckBoundWitness(llvm::IRBuilder& builder, llvm::Value* toCheck, llvm::Value* witness) {
  insertCheckBoundWitnessLower(builder, toCheck, witness);
  insertCheckBoundWitnessUpper(builder, toCheck, witness);
}


};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H

