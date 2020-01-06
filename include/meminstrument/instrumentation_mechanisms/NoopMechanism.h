//===----------------------------------------------------------------------===//
///
/// \file TODO doku
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

  virtual void insertWitness(ITarget &Target) const override;

  virtual void relocCloneWitness(Witness &W, ITarget &Target) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::Value *getFailFunction(void) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessPhi(ITarget &Target) const override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const override;

  virtual bool initialize(llvm::Module &M) override;

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
