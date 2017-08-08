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

MemSafetyAnalysisPass::MemSafetyAnalysisPass() : ModulePass(ID) {}

bool MemSafetyAnalysisPass::doInitialization(llvm::Module &) {
  return false;
}

bool MemSafetyAnalysisPass::runOnModule(Module &) {
  auto *GITPass =
      cast<GatherITargetsPass>(&this->getAnalysis<GatherITargetsPass>());
  this->connectToProvider(GITPass);
  return false;
}

void MemSafetyAnalysisPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<GatherITargetsPass>();
  AU.setPreservesAll();
}

char MemSafetyAnalysisPass::ID = 0;
