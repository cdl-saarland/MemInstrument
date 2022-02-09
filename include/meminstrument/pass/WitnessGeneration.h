//===- meminstrument/WitnessGeneration.h - Generate Witnesses ---*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Make sure that bound witnesses are available at the locations where they are
/// needed.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_WITNESSGENERATION_H
#define MEMINSTRUMENT_PASS_WITNESSGENERATION_H

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

void generateWitnesses(GlobalConfig &, ITargetVector &, llvm::Function &);
}

#endif
