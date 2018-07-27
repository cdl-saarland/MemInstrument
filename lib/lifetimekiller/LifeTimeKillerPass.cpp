//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/lifetimekiller/LifeTimeKillerPass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

STATISTIC(NumKilledLifeTimeStart, "The # of killed lifetime.start calls");
STATISTIC(NumKilledLifeTimeEnd, "The # of killed lifetime.end calls");

LifeTimeKillerPass::LifeTimeKillerPass() : ModulePass(ID) {}

void LifeTimeKillerPass::releaseMemory(void) {
}

bool LifeTimeKillerPass::runOnModule(Module &M) {

  if (M.getName().endswith("tools/timeit.c")) {
    // small hack to avoid unnecessary work in the lnt tests
    DEBUG(dbgs() << "LifeTimeKillerPass: skip module `" << M.getName().str()
                 << "`\n";);
    return false;
  }

  bool didSomething = false;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto It = BB.begin(); It != BB.end();) {
        Instruction *I = &*It;
        ++It;
        if (auto *Intr = dyn_cast<IntrinsicInst>(I)) {
          switch (Intr->getIntrinsicID()) {
          case Intrinsic::lifetime_start:
            ++NumKilledLifeTimeStart;
          // FALLTHROUGH
          case Intrinsic::lifetime_end:
            ++NumKilledLifeTimeEnd;
            I->eraseFromParent();
            didSomething = true;
          default:;
          }
        }
      }
    }
  }

  return didSomething;
}

void LifeTimeKillerPass::getAnalysisUsage(AnalysisUsage &AU) const {
}

void LifeTimeKillerPass::print(llvm::raw_ostream &O,
                              const llvm::Module *M) const {
  O << "LifeTimeKillerPass for module '" << M->getName() << "'\n";
}

char LifeTimeKillerPass::ID = 0;

