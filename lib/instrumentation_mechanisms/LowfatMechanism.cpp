//===- LowfatMechanism.cpp - Low-Fat Pointer ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/LowfatMechanism.h"

#include "meminstrument-rt/LFSizes.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"

#include "meminstrument/pass/Util.h"

STATISTIC(LowfatNumInboundsChecks, "The # of inbounds checks inserted");
STATISTIC(LowfatNumDereferenceChecks, "The # of dereference checks inserted");
STATISTIC(LowfatNumBounds, "The # of bound(pairs) materialized");
STATISTIC(LowfatNumWitnessPhis, "The # of witness phis inserted");
STATISTIC(LowfatNumWitnessSelects, "The # of witness selects inserted");
STATISTIC(LowfatNumWitnessLookups, "The # of witness lookups inserted");
STATISTIC(LowfatNumAllocsEncountered, "The # of allocas encountered");
STATISTIC(LowfatNumAllocs, "The # of allocas transformed");
STATISTIC(LowfatNumVariableLengthArrays,
          "The # of variable length arrays encountered");

using namespace llvm;
using namespace meminstrument;

cl::opt<bool> NoStackProtection("mi-lf-no-stack-protection",
                                cl::desc("Use lowfat without stack protection"),
                                cl::init(false));

cl::opt<bool> NoVLAProtection(
    "mi-lf-no-vla-protection",
    cl::desc("Use lowfat without variable length array protection"),
    cl::init(false));

Value *LowfatWitness::getLowerBound(void) const { return LowerBound; }

Value *LowfatWitness::getUpperBound(void) const { return UpperBound; }

LowfatWitness::LowfatWitness(Value *WitnessValue, Instruction *Location)
    : Witness(WK_Lowfat), WitnessValue(WitnessValue), Location(Location) {}

Instruction *LowfatWitness::getInsertionLocation() const {
  auto *Res = Location;
  while (isa<PHINode>(Res)) {
    Res = Res->getNextNode();
  }
  return Res;
}

bool LowfatWitness::hasBoundsMaterialized(void) const {
  return UpperBound != nullptr && LowerBound != nullptr;
}

void LowfatMechanism::insertWitnesses(ITarget &Target) const {
  // There should be no targets without an instrumentee for lowfat
  assert(Target.hasInstrumentee());

  auto instrumentee = Target.getInstrumentee();

  if (!instrumentee->getType()->isAggregateType()) {
    auto *CastVal = insertCast(WitnessType, Target.getInstrumentee(),
                               Target.getLocation(), "_witness");
    Target.setSingleBoundWitness(
        std::make_shared<LowfatWitness>(CastVal, Target.getLocation()));

    ++LowfatNumWitnessLookups;
    return;
  }

  // The witnesses for pointers returned from a call/landingpad are the values
  // in the aggregate themselves.
  if (isa<CallBase>(instrumentee) || isa<LandingPadInst>(instrumentee)) {
    IRBuilder<> builder(Target.getLocation());
    // Find all locations of pointer values in the aggregate type
    auto indices = computePointerIndices(instrumentee->getType());
    for (auto index : indices) {
      // Extract the pointer to have the witness at hand.
      auto ptr = builder.CreateExtractValue(instrumentee, index);
      auto *castVal = insertCast(WitnessType, ptr, builder);
      Target.setBoundWitness(
          std::make_shared<LowfatWitness>(castVal, Target.getLocation()),
          index);
    }
    return;
  }

  // The only aggregates that do not need a source are those that are constant
  assert(isa<Constant>(instrumentee));
  auto con = cast<Constant>(instrumentee);

  auto indexToPtr =
      InstrumentationMechanism::getAggregatePointerIndicesAndValues(con);
  for (auto KV : indexToPtr) {
    auto *CastVal =
        insertCast(WitnessType, KV.second, Target.getLocation(), "_witness");
    Target.setBoundWitness(
        std::make_shared<LowfatWitness>(CastVal, Target.getLocation()),
        KV.first);

    ++LowfatNumWitnessLookups;
  }
}

WitnessPtr LowfatMechanism::getRelocatedClone(const Witness &wit,
                                              Instruction *location) const {
  const auto *lowfatWit = dyn_cast<LowfatWitness>(&wit);
  assert(lowfatWit != nullptr);

  ++LowfatNumWitnessLookups;

  return std::make_shared<LowfatWitness>(lowfatWit->WitnessValue, location);
}

void LowfatMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck() || Target.isInvariant());

  IRBuilder<> Builder(Target.getLocation());

  auto *Witness = cast<LowfatWitness>(Target.getSingleBoundWitness().get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.getInstrumentee(), Builder);

  if (Target.isCheck()) {

    // Determine the access width depending on the target kind
    Value *Size = nullptr;
    if (isa<CallCheckIT>(&Target)) {
      Size = ConstantInt::get(SizeType, 1);
    }
    if (auto constSizeTarget = dyn_cast<ConstSizeCheckIT>(&Target)) {
      Size = ConstantInt::get(SizeType, constSizeTarget->getAccessSize());
    }
    if (auto varSizeTarget = dyn_cast<VarSizeCheckIT>(&Target)) {
      Size = varSizeTarget->getAccessSizeVal();
    }

    assert(Size);
    insertCall(Builder, CheckDerefFunction,
               std::vector<Value *>{WitnessVal, CastVal, Size});
    ++LowfatNumDereferenceChecks;
  } else {
    assert(Target.isInvariant());
    insertCall(Builder, CheckOOBFunction,
               std::vector<Value *>{WitnessVal, CastVal});
    ++LowfatNumInboundsChecks;
  }
}

void LowfatMechanism::materializeBounds(ITarget &Target) {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());
  auto witnesses = Target.getBoundWitnesses();

  for (auto kv : witnesses) {

    auto *Witness = cast<LowfatWitness>(kv.second.get());

    if (Witness->hasBoundsMaterialized()) {
      return;
    }

    auto *WitnessVal = Witness->WitnessValue;

    IRBuilder<> Builder(Witness->getInsertionLocation());

    if (Target.hasUpperBoundFlag()) {
      auto Name = Target.getInstrumentee()->getName() + "_upper";
      auto *UpperVal = insertCall(Builder, GetUpperBoundFunction,
                                  std::vector<Value *>{WitnessVal}, Name);
      Witness->UpperBound = UpperVal;
    }
    if (Target.hasLowerBoundFlag()) {
      auto Name = Target.getInstrumentee()->getName() + "_lower";
      auto *LowerVal = insertCall(Builder, GetLowerBoundFunction,
                                  std::vector<Value *>{WitnessVal}, Name);
      Witness->LowerBound = LowerVal;
    }
    ++LowfatNumBounds;
  }
}

FunctionCallee LowfatMechanism::getFailFunction(void) const {
  return failFunction;
}

FunctionCallee LowfatMechanism::getVerboseFailFunction(void) const {
  return verboseFailFunction;
}

void LowfatMechanism::initialize(Module &module) {
  initTypes(module.getContext());
  insertFunctionDeclarations(module);

  if (NoStackProtection) {
    return;
  }

  SmallVector<AllocaInst *, 5> allocas;
  for (auto &fun : module) {
    if (fun.isDeclaration() || hasNoInstrument(&fun))
      continue;

    for (auto &bb : fun) {
      for (auto &inst : bb) {
        if (auto *alloc = dyn_cast<AllocaInst>(&inst)) {
          allocas.push_back(alloc);
        }
        if (isLifeTimeIntrinsic(&inst)) {
          MemInstrumentError::report(
              "Found a call to a lifetime intrinsic, but the lowfat stack "
              "protection does not support them. Don't use "
              "`-mi-no-lifetime-killer` with clang+lowfat+stack. In case you "
              "use opt, make sure to add `-lifetimekiller` before "
              "`-meminstrument`.");
        }
      }
    }
  }

  for (auto *alloc : allocas) {
    instrumentAlloca(alloc);
  }
}

WitnessPtr LowfatMechanism::getWitnessPhi(PHINode *Phi) const {
  IRBuilder<> builder(Phi);

  auto Name = Phi->getName() + "_witness";
  auto *NewPhi =
      builder.CreatePHI(WitnessType, Phi->getNumIncomingValues(), Name);

  ++LowfatNumWitnessPhis;
  return std::make_shared<LowfatWitness>(NewPhi, Phi);
}

