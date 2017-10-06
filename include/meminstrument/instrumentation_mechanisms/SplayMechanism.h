//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SPLAYMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SPLAYMECHANISM_H

#include "meminstrument/pass/ITarget.h"
#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

struct SplayWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  SplayWitness(llvm::Value *WitnessValue);

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

  virtual bool initialize(llvm::Module &M) override;

private:
  llvm::Constant *GlobalAllocFunction = nullptr;
  llvm::Constant *AllocFunction = nullptr;
  llvm::Constant *CheckInboundsFunction = nullptr;
  llvm::Constant *CheckDereferenceFunction = nullptr;
  llvm::Constant *GetUpperBoundFunction = nullptr;
  llvm::Constant *GetLowerBoundFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  void initTypes(llvm::LLVMContext &Ctx);
  void insertFunctionDeclarations(llvm::Module &M);
  void setupGlobals(llvm::Module &M);
  void instrumentAlloca(llvm::Module &M, llvm::AllocaInst *AI);
};

}

#endif
