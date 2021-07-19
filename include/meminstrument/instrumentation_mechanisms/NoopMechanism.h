//===- meminstrument/NoopMechanism.h - Add Wide Bound Checks ----*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_NOOPMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_NOOPMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/ValueMap.h"

namespace meminstrument {

struct NoopWitness : public Witness {
  NoopWitness(void);

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  static bool classof(const Witness *W) { return W->getKind() == WK_Noop; }
};

class NoopMechanism : public InstrumentationMechanism {
public:
  NoopMechanism(GlobalConfig &CFG) : InstrumentationMechanism(CFG) {}

  virtual void insertWitnesses(ITarget &Target) const override;

  virtual WitnessPtr
  getRelocatedClone(const Witness &,
                    llvm::Instruction *location) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::Value *getFailFunction(void) const override;

  virtual WitnessPtr getWitnessPhi(llvm::PHINode *) const override;

  virtual void addIncomingWitnessToPhi(WitnessPtr &Phi, WitnessPtr &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual WitnessPtr getWitnessSelect(llvm::SelectInst *,
                                      WitnessPtr &TrueWitness,
                                      WitnessPtr &FalseWitness) const override;

  virtual void initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Noop"; }

  virtual ~NoopMechanism(void) {}

private:
  llvm::Type *SizeType = nullptr;

  llvm::Value *FailFunction = nullptr;

  llvm::Value *LowerBoundLocation = nullptr;
  llvm::Value *UpperBoundLocation = nullptr;

  llvm::Value *CheckResultLocation = nullptr;
};

} // namespace meminstrument

#endif