void LowfatMechanism::addIncomingWitnessToPhi(WitnessPtr &Phi,
                                              WitnessPtr &Incoming,
                                              BasicBlock *InBB) const {
  auto *PhiWitness = cast<LowfatWitness>(Phi.get());
  auto *PhiVal = cast<PHINode>(PhiWitness->WitnessValue);

  auto *InWitness = cast<LowfatWitness>(Incoming.get());
  PhiVal->addIncoming(InWitness->WitnessValue, InBB);
}

WitnessPtr LowfatMechanism::getWitnessSelect(SelectInst *Sel,
                                             WitnessPtr &TrueWitness,
                                             WitnessPtr &FalseWitness) const {

  IRBuilder<> builder(Sel);

  auto *TrueVal = cast<LowfatWitness>(TrueWitness.get())->WitnessValue;
  auto *FalseVal = cast<LowfatWitness>(FalseWitness.get())->WitnessValue;

  auto Name = Sel->getName() + "_witness";
  auto *NewSel =
      builder.CreateSelect(Sel->getCondition(), TrueVal, FalseVal, Name);

  ++LowfatNumWitnessSelects;
  return std::make_shared<LowfatWitness>(NewSel, Sel);
}

bool LowfatMechanism::invariantsAreChecks() const { return true; }

//===---------------------------- private ---------------------------------===//

void LowfatMechanism::initTypes(LLVMContext &Ctx) {
  WitnessType = Type::getInt8PtrTy(Ctx);
  PtrArgType = Type::getInt8PtrTy(Ctx);
  SizeType = Type::getInt64Ty(Ctx);
}

void LowfatMechanism::insertFunctionDeclarations(Module &M) {

  // Register common functions
  insertCommonFunctionDeclarations(M);

  auto &Ctx = M.getContext();
  auto *VoidTy = Type::getVoidTy(Ctx);

  CheckDerefFunction = insertFunDecl(M, "__lowfat_check_deref", VoidTy,
                                     WitnessType, PtrArgType, SizeType);
  CheckOOBFunction =
      insertFunDecl(M, "__lowfat_check_oob", VoidTy, WitnessType, PtrArgType);
  GetUpperBoundFunction =
      insertFunDecl(M, "__lowfat_get_upper_bound", PtrArgType, WitnessType);
  GetLowerBoundFunction =
      insertFunDecl(M, "__lowfat_get_lower_bound", PtrArgType, WitnessType);
  StackMirrorFunction =
      insertFunDecl(M, "__lowfat_get_mirror", PtrArgType, PtrArgType, SizeType);
  StackSizesFunction =
      insertFunDecl(M, "__lowfat_lookup_stack_size", SizeType, SizeType);
  StackOffsetFunction =
      insertFunDecl(M, "__lowfat_lookup_stack_offset", SizeType, SizeType);
  StackMaskFunction = insertFunDecl(M, "__lowfat_compute_aligned", PtrArgType,
                                    PtrArgType, SizeType);
}

void LowfatMechanism::mirrorPointerAndReplaceAlloca(IRBuilder<> &builder,
                                                    AllocaInst *oldAlloc,
                                                    Instruction *newAlloc,
                                                    Value *offset) const {

  auto newAllocCasted = builder.CreateBitCast(newAlloc, PtrArgType);

  Value *mirroredPtr =
      insertCall(builder, StackMirrorFunction,
                 std::vector<Value *>{newAllocCasted, offset}, "lf.mirror");

  if (oldAlloc->getType() != mirroredPtr->getType()) {
    mirroredPtr = builder.CreateBitCast(mirroredPtr, oldAlloc->getType());
  }

  // Replace the uses of the old alloc
  SmallVector<User *, 5> toRepl;
  for (auto *usr : oldAlloc->users()) {
    toRepl.push_back(usr);
  }
  for (auto *elem : toRepl) {
    elem->replaceUsesOfWith(oldAlloc, mirroredPtr);
  }

  // Drop the old alloc
  oldAlloc->removeFromParent();
  oldAlloc->deleteValue();
}

