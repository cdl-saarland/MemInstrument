//===----------------------------------------------------------------------===//
///
/// \file TODO doku
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

  virtual void insertWitness(ITarget &Target) const override;

  virtual void relocCloneWitness(Witness &W, ITarget &Target) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::Value *getFailFunction(void) const override;

  virtual llvm::Value *getVerboseFailFunction(void) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessPhi(ITarget &Target) const override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const override;

  virtual void initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Lowfat"; }

private:
  llvm::Value *CheckDerefFunction = nullptr;
  llvm::Value *CheckOOBFunction = nullptr;
  llvm::Value *GetUpperBoundFunction = nullptr;
  llvm::Value *GetLowerBoundFunction = nullptr;
  llvm::Value *FailFunction = nullptr;
  llvm::Value *VerboseFailFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  void initTypes(llvm::LLVMContext &Ctx);
  void insertFunctionDeclarations(llvm::Module &M);
};

} // namespace meminstrument

#endif
