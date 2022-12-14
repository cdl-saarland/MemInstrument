//===- ExampleExternalChecksPass.cpp - Example Checks ---------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/ExampleExternalChecksPass.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/Witness.h"

#include "llvm/IR/Argument.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

//===--------------------------- ModulePass -------------------------------===//

char ExampleExternalChecksPass::ID = 0;

ExampleExternalChecksPass::ExampleExternalChecksPass() : ModulePass(ID) {}

bool ExampleExternalChecksPass::runOnModule(Module &) {

  LLVM_DEBUG(dbgs() << "Running External Checks Pass\n";);
  return false;
}

void ExampleExternalChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool ExampleExternalChecksPass::doFinalization(Module &) {
  WorkList.clear();
  return false;
}

void ExampleExternalChecksPass::print(raw_ostream &OS, const Module *M) const {
  OS << "Running Example External Checks Pass on\n" << *M << "\n";
}

//===--------------------- OptimizationInterface --------------------------===//

void ExampleExternalChecksPass::updateITargetsForFunction(MemInstrumentPass &P,
                                                          ITargetVector &Vec,
                                                          Function &F) {
  // we store our relevant targets in a worklist for later materialization
  auto &CurrentWL = WorkList[&F];

  // if an instruction is annotated as "checkearly", check function argument
  // operands at the beginning of the function instead of here.
  auto *EntryLoc = &(*F.getEntryBlock().getFirstInsertionPt());

  for (auto &IT : Vec) {
    if (!IT->isValid() || !isa<ConstSizeCheckIT>(IT)) {
      continue;
    }
    auto *L = IT->getLocation();
    if (L->getMetadata("checkearly")) {
      if (auto *A = dyn_cast<Argument>(IT->getInstrumentee())) {
        // create a new ITarget for the beginning of the function
        auto res = ITargetBuilder::createBoundsTarget(A, EntryLoc);
        // remember it for later
        CurrentWL.push_back(res);
        // invalidate the initial ITarget so that no checks are generated for it
        IT->invalidate();
      }
    }
  }
  LLVM_DEBUG(dbgs() << "number of remaining valid targets: "
                    << ITargetBuilder::getNumValidITargets(Vec) << "\n";);
  // add new targets to the ITarget vector
  Vec.insert(Vec.end(), CurrentWL.begin(), CurrentWL.end());
}

void ExampleExternalChecksPass::materializeExternalChecksForFunction(
    MemInstrumentPass &P, ITargetVector &, Function &F) {
  auto &CFG = P.getConfig();
  auto &IM = CFG.getInstrumentationMechanism();
  auto &Ctx = F.getContext();

  auto &CurrentWL = WorkList[&F];

  auto *I64Ty = Type::getInt64Ty(Ctx);

  for (auto &IT : CurrentWL) {
    IRBuilder<> Builder(IT->getLocation());

    auto *Ptr2Int = Builder.CreatePtrToInt(IT->getInstrumentee(), I64Ty);

    auto *LowerPtr = IT->getSingleBoundWitness()->getLowerBound();
    auto *Lower = Builder.CreatePtrToInt(LowerPtr, I64Ty);
    auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

    auto acc_size = getPointerAccessSize(F.getParent()->getDataLayout(),
                                         IT->getInstrumentee());
    auto *Sum = Builder.CreateAdd(Ptr2Int, ConstantInt::get(I64Ty, acc_size));
    auto *UpperPtr = IT->getSingleBoundWitness()->getUpperBound();
    auto *Upper = Builder.CreatePtrToInt(UpperPtr, I64Ty);
    auto *CmpUpper = Builder.CreateICmpUGT(Sum, Upper);

    auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

    auto Unreach = SplitBlockAndInsertIfThen(Or, IT->getLocation(), true);
    Builder.SetInsertPoint(Unreach);
    Builder.CreateCall(IM.getFailFunction());
    IT->invalidate();
  }
}
