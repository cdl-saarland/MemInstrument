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
  // virtual llvm::Value *insertGetBoundWitness(llvm::IRBuilder &Builder,
  //                                            llvm::Value *ToCheck) = 0;
  //
  // virtual void insertCheckBoundWitnessLower(llvm::IRBuilder &Builder,
  //                                           llvm::Value *ToCheck,
  //                                           llvm::Value *Witness) = 0;
  // virtual void insertCheckBoundWitnessUpper(llvm::IRBuilder &Builder,
  //                                           llvm::Value *ToCheck,
  //                                           size_t AccessSize,
  //                                           llvm::Value *Witness) = 0;
  // // TODO temporal checks?
  //
  // virtual void insertCheckBoundWitness(llvm::IRBuilder &Builder,
  //                                      llvm::Value *ToCheck,
  //                                      size_t AccessSize,
  //                                      llvm::Value *Witness) {
  //   insertCheckBoundWitnessLower(Builder, ToCheck, Witness);
  //   insertCheckBoundWitnessUpper(Builder, ToCheck, AccessSize, Witness);
  // }
  //
  // virtual llvm::Value *insertGetLowerBound(llvm::IRBuilder &Builder,
  //                                          llvm::Value *Ptr) = 0;
  // virtual llvm::Value *insertGetUpperBound(llvm::IRBuilder &Builder,
  //                                          llvm::Value *Ptr) = 0;
  // virtual void insertGetBounds(llvm::Value *&LowerDest, llvm::Value
  // *&UpperDest,
  //                              llvm::IRBuilder &Builder, llvm::Value *Ptr) =
  //                              0;
  //
  // virtual void handleAlloca(llvm::AllocaInst *AI) = 0;
  //
  // virtual llvm::Type *getWitnessType(void) = 0;
  //
  // virtual ~InstrumentationMechanism(void) {}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
