//===------ meminstrument/SplayMechanism.h - MemSafety Instr -*- C++ -*-----==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_SPLAYMECHANISM_H
#define MEMINSTRUMENT_SPLAYMECHANISM_H

#include "meminstrument/ITarget.h"
#include "meminstrument/InstrumentationMechanism.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

struct SplayWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  SplayWitness(llvm::Value *WitnessValue);

  static llvm::Type *getWitnessType(llvm::LLVMContext &Ctx);

  static bool classof(const Witness *W) { return W->getKind() == WK_Splay; }
};

class SplayMechanism : public InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessPhi(ITarget &Target) const override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const override;

  virtual bool insertGlobalDefinitions(llvm::Module &M) override;

private:
  llvm::Constant *CheckAccessFunction = nullptr;
  llvm::Constant *GetUpperBoundFunction = nullptr;
  llvm::Constant *GetLowerBoundFunction = nullptr;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_SPLAYMECHANISM_H
