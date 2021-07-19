//===- NonePolicy.cpp - Don't Generate any Target Policy ------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_policies/NonePolicy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

void NonePolicy::classifyTargets(ITargetVector &, Instruction *) {}
