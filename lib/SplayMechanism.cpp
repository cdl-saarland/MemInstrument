//===--------- SplayMechanism.cpp -- MemSafety Instrumentation ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/SplayMechanism.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/Util.h"

STATISTIC(SplayNumInboundsChecks, "The # of inbounds checks inserted");
STATISTIC(SplayNumDereferenceChecks, "The # of dereference checks inserted");
STATISTIC(SplayNumBounds, "The # of bound(pairs) materialized");
STATISTIC(SplayNumWitnessPhis, "The # of witness phis inserted");
STATISTIC(SplayNumWitnessSelects, "The # of witness selects inserted");
STATISTIC(SplayNumWitnessLookups, "The # of witness lookups inserted");
STATISTIC(SplayNumGlobals, "The # of globals registered");
STATISTIC(SplayNumFunctions, "The # of functions registered");
STATISTIC(SplayNumAllocas, "The # of allocas registered");

using namespace llvm;
using namespace meminstrument;

namespace {
cl::opt<bool> SplayVerbose("memsafety-splay-verbose",
                           cl::desc("Use verbose splay check functions"),
                           cl::init(false));
}

// FIXME currently, all out-of-bounds pointers are marked invalid here,
// including legal one-after-allocation ones.

llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                        llvm::IRBuilder<> &Builder, llvm::StringRef Suffix) {
  return Builder.CreateBitCast(FromVal, DestType, FromVal->getName() + Suffix);
}

llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                        llvm::IRBuilder<> &Builder) {
  return insertCast(DestType, FromVal, Builder, "_casted");
}

llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                        llvm::Instruction *Location) {
  IRBuilder<> Builder(Location);
  return insertCast(DestType, FromVal, Builder);
}

llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                        llvm::Instruction *Location, llvm::StringRef Suffix) {
  IRBuilder<> Builder(Location);
  return insertCast(DestType, FromVal, Builder, Suffix);
}

llvm::Value *SplayWitness::getLowerBound(void) const { return LowerBound; }

llvm::Value *SplayWitness::getUpperBound(void) const { return UpperBound; }

SplayWitness::SplayWitness(llvm::Value *WitnessValue)
    : Witness(WK_Splay), WitnessValue(WitnessValue) {}

void SplayMechanism::insertWitness(ITarget &Target) const {
  auto *CastVal =
      insertCast(WitnessType, Target.Instrumentee, Target.Location, "_witness");
  Target.BoundWitness = std::make_shared<SplayWitness>(CastVal);
  ++SplayNumWitnessLookups;
}

void SplayMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> Builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());
  auto *CastVal = insertCast(PtrArgType, Target.Instrumentee, Builder);

  std::vector<Value *> Args;
  Args.push_back(Witness->WitnessValue);
  Args.push_back(CastVal);

  bool doDerefCheck = Target.CheckUpperBoundFlag || Target.CheckLowerBoundFlag;

  if (doDerefCheck) {
    auto *I64Type = Type::getInt64Ty(Target.Location->getContext());
    Args.push_back(ConstantInt::get(I64Type, Target.AccessSize));
  }

  if (SplayVerbose) {
    Module *M = Target.Location->getModule();
    auto Name = Target.Location->getFunction()->getName() +
                "::" + Target.Instrumentee->getName();
    auto *Val = insertStringLiteral(*M, Name.str());
    auto *CastedVal = insertCast(PtrArgType, Val, Builder);
    Args.push_back(CastedVal);
  }

  if (doDerefCheck) {
    Builder.CreateCall(CheckDereferenceFunction, Args);
    ++SplayNumDereferenceChecks;
  } else {
    Builder.CreateCall(CheckInboundsFunction, Args);
    ++SplayNumInboundsChecks;
  }
}

void SplayMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.RequiresExplicitBounds);

  IRBuilder<> builder(Target.Location);

  auto *Witness = cast<SplayWitness>(Target.BoundWitness.get());

  std::vector<Value *> Args;
  Args.push_back(Witness->WitnessValue);

  if (Target.CheckUpperBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_upper";
    auto *UpperVal = builder.CreateCall(GetUpperBoundFunction, Args, Name);
    Witness->UpperBound = UpperVal;
  }
  if (Target.CheckLowerBoundFlag) {
    auto Name = Target.Instrumentee->getName() + "_lower";
    auto *LowerVal = builder.CreateCall(GetUpperBoundFunction, Args, Name);
    Witness->LowerBound = LowerVal;
  }
  ++SplayNumBounds;
}

