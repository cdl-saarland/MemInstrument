//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_DUMMYMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_DUMMYMECHANISM_H

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

struct DummyWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  DummyWitness(llvm::Value *WitnessValue);

  static bool classof(const Witness *W) { return W->getKind() == WK_Dummy; }
};

class DummyMechanism : public InstrumentationMechanism {
public:
  DummyMechanism(GlobalConfig &CFG) : InstrumentationMechanism(CFG) {}

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

  virtual const char *getName(void) const override { return "Dummy"; }

private:
  llvm::Value *CreateWitnessFunction = nullptr;
  llvm::Value *CheckAccessFunction = nullptr;
  llvm::Value *GetUpperBoundFunction = nullptr;
  llvm::Value *GetLowerBoundFunction = nullptr;
  llvm::Value *FailFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  void initTypes(llvm::LLVMContext &Ctx);
};

} // namespace meminstrument

#endif