void LowfatMechanism::handleVariableLengthArray(AllocaInst *alloc) const {

  if (NoVLAProtection) {
    return;
  }

  IRBuilder<> builder(alloc);
  auto &DL = alloc->getModule()->getDataLayout();
  // Generate a value for the allocation size
  // The size is the number of elements times the size of the element
  auto typeSize = DL.getTypeAllocSize(alloc->getAllocatedType());
  auto allocSize = builder.CreateMul(alloc->getArraySize(),
                                     builder.getInt64(typeSize), "lf.vla.size");

  LLVM_DEBUG(dbgs() << "Alloc Size: " << allocSize << "\n";);

  // Determine the index
  auto ctlzFun =
      Intrinsic::getDeclaration(alloc->getModule(), Intrinsic::ctlz,
                                {Type::getInt64Ty(alloc->getContext())});
  // TODO we disallow size zero here by handing over true, does this make sense?
  auto index = insertCall(
      builder, ctlzFun, std::vector<Value *>{allocSize, builder.getInt1(true)},
      "lf.index");

  LLVM_DEBUG(dbgs() << "Index: " << *index << "\n";);

  // Look up the lf size for the allocation
  auto newAllocAlignedSize =
      insertCall(builder, StackSizesFunction, index, "lf.new.vla.size");

  // Allocate an array of the computed size
  auto newAllocAligned = builder.CreateAlloca(
      builder.getInt8Ty(), newAllocAlignedSize, "lf.vla.alloc");
  setNoInstrument(newAllocAligned);

  LLVM_DEBUG(dbgs() << "New alloc: " << *newAllocAligned << "\n";);

  // Compute the offset for the mirrored pointer
  auto offset =
      insertCall(builder, StackOffsetFunction, index, "lf.vla.offset");

  LLVM_DEBUG(dbgs() << "Offset: " << *offset << "\n";);
  // Compute the alignment
  auto alignAlloc = insertCall(builder, StackMaskFunction,
                               std::vector<Value *>{newAllocAligned, index},
                               "lf.align.alloc");

  LLVM_DEBUG(dbgs() << "Align alloc: " << *alignAlloc << "\n";);

  // Manipulate the stack pointer to fulfill alignment conditions
  auto stackRestoreFun =
      Intrinsic::getDeclaration(alloc->getModule(), Intrinsic::stackrestore);

  LLVM_DEBUG(dbgs() << "Stack restore function: " << *stackRestoreFun << "\n";);
  auto stackRestoreCall = insertCall(builder, stackRestoreFun, alignAlloc);

  LLVM_DEBUG(dbgs() << "Stack restore: " << *stackRestoreCall << "\n";);

  mirrorPointerAndReplaceAlloca(builder, alloc, alignAlloc, offset);
}

void LowfatMechanism::instrumentAlloca(AllocaInst *alloc) const {

  if (hasNoInstrument(alloc) || hasVarArgHandling(alloc)) {
    return;
  }

  LowfatNumAllocsEncountered++;
  auto &DL = alloc->getModule()->getDataLayout();

  auto allocatedSize = alloc->getAllocationSizeInBits(DL);

  if (!allocatedSize) {
    ++LowfatNumVariableLengthArrays;
    handleVariableLengthArray(alloc);
    return;
  }

  // Determine the allocation size (add one to ensure that the allocation is at
  // least one byte larger to avoid one-past-end errors)
  // TODO might actually not be all too reasonable to use the intrinsic here, as
  // CT is not what we care about
  auto index = __builtin_clzll((allocatedSize.getValue() / 8));
  LLVM_DEBUG(dbgs() << "Index: " << index << "\n";);

  // Make sure the allocation is small enough to be handled by the mechanism
  // (few leading zeros, large number)
  if (index <= __builtin_clzll(MAX_PERMITTED_LF_STACK_ALLOC_SIZE)) {
    LLVM_DEBUG(dbgs() << "Allocation is too large\n";);
    return;
  }

  LowfatNumAllocs++;
  auto allocSize = STACK_SIZES[index];
  LLVM_DEBUG(dbgs() << "Alloc size: " << allocSize << "\n";);

  IRBuilder<> builder(alloc);
  // Make the aligned allocation
  auto algn = Align(~STACK_MASKS[index] + 1);
  LLVM_DEBUG(dbgs() << "Mask value: " << algn.value() << "\n";);

  auto newAllocAligned = builder.CreateAlloca(
      builder.getInt8Ty(), builder.getInt64(allocSize), "lf.alloc");
  newAllocAligned->setAlignment(algn > alloc->getAlign() ? algn
                                                         : alloc->getAlign());

  // Create the mirror pointer
  auto offset = STACK_OFFSETS[index];

  // Mirror the pointer and replace its uses
  mirrorPointerAndReplaceAlloca(builder, alloc, newAllocAligned,
                                builder.getInt64(offset));
}
