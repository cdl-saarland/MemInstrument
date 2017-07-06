//===-------- GatherITargetsPass.cpp -- Memory Instrumentation ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/GatherITargetsPass.h"

#include "meminstrument/InstrumentationPolicy.h"

#include "llvm/Support/Debug.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include <iostream>

// Debug
#define DEBUG_TYPE "meminstrument"


using namespace meminstrument;
using namespace llvm;

GatherITargetsPass::GatherITargetsPass()
    : FunctionPass(ID) {
}

bool GatherITargetsPass::runOnFunction(Function &F) {
  const DataLayout& dl = F.getParent()->getDataLayout();

  auto ip = std::unique_ptr<InstrumentationPolicy>(new BeforeOutflowPolicy(dl));

  std::vector<ITarget> dest;

  for (auto& BB : F) {
    DEBUG(
      dbgs() << "GatherITargetsPass: processing block `" << F.getName().str()
      << "::" << BB.getName().str() << "`\n";
    );
    for (auto& I: BB) {
      ip->classifyTarget(dest, &I);
    }
  }
  DEBUG(
    dbgs() << "identified instrumentation targets:" << "\n";
    for (auto& it : dest) {
      dbgs() << "  " << it << "\n";

    }
  );
  return false;
}

void GatherITargetsPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char GatherITargetsPass::ID = 0;

