//=== meminstrument/InstrumentationMechanism.h - MemSafety Instr -*- C++ -*-==//
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

#ifndef MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"

namespace meminstrument {

class InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) = 0;

  virtual void insertCheck(ITarget &Target) = 0;

  virtual void materializeBounds(ITarget &Target) = 0;

  virtual std::shared_ptr<Witness> insertWitnessPhi(ITarget &Target) = 0;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) = 0;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) = 0;

  virtual bool insertFunctionDefinitions(llvm::Module &M) = 0;

  virtual ~InstrumentationMechanism(void) {}
};

struct DummyWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  DummyWitness(llvm::Value *WitnessValue);

  static llvm::Type *getWitnessType(llvm::LLVMContext &Ctx);

  static bool classof(const Witness *W) {
    return W->getKind() == WK_Dummy;
  }
};

class DummyMechanism : public InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) override;

  virtual void insertCheck(ITarget &Target) override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual std::shared_ptr<Witness> insertWitnessPhi(ITarget &Target) override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) override;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) override;

  virtual bool insertFunctionDefinitions(llvm::Module &M) override;

private:
  llvm::Constant *CreateWitnessFunction = nullptr;
  llvm::Constant *CheckAccessFunction = nullptr;
  llvm::Constant *GetUpperBoundFunction = nullptr;
  llvm::Constant *GetLowerBoundFunction = nullptr;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
