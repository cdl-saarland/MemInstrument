//===------------ ITarget.cpp -- Memory Instrumentation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITarget.h"

using namespace llvm;
using namespace meminstrument;

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub,
    bool lb, bool temp) : instrumentee(i), location(loc), accessSize(sz),
    checkUpperBound(ub), checkLowerBound(lb), checkTemporal(temp) {
}

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub,
    bool lb) : ITarget(i, loc, sz, ub, lb, false) {
}

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz)
  : ITarget(i, loc, sz, true, true) {
}

