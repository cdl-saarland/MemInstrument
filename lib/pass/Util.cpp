//===- Util.cpp - Helper functions ----------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

#include "llvm/IR/Metadata.h"

using namespace llvm;
using namespace meminstrument;

#define MEMINSTRUMENT_MD "meminstrument"
#define NOINSTRUMENT_MD "no_instrument"
#define VARARG_MD "vararg_handling"
#define VARARG_LOAD_MD "vararg_load_arg"
#define PTR_DEOB_MD "ptr_deobfuscation"
#define ACCESS_ID "mi_access_id"

namespace {
bool hasMDStrImpl(const char *ref, const MDNode *N) {
  if (!N || N->getNumOperands() < 1) {
    return false;
  }

  for (unsigned i = 0; i < N->getNumOperands(); i++) {
    if (const auto *Str = dyn_cast<MDString>(N->getOperand(i))) {
      if (Str->getString().equals(ref)) {
        return true;
      }
    }
  }

  return false;
}

void addMIMetadata(Value *V, const char *toAdd) {
  auto &Ctx = V->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, toAdd));
  if (auto *O = dyn_cast<GlobalObject>(V)) {
    N = MDNode::concatenate(O->getMetadata(MEMINSTRUMENT_MD), N);
    O->setMetadata(MEMINSTRUMENT_MD, N);
  } else if (auto *I = dyn_cast<Instruction>(V)) {
    N = MDNode::concatenate(I->getMetadata(MEMINSTRUMENT_MD), N);
    I->setMetadata(MEMINSTRUMENT_MD, N);
  } else {
    llvm_unreachable("Value not supported for setting metadata!");
  }
}

} // namespace

namespace meminstrument {

char MemInstrumentError::ID = 0;

std::error_code MemInstrumentError::convertToErrorCode() const {
  return inconvertibleErrorCode();
}

void MemInstrumentError::log(raw_ostream &stream) const {
  stream << "[Meminstrument Error] " << errorMessage;
}

void MemInstrumentError::report(const Twine &msg) {
  ExitOnError()(make_error<MemInstrumentError>(msg));
  llvm_unreachable("Error reporting failed.");
}

void MemInstrumentError::report(const Twine &msg, const Value *val) {
  auto str = msg.str();
  raw_string_ostream stream(str);
  stream << *val;
  MemInstrumentError::report(str);
}

void setNoInstrument(Value *V) { addMIMetadata(V, NOINSTRUMENT_MD); }

void setVarArgHandling(Value *V) { addMIMetadata(V, VARARG_MD); }

void setVarArgLoadArg(Value *V) { addMIMetadata(V, VARARG_LOAD_MD); }

void setPointerDeobfuscation(Value *V) { addMIMetadata(V, PTR_DEOB_MD); }

void setAccessID(Instruction *inst, uint64_t id) {
  auto &ctx = inst->getContext();
  MDNode *mdNode = MDNode::get(ctx, MDString::get(ctx, std::to_string(id)));
  inst->setMetadata(ACCESS_ID, mdNode);
}

bool hasNoInstrument(const GlobalObject *O) {
  return hasMDStrImpl(NOINSTRUMENT_MD, O->getMetadata(MEMINSTRUMENT_MD));
}

bool hasNoInstrument(const Instruction *I) {
  return hasMDStrImpl(NOINSTRUMENT_MD, I->getMetadata(MEMINSTRUMENT_MD));
}

bool hasVarArgHandling(const Instruction *I) {
  return hasMDStrImpl(VARARG_MD, I->getMetadata(MEMINSTRUMENT_MD));
}

bool hasVarArgLoadArg(const Instruction *I) {
  return hasMDStrImpl(VARARG_LOAD_MD, I->getMetadata(MEMINSTRUMENT_MD));
}

bool hasAccessID(Instruction *inst) { return inst->getMetadata(ACCESS_ID); }

uint64_t getAccessID(Instruction *inst) {
  assert(hasAccessID(inst));
  auto mdNode = inst->getMetadata(ACCESS_ID);

  assert(mdNode->getNumOperands() == 1);
  assert(isa<MDString>(mdNode->getOperand(0)));

  auto *strOp = cast<MDString>(mdNode->getOperand(0));
  uint64_t result;
  strOp->getString().getAsInteger(10, result);
  return result;
}

bool hasPointerAccessSize(const Value *V) {
  const auto *Ty = V->getType();
  if (!Ty->isPointerTy()) {
    return false;
  }

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return true;
  }

  if (!PointeeType->isSized()) {
    return false;
  }

  return true;
}

size_t getPointerAccessSize(const DataLayout &DL, Value *V) {
  assert(hasPointerAccessSize(V));

  auto *Ty = V->getType();

  auto *PointeeType = Ty->getPointerElementType();

  if (PointeeType->isFunctionTy()) {
    return 0;
  }

  size_t Size = DL.getTypeStoreSize(PointeeType);

  return Size;
}

bool canHoldMetadata(const Value *V) {
  return isa<Instruction>(V) || isa<GlobalObject>(V);
}

bool isLifeTimeIntrinsic(const Instruction *inst) {
  if (auto *intr = dyn_cast<IntrinsicInst>(inst)) {
    switch (intr->getIntrinsicID()) {
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
      return true;
    default:;
    }
  }
  return false;
}

bool isVarArgMetadataType(const Type *type) {
  auto varArgStructName = "struct.__va_list_tag";
  if (const auto *pTy = dyn_cast<PointerType>(type)) {
    type = pTy->getPointerElementType();
  }
  if (const auto *sTy = dyn_cast<StructType>(type)) {
    if (sTy->hasName() && sTy->getName() == varArgStructName) {
      return true;
    }
  }
  if (const auto *arTy = dyn_cast<ArrayType>(type)) {
    const auto *someTy = arTy->getElementType();
    if (const auto *stTy = dyn_cast<StructType>(someTy)) {
      if (stTy->hasName() && stTy->getName() == varArgStructName) {
        return true;
      }
    }
  }
  return false;
}

SmallVector<Value *, 2> getVarArgHandles(Function &F) {

  SmallVector<Value *, 2> handles;
  if (F.isDeclaration()) {
    return handles;
  }

  auto &entry = F.getEntryBlock();
  for (auto &inst : entry) {
    // Search for allocations of the vararg type in the entry block
    if (AllocaInst *allocInst = dyn_cast<AllocaInst>(&inst)) {
      if (auto allocedType = allocInst->getAllocatedType()) {
        if (isVarArgMetadataType(allocedType)) {
          handles.push_back(allocInst);
        }
      }
    }
  }

  // Search for va_list arguments
  for (auto &arg : F.args()) {
    Type *argTy = arg.getType();
    if (isVarArgMetadataType(argTy)) {
      handles.push_back(&arg);
    }
  }

  // Search for loads that load va_lists/calls that return them everywhere
  for (auto &bb : F) {
    for (auto &inst : bb) {
      if (isa<LoadInst>(&inst) || isa<CallInst>(&inst)) {
        if (isVarArgMetadataType(inst.getType())) {
          handles.push_back(&inst);
        }
      }
    }
  }

  return handles;
}

} // namespace meminstrument
