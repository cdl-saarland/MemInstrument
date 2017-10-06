//=== meminstrument/pass/WitnessGeneration.h - MemSafety Instr. -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_PASS_WITNESSGENERATION_H
#define MEMINSTRUMENT_PASS_WITNESSGENERATION_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

void generateWitnesses(ITargetVector &Vec, llvm::Function &F);

}

#endif
