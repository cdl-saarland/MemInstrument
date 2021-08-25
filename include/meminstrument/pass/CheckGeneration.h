//===- meminstrument/CheckGeneration.h - Generate Checks --------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Manage check code generation.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_CHECKGENERATION_H
#define MEMINSTRUMENT_PASS_CHECKGENERATION_H

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"

namespace meminstrument {

void generateInvariants(GlobalConfig &, ITargetVector &, llvm::Function &);

void generateChecks(GlobalConfig &, ITargetVector &, llvm::Function &);
} // namespace meminstrument

#endif
