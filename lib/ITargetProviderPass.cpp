//===------- ITargetProviderPass.cpp -- MemSafety Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITargetProviderPass.h"

#include "meminstrument/InstrumentationPolicy.h"
#include "meminstrument/MemInstrumentSetupPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"

#include "meminstrument/Util.h"

using namespace meminstrument;
using namespace llvm;

ITargetProviderPass::ITargetProviderPass() : ModulePass(ID) {}

bool ITargetProviderPass::doInitialization(llvm::Module &) { return false; }

bool ITargetProviderPass::runOnModule(Module &M) {
  static Module *Prev = nullptr;
  assert((Prev != &M) && "ITargetProviderPass ran twice!");
  Prev = &M;

  const DataLayout &DL = M.getDataLayout();

  auto &IP = InstrumentationPolicy::get(DL);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F)) {
      continue;
    }
    DEBUG(dbgs() << "ITargetProviderPass: processing function `"
                 << F.getName().str() << "`\n";);
    ITargetVector &Destination = this->getITargetsForFunction(&F);
    for (auto &BB : F) {
      DEBUG(dbgs() << "ITargetProviderPass: processing block `"
                   << F.getName().str() << "::" << BB.getName().str()
                   << "`\n";);
      for (auto &I : BB) {
        if (hasNoInstrument(&I)) {
          continue;
        }
        IP.classifyTargets(Destination, &I);
      }
    }
    DEBUG_ALSO_WITH_TYPE(
        "meminstrument-itargetprovider",
        dbgs() << "identified instrumentation targets:"
               << "\n";
        for (auto &Target
             : Destination) { dbgs() << "  " << *Target << "\n"; });
  }
  return false;
}

ITargetVector &ITargetProviderPass::getITargetsForFunction(llvm::Function *F) {
  TargetMap.lookup(F);
  return TargetMap[F];
}

void ITargetProviderPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MemInstrumentSetupPass>();
  AU.setPreservesAll();
}

char ITargetProviderPass::ID = 0;
