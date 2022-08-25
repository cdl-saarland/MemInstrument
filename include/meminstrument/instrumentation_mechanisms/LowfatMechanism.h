//===- meminstrument/LowfatMechanism.h - Low-Fat Pointer --------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Low-Fat Pointers manipulate memory allocations in such a way that the bounds
/// of the allocation can be inferred from the address of the pointer.
/// Therefore, no specific datastructure is required to store the bound
/// information, and as all low-fat pointers have regular addresses, the
/// interoperability with uninstrumented libraries is high. However, the size
/// of the low-fat allocation is larger than the allocation size requested by
/// the programmer, which results in missing error reports for accesses to the
/// padding. In-bounds checks are performed whenever a pointer is dereferenced,
/// when it is stored to memory, handed over at a call site or the like.
/// Therefore, not only out-of-bounds accesses, but also some out-of-bounds
/// pointer arithmetic is reported.
///
/// Find more details on how stack, heap, and global variables are handled in
/// the original publications by Duck, Yap, and Cavallaro:
///   https://dblp.org/rec/conf/cc/DuckY16
///   https://dblp.org/rec/conf/ndss/DuckYC17
///   https://dblp.org/rec/journals/corr/abs-1804-04812
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_LOWFATMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_LOWFATMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

class GlobalConfig;

struct LowfatWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  llvm::Instruction *getInsertionLocation(void) const;

  bool hasBoundsMaterialized(void) const;

  LowfatWitness(llvm::Value *WitnessValue, llvm::Instruction *Location);

  static bool classof(const Witness *W) { return W->getKind() == WK_Lowfat; }

private:
  llvm::Instruction *Location;
};

class LowfatMechanism : public InstrumentationMechanism {
public:
  LowfatMechanism(GlobalConfig &cfg) : InstrumentationMechanism(cfg) {}

  virtual void insertWitnesses(ITarget &) const override;

  virtual WitnessPtr
  getRelocatedClone(const Witness &,
                    llvm::Instruction *location) const override;

  virtual void insertCheck(ITarget &) const override;

  virtual void materializeBounds(ITarget &) override;

  virtual llvm::FunctionCallee getFailFunction(void) const override;

  virtual llvm::FunctionCallee getVerboseFailFunction(void) const override;

  virtual WitnessPtr getWitnessPhi(llvm::PHINode *) const override;

  virtual void addIncomingWitnessToPhi(WitnessPtr &Phi, WitnessPtr &Incoming,
                                       llvm::BasicBlock *InBB) const override;
  virtual WitnessPtr getWitnessSelect(llvm::SelectInst *,
                                      WitnessPtr &TrueWitness,
                                      WitnessPtr &FalseWitness) const override;

  virtual void initialize(llvm::Module &) override;

  virtual const char *getName(void) const override { return "Lowfat"; }

  virtual bool invariantsAreChecks() const override;

private:
  llvm::FunctionCallee CheckDerefFunction = nullptr;
  llvm::FunctionCallee CheckOOBFunction = nullptr;
  llvm::FunctionCallee CalcBaseFunction = nullptr;
  llvm::FunctionCallee GetUpperBoundFunction = nullptr;
  llvm::FunctionCallee GetLowerBoundFunction = nullptr;
  llvm::FunctionCallee StackMirrorFunction = nullptr;
  llvm::FunctionCallee StackSizesFunction = nullptr;
  llvm::FunctionCallee StackOffsetFunction = nullptr;
  llvm::FunctionCallee StackMaskFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  void initTypes(llvm::LLVMContext &);
  void insertFunctionDeclarations(llvm::Module &);
  void prepareGlobals(llvm::Module &) const;
  bool globalCannotBeInstrumented(const llvm::GlobalVariable &) const;
  void instrumentGlobal(llvm::GlobalVariable &) const;
  void instrumentAlloca(llvm::AllocaInst *) const;
  void handleVariableLengthArray(llvm::AllocaInst *) const;
  void mirrorPointerAndReplaceAlloca(llvm::IRBuilder<> &,
                                     llvm::AllocaInst *oldAlloc,
                                     llvm::Instruction *newAlloc,
                                     llvm::Value *offset) const;
  auto getWitness(llvm::Value *incoming, llvm::Instruction *location) const
      -> llvm::Value *;
};

} // namespace meminstrument

#endif
