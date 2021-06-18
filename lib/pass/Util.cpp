//===---------------------------------------------------------------------===///
///
/// \file  See corresponding header for more descriptions.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Metadata.h"

#include "meminstrument/Config.h"

using namespace llvm;
using namespace meminstrument;

#define MEMINSTRUMENT_MD "meminstrument"
#define NOINSTRUMENT_MD "no_instrument"
#define VARARG_MD "vararg_handling"
#define VARARG_LOAD_MD "vararg_load_arg"

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

void setNoInstrument(Value *V) { addMIMetadata(V, NOINSTRUMENT_MD); }

void setVarArgHandling(Value *V) { addMIMetadata(V, VARARG_MD); }

void setVarArgLoadArg(Value *V) { addMIMetadata(V, VARARG_LOAD_MD); }

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

bool isVarArgMetadataType(Type *type) {
  auto varArgStructName = "struct.__va_list_tag";
  if (PointerType *pTy = dyn_cast<PointerType>(type)) {
    type = pTy->getPointerElementType();
  }
  if (StructType *sTy = dyn_cast<StructType>(type)) {
    if (sTy->hasName() && sTy->getName() == varArgStructName) {
      return true;
    }
  }
  if (ArrayType *arTy = dyn_cast<ArrayType>(type)) {
    Type *someTy = arTy->getElementType();
    if (StructType *stTy = dyn_cast<StructType>(someTy)) {
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

  return handles;
}

} // namespace meminstrument