void SplayMechanism::insertFunctionDeclarations(llvm::Module &M) {
  auto &Ctx = M.getContext();
  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *StringTy = Type::getInt8PtrTy(Ctx);

  if (SplayVerbose) {
    CheckInboundsFunction =
        insertFunDecl(M, "__splay_check_inbounds_named", VoidTy, WitnessType,
                      PtrArgType, StringTy);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference_named", VoidTy, WitnessType,
                      PtrArgType, SizeType, StringTy);
  } else {
    CheckInboundsFunction = insertFunDecl(M, "__splay_check_inbounds", VoidTy,
                                          WitnessType, PtrArgType);
    CheckDereferenceFunction =
        insertFunDecl(M, "__splay_check_dereference", VoidTy, WitnessType,
                      PtrArgType, SizeType);
  }

  GetLowerBoundFunction =
      insertFunDecl(M, "__splay_get_lower", PtrArgType, WitnessType);
  GetUpperBoundFunction =
      insertFunDecl(M, "__splay_get_upper", PtrArgType, WitnessType);

  GlobalAllocFunction =
      insertFunDecl(M, "__splay_alloc_or_merge", VoidTy, PtrArgType, SizeType);
  AllocFunction = insertFunDecl(M, "__splay_alloc_or_replace", VoidTy,
                                PtrArgType, SizeType);
}

void SplayMechanism::setupGlobals(llvm::Module &M) {
  auto &Ctx = M.getContext();

  // register a static constructor that inserts all globals into the splay tree
  auto Fun = registerCtors(
      M, std::make_pair<StringRef, int>("__splay_globals_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);
  std::vector<Value *> ArgVals;

  for (auto &GV : M.globals()) {
    if (hasNoInstrument(&GV)) {
      continue;
    }
    DEBUG(dbgs() << "Creating splay init code for GlobalVariable `" << GV
                 << "`\n");
    auto *PtrArg = insertCast(PtrArgType, &GV, Builder);
    ArgVals.push_back(PtrArg); // Pointer

    auto *PtrType = cast<PointerType>(GV.getType());
    auto *PointeeType = PtrType->getElementType();
    uint64_t sz = M.getDataLayout().getTypeAllocSize(PointeeType);
    ArgVals.push_back(
        Constant::getIntegerValue(SizeType, APInt(64, sz))); // Size

    Builder.CreateCall(GlobalAllocFunction, ArgVals);
    ++SplayNumGlobals;
    ArgVals.clear();
  }

  for (auto &F : M.functions()) {
    // insert functions as having size 1
    if (hasNoInstrument(&F) || F.isIntrinsic()) {
      continue;
    }
    DEBUG(dbgs() << "Creating splay init code for Function `" << F << "`\n");
    auto *PtrArg = insertCast(PtrArgType, &F, Builder);

    ArgVals.push_back(PtrArg); // Pointer

    ArgVals.push_back(Constant::getIntegerValue(SizeType, APInt(64, 1)));

    Builder.CreateCall(GlobalAllocFunction, ArgVals);
    ++SplayNumFunctions;
    ArgVals.clear();
  }
  Builder.CreateRetVoid();
}

void SplayMechanism::instrumentAlloca(Module &M, llvm::AllocaInst *AI) {
  IRBuilder<> Builder(AI->getNextNode());
  std::vector<Value *> ArgVals;

  auto *PtrArg = insertCast(PtrArgType, AI, Builder);
  ArgVals.push_back(PtrArg);

  uint64_t sz = M.getDataLayout().getTypeAllocSize(AI->getAllocatedType());

  Value *Size = Constant::getIntegerValue(SizeType, APInt(64, sz));
  if (AI->isArrayAllocation()) {
    Size = Builder.CreateMul(AI->getArraySize(), Size,
                             AI->getName() + "_bytes_size", /*hasNUW*/ true,
                             /*hasNSW*/ false);
  }
  ArgVals.push_back(Size);

  auto *Call = Builder.CreateCall(AllocFunction, ArgVals);
  setNoInstrument(Call);
  ++SplayNumAllocas;
}

void SplayMechanism::initTypes(llvm::LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

bool SplayMechanism::initialize(llvm::Module &M) {
  initTypes(M.getContext());

  insertFunctionDeclarations(M);

  setupGlobals(M);

  for (auto &F : M) {
    if (F.empty() || hasNoInstrument(&F))
      continue;
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I)) {
          instrumentAlloca(M, AI);
        }
      }
    }
  }

  return true;
}

std::shared_ptr<Witness>
SplayMechanism::insertWitnessPhi(ITarget &Target) const {
  auto *Phi = cast<PHINode>(Target.Instrumentee);

  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  Target.BoundWitness = std::make_shared<SplayWitness>(NewPhi);
  ++SplayNumWitnessPhis;
  return Target.BoundWitness;
}

void SplayMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                             std::shared_ptr<Witness> &Incoming,
                                             llvm::BasicBlock *InBB) const {
  auto *PhiWitness = cast<SplayWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<SplayWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

std::shared_ptr<Witness> SplayMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const {
  auto *Sel = cast<SelectInst>(Target.Instrumentee);

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<SplayWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<SplayWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  Target.BoundWitness = std::make_shared<SplayWitness>(NewSel);
  ++SplayNumWitnessSelects;
  return std::make_shared<SplayWitness>(NewSel);
}
