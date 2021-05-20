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
#define BYVAL_HANDLING_MD "byval_handling"

namespace {
bool hasMDStrImpl(const char *ref, const MDNode *N) {
  if (!N || N->getNumOperands() < 1) {
    return false;
  }

  if (const auto *Str = dyn_cast<MDString>(N->getOperand(0))) {
    if (Str->getString().equals(ref)) {
      return true;
    }
  }

  return false;
}
} // namespace

namespace meminstrument {

void setNoInstrument(Value *V) {
  auto &Ctx = V->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, NOINSTRUMENT_MD));
  if (auto *O = dyn_cast<GlobalObject>(V)) {
    O->setMetadata(MEMINSTRUMENT_MD, N);
  } else if (auto *I = dyn_cast<Instruction>(V)) {
    I->setMetadata(MEMINSTRUMENT_MD, N);
  } else {
    llvm_unreachable("Value not supported for setting metadata!");
  }
}

void setByvalHandling(Instruction *I) {
  auto &Ctx = I->getContext();
  MDNode *N = MDNode::get(Ctx, MDString::get(Ctx, BYVAL_HANDLING_MD));
  I->setMetadata(BYVAL_HANDLING_MD, N);
}

bool hasNoInstrument(const GlobalObject *O) {
  return hasMDStrImpl(NOINSTRUMENT_MD, O->getMetadata(MEMINSTRUMENT_MD));
}

bool hasNoInstrument(const Instruction *O) {
  return hasMDStrImpl(NOINSTRUMENT_MD, O->getMetadata(MEMINSTRUMENT_MD));
}

bool hasByvalHandling(const Instruction *O) {
  return hasMDStrImpl(BYVAL_HANDLING_MD, O->getMetadata(BYVAL_HANDLING_MD));
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
  auto vaargStructName = "struct.__va_list_tag";
  if (PointerType *pTy = dyn_cast<PointerType>(type)) {
    type = pTy->getPointerElementType();
  }
  if (StructType *sTy = dyn_cast<StructType>(type)) {
    if (sTy->hasName() && sTy->getName() == vaargStructName) {
      return true;
    }
  }
  if (ArrayType *arTy = dyn_cast<ArrayType>(type)) {
    Type *someTy = arTy->getElementType();
    if (StructType *stTy = dyn_cast<StructType>(someTy)) {
      if (stTy->hasName() && stTy->getName() == vaargStructName) {
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
