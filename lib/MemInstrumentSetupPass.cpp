//===---- MemInstrumentSetupPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/MemInstrumentSetupPass.h"

#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

MemInstrumentSetupPass::MemInstrumentSetupPass() : ModulePass(ID) {}

bool MemInstrumentSetupPass::runOnModule(Module &M) {

  DEBUG(dbgs() << "MemInstrumentSetupPass: processing module `"
               << M.getName().str() << "`\n";);
  return true;
}

void MemInstrumentSetupPass::getAnalysisUsage(AnalysisUsage &AU) const {}

char MemInstrumentSetupPass::ID = 0;