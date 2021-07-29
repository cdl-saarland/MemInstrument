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
#include "meminstrument/pass/WitnessGeneration.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

STATISTIC(NumVarArgs, "The # of function using varargs or a va_list");
STATISTIC(NumVarArgMetadataLoadStore,
          "The # of loads and stores tagged as varargs metadata management");

// Number of byval arguments converted.
STATISTIC(ByValArgsConverted, "Number of byval arguments converted at calls");

char MemInstrumentPass::ID = 0;

MemInstrumentPass::MemInstrumentPass() : ModulePass(ID) {}

bool MemInstrumentPass::runOnModule(Module &M) {

  CFG = GlobalConfig::create(M);

  MIMode Mode = CFG->getMIMode();

  if (Mode == MIMode::NOTHING)
    return true;

  auto &IM = CFG->getInstrumentationMechanism();
  if (M.getName().endswith("tools/timeit.c")) {
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

  if (Mode == MIMode::SETUP || CFG->hasErrors())
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

  if (Mode == MIMode::GATHER_ITARGETS || CFG->hasErrors())
    return true;

  optRunner.updateITargets(TargetMap);

  if (Mode == MIMode::FILTER_ITARGETS || CFG->hasErrors())
    return true;

  for (auto &F : M) {
    if (F.isDeclaration() || hasNoInstrument(&F)) {
      continue;
    }

    auto &Targets = TargetMap[&F];

    LLVM_DEBUG(dbgs() << "MemInstrumentPass: generating Witnesses\n";);

    generateWitnesses(*CFG, Targets, F);

    if (Mode == MIMode::GENERATE_WITNESSES || CFG->hasErrors())
      continue;

    optRunner.placeChecks(Targets, F);

    if (Mode == MIMode::GENERATE_OPTIMIZATION_CHECKS || CFG->hasErrors())
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

//===---------------------------- private ---------------------------------===//

namespace {

void labelAccesses(Function &F) {
  // This heavily relies on clang and llvm behaving deterministically
  // (which may or may not be the case)
  uint64_t idx = 0;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isa<StoreInst>(&I) || isa<LoadInst>(&I)) {
        setAccessID(&I, idx++);
      }
    }
  }
}

bool markVarargInsts(Function &fun) {

  // Determine which value contains the metadata for vararg accesses
  SmallVector<Value *, 2> handles = getVarArgHandles(fun);
  if (handles.empty()) {
    // If there are not varargs, we have nothing to do.
    return false;
  }

  // Determine loads and stores to this data structure.
  SmallVector<std::pair<Instruction *, unsigned>, 12> worklist;
  for (auto *handle : handles) {

    DEBUG_WITH_TYPE("MIVarArg",
                    { dbgs() << "Vararg handle: " << *handle << "\n"; });

    // Mark the handle itself `noinstrument` if possible.
    if (auto inst = dyn_cast<Instruction>(handle)) {
      setVarArgHandling(inst);
    }

    // Initialize the work list with all direct users of the vargargs
    for (auto *user : handle->users()) {
      if (auto inst = dyn_cast<Instruction>(user)) {
        worklist.push_back(std::make_pair(inst, 2));
      }
    }
  }

  while (!worklist.empty()) {
    Instruction *entry;
    unsigned propLevel;
    std::tie(entry, propLevel) = worklist.pop_back_val();

    // Label vararg specific calls.
    if (auto *intrInst = dyn_cast<IntrinsicInst>(entry)) {
      switch (intrInst->getIntrinsicID()) {
      case Intrinsic::vastart:
      case Intrinsic::vacopy:
      case Intrinsic::vaend: {
        // Mark the call as vararg related
        setVarArgHandling(intrInst);
        break;
      }
      default:
        break;
      }
      continue;
    }

    // Don't label calls just because varargs are handed over.
    if (isa<CallBase>(entry)) {
      continue;
    }

    // If the instruction is already marked as no instrument, ignore it
    if (hasNoInstrument(entry) || hasVarArgHandling(entry)) {
      continue;
    }

    // Collect some statistics on how many loads/stores belong to the vararg
    // metadata handling
    if (isa<LoadInst>(entry) || isa<StoreInst>(entry)) {
      ++NumVarArgMetadataLoadStore;
    }

    DEBUG_WITH_TYPE("MIVarArg", {
      dbgs() << "Vararg handle (Lvl " << propLevel << "): " << *entry << "\n";
    });

    // Don't instrument this vararg related instruction or value
    setVarArgHandling(entry);

    // The vararg metadata has two levels of metadata loads, decrease the level
    // when encountering a load
    if (isa<LoadInst>(entry)) {
      assert(propLevel > 0);
      propLevel--;

      // This load does actually load arguments that are handed over to the
      // function, mark it.
      if (propLevel == 0) {
        setVarArgLoadArg(entry);
      }
    }

    // Stop following users when the propagation level hits zero
    if (propLevel == 0) {
      continue;
    }

    // Add all users to the worklist
    for (auto *user : entry->users()) {
      if (auto inst = dyn_cast<Instruction>(user)) {
        worklist.push_back(std::make_pair(inst, propLevel));
      }
    }
  }

  return true;
}

/// Transform the given call byval arguments (if any) to hand over a pointer
/// to the call site allocated stuff.
void transformCallByValArgs(CallBase &call, IRBuilder<> &builder) {

  for (unsigned i = 0; i < call.getNumArgOperands(); i++) {
    // Find the byval attributes
    if (call.isByValArgument(i)) {

      ++ByValArgsConverted;

      auto *callArg = call.getArgOperand(i);
      assert(callArg->getType()->isPointerTy());
      auto callArgElemType = callArg->getType()->getPointerElementType();

      // Allocate memory for the value about to be copied
      builder.SetInsertPoint(
          &(*call.getCaller()->getEntryBlock().getFirstInsertionPt()));
      auto alloc = builder.CreateAlloca(callArgElemType);
      alloc->setName("byval.alloc");

      // Copy the value into the alloca
      builder.SetInsertPoint(&call);
      auto size =
          call.getModule()->getDataLayout().getTypeAllocSize(callArgElemType);
      auto cpy = builder.CreateMemCpy(alloc, alloc->getAlignment(), callArg,
                                      alloc->getAlignment(), size);
      // TODO We cannot mark this memcpy "noinstrument" as it might be relevant
      // to propagate metadata for some instrumentations. However, a check on
      // dest is redundant, as we generate the dest ourselves with a valid size
      // (hopefully).
      if (!cpy->getType()->isVoidTy()) {
        cpy->setName("byval.cpy");
      }

      // Replace the old argument with the newly copied one and make sure it is
      // no longer classified as byval.
      call.setArgOperand(i, alloc);
      call.removeParamAttr(i, Attribute::AttrKind::ByVal);
    }
  }

  return;
}

/// Transform all functions that have byval arguments to functions without
/// byval arguments (allocate memory for them at every call site).
void transformByValFunctions(Module &module) {

  IRBuilder<> builder(module.getContext());
  for (Function &fun : module) {

    // Remove byval attributes from functions
    for (Argument &arg : fun.args()) {
      if (arg.hasByValAttr()) {
        // Remove the attribute
        arg.removeAttr(Attribute::AttrKind::ByVal);
      }
    }

    if (fun.isDeclaration()) {
      continue;
    }

    // Remove byval arguments from calls
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (auto *cb = dyn_cast<CallBase>(&inst)) {
          if (cb->hasByValArgument()) {
            transformCallByValArgs(*cb, builder);
          }
        }
      }
    }
  }
}

} // namespace

void MemInstrumentPass::prepareModule(Module &module) {

  for (auto &fun : module) {
    if (fun.isDeclaration()) {
      continue;
    }

    labelAccesses(fun);
    // If the function has varargs or a va_list, mark bookkeeping loads and
    // stores for the vararg handling as such.
    if (markVarargInsts(fun)) {
      NumVarArgs++;
    }
  }

  // Remove `byval` attributes by allocating them at call sites
  transformByValFunctions(module);
}
