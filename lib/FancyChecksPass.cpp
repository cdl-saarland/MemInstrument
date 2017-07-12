//===--------- FancyChecksPass.cpp -- MemSafety Instrumentation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/FancyChecksPass.h"

#include "meminstrument/GenerateWitnessesPass.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

FancyChecksPass::FancyChecksPass() : FunctionPass(ID) {}

bool FancyChecksPass::runOnFunction(Function &F) {

  if (F.empty())
    return false;

  DEBUG(dbgs() << "FancyChecksPass: processing function `" << F.getName().str()
               << "`\n";);
  auto &GWPass = getAnalysis<GenerateWitnessesPass>(F);
  return true;
}

void FancyChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenerateWitnessesPass>();
}

char FancyChecksPass::ID = 0;
