//===- meminstrument/LowfatMechanism.h - Low-Fat Pointer --------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_LOWFATMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_LOWFATMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
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

  virtual void insertWitnesses(ITarget &Target) const override;

  virtual WitnessPtr
  getRelocatedClone(const Witness &,
                    llvm::Instruction *location) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::FunctionCallee getFailFunction(void) const override;

  virtual llvm::FunctionCallee getVerboseFailFunction(void) const override;

  virtual WitnessPtr getWitnessPhi(llvm::PHINode *) const override;

  virtual void addIncomingWitnessToPhi(WitnessPtr &Phi, WitnessPtr &Incoming,
                                       llvm::BasicBlock *InBB) const override;
  virtual WitnessPtr getWitnessSelect(llvm::SelectInst *,
                                      WitnessPtr &TrueWitness,
                                      WitnessPtr &FalseWitness) const override;

  virtual void initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Lowfat"; }

  virtual bool invariantsAreChecks() const override;

private:
  llvm::FunctionCallee CheckDerefFunction = nullptr;
  llvm::FunctionCallee CheckOOBFunction = nullptr;
  llvm::FunctionCallee GetUpperBoundFunction = nullptr;
  llvm::FunctionCallee GetLowerBoundFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  void initTypes(llvm::LLVMContext &Ctx);
  void insertFunctionDeclarations(llvm::Module &M);
};

} // namespace meminstrument

#endif
