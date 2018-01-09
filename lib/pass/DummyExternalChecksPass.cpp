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

bool DummyExternalChecksPass::runOnModule(Module &M) {

  DEBUG(dbgs() << "Running External Checks Pass\n";);

  return false;
}

void DummyExternalChecksPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char DummyExternalChecksPass::ID = 0;

/// Implementation that creates new ITargets for all relevant Values
void DummyExternalChecksPass::updateITargetsForFunction(ITargetVector &Vec,
                                                        llvm::Function &F) {

  // Get the DataLayout so that we can compute pointer access sizes
  const DataLayout &DL = F.getParent()->getDataLayout();

  // if a load or store instruction is annotated as "checkearly", check
  // function argument operands at the beginning of the function instead of
  // here.
  auto *EntryLoc = F.getEntryBlock().getFirstNonPHI();

  // we store our relevant targets in a worklist for later materialization
  auto &CurrentWL = WorkList[&F];

  for (auto &BB : F) {
    for (auto &I: BB) {
      if (I.getMetadata("checkearly")) {
        Argument *A = nullptr;
        if (auto* MI = dyn_cast<StoreInst>(&I)) {
          A = dyn_cast<Argument>(MI->getPointerOperand());
        } else if (auto* MI = dyn_cast<LoadInst>(&I)) {
          A = dyn_cast<Argument>(MI->getPointerOperand());
        }

        if (A) {
          // create a new ITarget for the beginning of the function
          auto res = std::make_shared<ITarget>(A, EntryLoc,
                                               getPointerAccessSize(DL, A),
                                               /* CheckUpperBoundFlag */ true,
                                               /* CheckLowerBoundFlag */ true,
                                               /* RequiresExplicitBounds */ true);
          // store it in our worklist so that we can add the checks later
          CurrentWL.push_back(res);
          // and also store it in the ITarget list so that bounds are generated
          Vec.push_back(res);
        }
      }
    }
  }
}

/// Alternative implementation that only changes existing ITargets
/// (required for use with external_only config)
// void DummyExternalChecksPass::updateITargetsForFunction(ITargetVector &Vec,
//                                                         llvm::Function &F) {
//   // we store our relevant targets in a worklist for later materialization
//   auto &CurrentWL = WorkList[&F];
//
//   // if an instruction is annotated as "checkearly", check function argument
//   // operands at the beginning of the function instead of here.
//   auto *EntryLoc = F.getEntryBlock().getFirstNonPHI();
//
//   for (auto &IT : Vec) {
//     if (!(IT->isValid() && IT->CheckUpperBoundFlag &&
//           IT->CheckLowerBoundFlag)) {
//       continue;
//     }
//     auto *L = IT->Location;
//     if (L->getMetadata("checkearly")) {
//       auto *A = dyn_cast<Argument>(IT->Instrumentee);
//       if (A && IT->AccessSizeVal == nullptr) {
//         // create a new ITarget for the beginning of the function
//         auto res = std::make_shared<ITarget>(A, EntryLoc, IT->AccessSize,
//                                              #<{(| RequiresExplicitBounds |)}># true);
//         // make sure it checks for the same criteria
//         res->joinFlags(*IT);
//         // also remember it for later
//         CurrentWL.push_back(res);
//         // invalidate the initial ITarget so that no checks are generated for it
//         IT->invalidate();
//       }
//     }
//   }
//   // add new targets to the ITarget vector
//   Vec.insert(Vec.end(), CurrentWL.begin(), CurrentWL.end());
// }

void DummyExternalChecksPass::materializeExternalChecksForFunction(
    ITargetVector &Vec, llvm::Function &F) {
  auto &IM = GlobalConfig::get(*F.getParent()).getInstrumentationMechanism();
  auto &Ctx = F.getContext();

  auto &CurrentWL = WorkList[&F];

  auto *I64Ty = Type::getInt64Ty(Ctx);

  for (auto &IT : CurrentWL) {
    IRBuilder<> Builder(IT->Location);

    auto *Ptr2Int = Builder.CreatePtrToInt(IT->Instrumentee, I64Ty);

    auto *Lower = IT->BoundWitness->getLowerBound();
    auto *CmpLower = Builder.CreateICmpULT(Ptr2Int, Lower);

    auto *Sum =
        Builder.CreateAdd(Ptr2Int, ConstantInt::get(I64Ty, IT->AccessSize));
    auto *Upper = IT->BoundWitness->getUpperBound();
    auto *CmpUpper = Builder.CreateICmpUGT(Sum, Upper);

    auto *Or = Builder.CreateOr(CmpLower, CmpUpper);

    auto Unreach = SplitBlockAndInsertIfThen(Or, IT->Location, true);
    Builder.SetInsertPoint(Unreach);
    Builder.CreateCall(IM.getFailFunction());
    IT->invalidate();
  }
}
