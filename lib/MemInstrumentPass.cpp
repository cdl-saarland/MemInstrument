//===---------- MemInstrument.cpp -- Memory Instrumentation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

// Debug
#define DEBUG_TYPE "meminstrument"
#include "meminstrument/MemInstrumentPass.h"
#include "meminstrument/GatherITargetsPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include <iostream>

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

MemInstrumentPass::MemInstrumentPass()
    : ModulePass(ID) {
}

bool MemInstrumentPass::runOnModule(Module &M) {

  for (auto& F : M) {
    if (F.empty())
      continue;

    DEBUG(
      dbgs() << "MemInstrumentPass: processing function `" << F.getName().str()
        << "`\n";
    );
    auto& IT = getAnalysis<GatherITargetsPass>(F);
  }
  return true;
}

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GatherITargetsPass>();
}

char MemInstrumentPass::ID = 0;

