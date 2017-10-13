//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/DummyExternalChecksPass.h"

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/Witness.h"

#include "llvm/IR/Argument.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

DummyExternalChecksPass::DummyExternalChecksPass() : ModulePass(ID) {}

bool DummyExternalChecksPass::runOnModule(Module &M) {

  DEBUG(dbgs() << "Running External Checks Pass\n";);

  return false;
}

void DummyExternalChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char DummyExternalChecksPass::ID = 0;

void DummyExternalChecksPass::updateITargetsForFunction(ITargetVector &Vec, llvm::Function &F) {
  // we store our relevant targets in a worklist for later materialization
  auto& CurrentWL = WorkList[&F];

  // if an instruction is annotated as "checkearly", check function argument
  // operands at the beginning of the function instead of here.
  auto* EntryLoc = F.getEntryBlock().getFirstNonPHI();

  for (auto &IT : Vec) {
    if (!(IT->isValid() && IT->CheckUpperBoundFlag && IT->CheckLowerBoundFlag )) {
      continue;
    }
    auto *L = IT->Location;
    if (L->getMetadata("checkearly")) {
      auto *A = dyn_cast<Argument>(IT->Instrumentee);
      if (A && IT->AccessSizeVal == nullptr) {
        // create a new ITarget for the beginning of the function
        auto res = std::make_shared<ITarget>(A, EntryLoc, IT->AccessSize, /* RequiresExplicitBounds */true);
        // make sure it checks for the same criteria
        res->joinFlags(*IT);
        // also remember it for later
        CurrentWL.push_back(res);
        // invalidate the initial ITarget so that no checks are generated for it
        IT->invalidate();
      }
    }
  }
  // add new targets to the ITarget vector
  Vec.insert(Vec.end(), CurrentWL.begin(), CurrentWL.end());
}

void DummyExternalChecksPass::materializeExternalChecksForFunction(ITargetVector &Vec, llvm::Function &F) {
  auto &IM = InstrumentationMechanism::get();
  auto& Ctx = F.getContext();

  auto& CurrentWL = WorkList[&F];

  auto* I64Ty = Type::getInt64Ty(Ctx);

  for (auto& IT : CurrentWL) {
    IRBuilder<> Builder(IT->Location);

    auto *Ptr2Int = Builder.CreatePtrToInt(IT->Instrumentee, I64Ty);

    auto *Lower = IT->BoundWitness->getLowerBound();
    auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

    auto *Sum = Builder.CreateAdd(Ptr2Int, ConstantInt::get(I64Ty, IT->AccessSize));
    auto *Upper = IT->BoundWitness->getUpperBound();
    auto *CmpUpper = Builder.CreateICmpUGT(Sum, Upper);

    auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

    auto Unreach = SplitBlockAndInsertIfThen(Or, IT->Location, true);
    Builder.SetInsertPoint(Unreach);
    Builder.CreateCall(IM.getFailFunction());
    IT->invalidate();
  }
}
