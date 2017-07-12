//===------- GenerateChecksPass.cpp -- MemSafety Instrumentation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/GenerateChecksPass.h"

#include "meminstrument/FancyChecksPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

GenerateChecksPass::GenerateChecksPass() : FunctionPass(ID) {}

bool GenerateChecksPass::runOnFunction(Function &F) {

  if (F.empty())
    return false;

  DEBUG(dbgs() << "GenerateChecksPass: processing function `"
               << F.getName().str() << "`\n";);
  auto &FCPass = getAnalysis<FancyChecksPass>(F);
  return true;
}

void GenerateChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<FancyChecksPass>();
}

char GenerateChecksPass::ID = 0;
