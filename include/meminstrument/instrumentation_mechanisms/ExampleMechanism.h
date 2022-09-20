//===- meminstrument/ExampleMechanism.h - Example Mechanism -----*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This mechanism does not implement a real safety instrumentation. It
/// demonstrates the minimal usage of the InstrumentationMechanism interface.
///
/// It simulates possible costs of inserting checks at memory dereferences. The
/// checks it places for this purpose are volatile loads of (fake) base and
/// bound values from global variables, and a comparison of the dereferenced
/// pointer to these bounds.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_EXAMPLEMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_EXAMPLEMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"

namespace meminstrument {

struct ExampleWitness : public Witness {
  ExampleWitness(void);

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  static bool classof(const Witness *W) { return W->getKind() == WK_Example; }
};

class ExampleMechanism : public InstrumentationMechanism {
public:
  ExampleMechanism(GlobalConfig &cfg) : InstrumentationMechanism(cfg) {}

  virtual void insertWitnesses(ITarget &) const override;

  virtual WitnessPtr
  getRelocatedClone(const Witness &,
                    llvm::Instruction *location) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::FunctionCallee getFailFunction(void) const override;

  virtual WitnessPtr getWitnessPhi(llvm::PHINode *) const override;

  virtual void addIncomingWitnessToPhi(WitnessPtr &Phi, WitnessPtr &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual WitnessPtr getWitnessSelect(llvm::SelectInst *,
                                      WitnessPtr &TrueWitness,
                                      WitnessPtr &FalseWitness) const override;

  virtual void initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Example"; }

  virtual ~ExampleMechanism(void) {}

  virtual bool invariantsAreChecks() const override;

private:
  llvm::Type *SizeType = nullptr;

  llvm::Value *LowerBoundLocation = nullptr;
  llvm::Value *UpperBoundLocation = nullptr;

  llvm::Value *CheckResultLocation = nullptr;
};

} // namespace meminstrument

#endif
