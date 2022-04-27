//===- MemInstrumentPass.cpp - Main Instrumentation Pass ------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/MemInstrumentPass.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/optimizations/OptimizationRunner.h"
#include "meminstrument/pass/CheckGeneration.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/ITargetGathering.h"
#include "meminstrument/pass/Setup.h"
#include "meminstrument/pass/WitnessGeneration.h"

#include "meminstrument/pass/Util.h"

#include "llvm/IR/LLVMContext.h"

#include <unordered_set>

using namespace meminstrument;
using namespace llvm;

char MemInstrumentPass::ID = 0;

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

auto dropAttributes(LLVMContext &context,
                    const std::vector<Attribute::AttrKind> &attrsToDrop,
                    const AttributeList &attrs, int numArgs) -> AttributeList {

  if (attrs.isEmpty()) {
    return attrs;
  }

  AttrBuilder funAttrBuilder(attrs.getFnAttributes());
  for (const auto &attrib : attrsToDrop) {
    funAttrBuilder.removeAttribute(attrib);
  }

  SmallVector<AttributeSet, 5> argAttrs;
  for (int i = 0; i < numArgs; i++) {
    AttrBuilder argAttrBuilder(attrs.getParamAttributes(i));
    for (const auto &attrib : attrsToDrop) {
      argAttrBuilder.removeAttribute(attrib);
    }
    argAttrs.push_back(AttributeSet::get(context, argAttrBuilder));
  }

  return AttributeList::get(context, AttributeSet::get(context, funAttrBuilder),
                            attrs.getRetAttributes(), argAttrs);
}

void dropFunctionAttributes(
    LLVMContext &context, const std::vector<Attribute::AttrKind> &attrsToDrop,
    llvm::Module &M, const std::unordered_set<llvm::Function *> skipFunctions) {

  for (auto &fun : M) {
    if (fun.isIntrinsic()) {
      // we won't instrument those, and LLVM complains about intrinsics
      // without certain attributes
      continue;
    }
    if (skipFunctions.count(&fun) == 0) {
      auto attrs = fun.getAttributes();
      auto newAttrs = dropAttributes(context, attrsToDrop, attrs,
                                     fun.getFunctionType()->getNumParams());
      fun.setAttributes(newAttrs);
    }
    if (fun.isDeclaration()) {
      continue;
    }
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (!isa<CallInst>(inst)) {
          continue;
        }
        auto &callInst = cast<CallBase>(inst);
        auto *calledfun = callInst.getCalledFunction();
        if (calledfun == nullptr || skipFunctions.count(&fun) == 0) {
          auto attrs = callInst.getAttributes();
          auto newAttrs =
              dropAttributes(context, attrsToDrop, attrs,
                             callInst.getFunctionType()->getNumParams());
          callInst.setAttributes(newAttrs);
        }
      }
    }
  }
}

void dropInvalidatedFunctionAttributes(
    const InstrumentationMechanism &IM, Module &M,
    std::map<Function *, ITargetVector> &TargetMap) {
  // Memory safety instrumentations add possible defined behavior to most
  // functions, we need to make sure to eliminate all function attributes
  // that are invalidated by this newly-introduced behavior so that
  // subsequent optimizations are not based on wrong assumptions.

  auto &context = M.getContext();

  // first, drop willreturn attributes on all functions that may contain a
  // check (since checks can cause a function to not return)
  auto dropWillReturn = {
      Attribute::WillReturn,
  };

  std::unordered_set<Function *> noDropWillReturn;

  // Find all functions of which we know that they don't contain checks and
  // they don't call any functions. They can keep willreturn attributes
  for (auto &fun : M) {
    if (fun.isDeclaration()) {
      continue;
    }
    bool isClean = true;

    const auto &targetlist = TargetMap[&fun];
    for (const auto &it : targetlist) {
      if (!it->isValid()) {
        continue;
      }
      if (it->isCheck() || (it->isInvariant() && IM.invariantsAreChecks())) {
        isClean = false;
        break;
      }
    }
    if (!isClean) {
      continue;
    }
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (isa<CallBase>(inst)) {
          isClean = false;
          break;
        }
      }
    }
    if (isClean) {
      noDropWillReturn.insert(&fun);
    }
  }

  dropFunctionAttributes(context, dropWillReturn, M, noDropWillReturn);

  auto dropNoMem = {Attribute::ReadNone,
                    Attribute::ReadOnly,
                    Attribute::WriteOnly,
                    Attribute::ArgMemOnly,
                    Attribute::InaccessibleMemOnly,
                    Attribute::InaccessibleMemOrArgMemOnly};

  std::unordered_set<Function *> noDropNoMem;

  // Find all functions of which we know that they don't contain checks or
  // invariants and they don't call any functions. They can keep NoMem-style
  // attributes
  for (auto &fun : M) {
    if (fun.isDeclaration()) {
      continue;
    }
    bool isClean = true;

    const auto &targetlist = TargetMap[&fun];
    for (const auto &it : targetlist) {
      if (!it->isValid()) {
        continue;
      }
      if (it->isCheck() || it->isInvariant()) {
        isClean = false;
        break;
      }
    }
    if (!isClean) {
      continue;
    }
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (isa<CallBase>(inst)) {
          isClean = false;
          break;
        }
      }
    }
    if (isClean) {
      noDropNoMem.insert(&fun);
    }
  }

  dropFunctionAttributes(context, dropNoMem, M, noDropNoMem);
}

