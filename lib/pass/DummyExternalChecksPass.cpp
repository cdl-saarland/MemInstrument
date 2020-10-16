//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/DummyExternalChecksPass.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/Witness.h"

#include "llvm/IR/Argument.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

DummyExternalChecksPass::DummyExternalChecksPass() : ModulePass(ID) {}

bool DummyExternalChecksPass::runOnModule(Module &) {

  LLVM_DEBUG(dbgs() << "Running External Checks Pass\n";);

  return false;
}

void DummyExternalChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char DummyExternalChecksPass::ID = 0;

bool DummyExternalChecksPass::prepareModule(MemInstrumentPass &, Module &) {
  // No need to do something here.
  return false;
}

/// Implementation that only changes existing ITargets
/// (required for use with external_only config)
void DummyExternalChecksPass::updateITargetsForFunction(MemInstrumentPass &P,
                                                        ITargetVector &Vec,
                                                        Function &F) {
  // we store our relevant targets in a worklist for later materialization
  auto &CurrentWL = WorkList[&F];

  // if an instruction is annotated as "checkearly", check function argument
  // operands at the beginning of the function instead of here.
  auto *EntryLoc = F.getEntryBlock().getFirstNonPHI();

  for (auto &IT : Vec) {
    if (!IT->isValid() || !IT->is(ITarget::Kind::ConstSizeCheck)) {
      continue;
    }
    auto *L = IT->getLocation();
    if (L->getMetadata("checkearly")) {
      if (auto *A = dyn_cast<Argument>(IT->getInstrumentee())) {
        // create a new ITarget for the beginning of the function
        auto res = ITarget::createBoundsTarget(A, EntryLoc);
        // remember it for later
        CurrentWL.push_back(res);
        // invalidate the initial ITarget so that no checks are generated for it
        IT->invalidate();
      }
    }
  }
  size_t Num = meminstrument::getNumValidITargets(Vec);
  LLVM_DEBUG(dbgs() << "number of remaining valid targets: " << Num << "\n";);
  // add new targets to the ITarget vector
  Vec.insert(Vec.end(), CurrentWL.begin(), CurrentWL.end());
}

void DummyExternalChecksPass::materializeExternalChecksForFunction(
    MemInstrumentPass &P, ITargetVector &Vec, Function &F) {
  auto &CFG = P.getConfig();
  auto &IM = CFG.getInstrumentationMechanism();
  auto &Ctx = F.getContext();

  auto &CurrentWL = WorkList[&F];

  auto *I64Ty = Type::getInt64Ty(Ctx);

  for (auto &IT : CurrentWL) {
    IRBuilder<> Builder(IT->getLocation());

    auto *Ptr2Int = Builder.CreatePtrToInt(IT->getInstrumentee(), I64Ty);

    auto *LowerPtr = IT->getBoundWitness()->getLowerBound();
    auto *Lower = Builder.CreatePtrToInt(LowerPtr, I64Ty);
    auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

    auto acc_size = getPointerAccessSize(F.getParent()->getDataLayout(),
                                         IT->getInstrumentee());
    auto *Sum = Builder.CreateAdd(Ptr2Int, ConstantInt::get(I64Ty, acc_size));
    auto *UpperPtr = IT->getBoundWitness()->getUpperBound();
    auto *Upper = Builder.CreatePtrToInt(UpperPtr, I64Ty);
    auto *CmpUpper = Builder.CreateICmpUGT(Sum, Upper);

    auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

    auto Unreach = SplitBlockAndInsertIfThen(Or, IT->getLocation(), true);
    Builder.SetInsertPoint(Unreach);
    Builder.CreateCall(IM.getFailFunction());
    IT->invalidate();
  }
}
