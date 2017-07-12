//===----- GenerateWitnessesPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/GenerateWitnessesPass.h"

#include "meminstrument/MemInstrumentSetupPass.h"
#include "meminstrument/MemSafetyAnalysisPass.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

GenerateWitnessesPass::GenerateWitnessesPass() : FunctionPass(ID) {}

bool GenerateWitnessesPass::runOnFunction(Function &F) {

  if (F.empty())
    return false;

  auto &MISPass = getAnalysis<MemSafetyAnalysisPass>();

  DEBUG(dbgs() << "GenerateWitnessesPass: processing function `"
               << F.getName().str() << "`\n";);
  auto &MAPass = getAnalysis<MemSafetyAnalysisPass>(F);
  return true;
}

void GenerateWitnessesPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<MemSafetyAnalysisPass>();
  AU.addRequiredTransitive<MemInstrumentSetupPass>();
}

char GenerateWitnessesPass::ID = 0;