bool MemInstrumentPass::runOnModule(Module &M) {

  CFG = GlobalConfig::create(M);

  MIMode Mode = CFG->getMIMode();

  if (Mode == MIMode::NOTHING)
    return true;

  auto &IM = CFG->getInstrumentationMechanism();
  if (M.getName().endswith("tools/timeit.c") ||
      M.getName().endswith("fpcmp.c")) {
    // small hack to avoid unnecessary work in the lnt tests
    LLVM_DEBUG(dbgs() << "MemInstrumentPass: skip module `" << M.getName().str()
                      << "`\n";);
    return IM.skipInstrumentation(M);
  }

  LLVM_DEBUG(dbgs() << "Dumped module:\n"; M.dump();
             dbgs() << "\nEnd of dumped module.\n";);

  // Apply transformations required for all instrumentations
  prepareModule(M);

  LLVM_DEBUG(dbgs() << "MemInstrumentPass: processing module `"
                    << M.getName().str() << "`\n";);

  OptimizationRunner optRunner(*this);
  optRunner.runPreparation(M);

  LLVM_DEBUG(
      dbgs() << "MemInstrumentPass: setting up instrumentation mechanism\n";);

  IM.initialize(M);

  if (Mode == MIMode::SETUP)
    return true;

  std::map<Function *, ITargetVector> TargetMap;

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];
    LLVM_DEBUG(dbgs() << "MemInstrumentPass: gathering ITargets for function `"
                      << F.getName() << "`\n";);

    gatherITargets(*CFG, Targets, F);
  }

  if (Mode == MIMode::GATHER_ITARGETS)
    return true;

  optRunner.updateITargets(TargetMap);

  if (Mode == MIMode::FILTER_ITARGETS)
    return true;

  dropInvalidatedFunctionAttributes(IM, M, TargetMap);

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    LLVM_DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_WITNESSES)
      continue;

    generateInvariants(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_INVARIANTS) {
      continue;
    }

    optRunner.placeChecks(Targets, F);

    if (Mode == MIMode::GENERATE_OPTIMIZATION_CHECKS)
      continue;

    LLVM_DEBUG(dbgs() << "MemInstrumentPass: generating checks\n";);

    generateChecks(*CFG, Targets, F);
  }

  DEBUG_ALSO_WITH_TYPE("meminstrument-finalmodule", M.dump(););

  return true;
}

void MemInstrumentPass::releaseMemory(void) { CFG.reset(nullptr); }

void MemInstrumentPass::getAnalysisUsage(AnalysisUsage &AU) const {
  OptimizationRunner::addRequiredAnalyses(AU);
}

void MemInstrumentPass::print(raw_ostream &O, const Module *M) const {
  O << "MemInstrumentPass for module\n" << *M << "\n";
}

GlobalConfig &MemInstrumentPass::getConfig(void) { return *CFG; }
