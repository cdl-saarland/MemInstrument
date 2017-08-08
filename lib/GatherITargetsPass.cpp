//===-------- GatherITargetsPass.cpp -- MemSafety Instrumentation ---------===//
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

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

GatherITargetsPass::GatherITargetsPass() : ModulePass(ID) {}

bool GatherITargetsPass::doInitialization(llvm::Module &) {
  return false;
}

bool GatherITargetsPass::runOnModule(Module &M) {
  this->initializeEmpty();

  const DataLayout &DL = M.getDataLayout();

  // TODO make configurable
  auto Policy =
      std::unique_ptr<InstrumentationPolicy>(new BeforeOutflowPolicy(DL));

  for (auto &F : M) {
    std::vector<std::shared_ptr<ITarget>> &Destination =
        this->getITargetsForFunction(&F);
    for (auto &BB : F) {
      DEBUG(dbgs() << "GatherITargetsPass: processing block `"
                   << F.getName().str() << "::" << BB.getName().str()
                   << "`\n";);
      for (auto &I : BB) {
        Policy->classifyTargets(Destination, &I);
      }
    }
    DEBUG(dbgs() << "identified instrumentation targets:"
                 << "\n";
          for (auto &Target
               : Destination) {
            dbgs() << "  " << *Target << "\n";

          });
  }
  return false;
}

void GatherITargetsPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char GatherITargetsPass::ID = 0;
