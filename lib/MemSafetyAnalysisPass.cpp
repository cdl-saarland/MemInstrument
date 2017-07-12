//===----- MemSafetyAnalysisPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/MemSafetyAnalysisPass.h"

#include "meminstrument/GatherITargetsPass.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

MemSafetyAnalysisPass::MemSafetyAnalysisPass() : FunctionPass(ID) {}

bool MemSafetyAnalysisPass::runOnFunction(Function &F) { return false; }

void MemSafetyAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequiredTransitive<GatherITargetsPass>();
}

char MemSafetyAnalysisPass::ID = 0;
