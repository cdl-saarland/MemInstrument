//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SPLAYMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SPLAYMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

#include <map>

namespace meminstrument {

class GlobalConfig;

struct SplayWitness : public Witness {
  llvm::Value *WitnessValue;

  llvm::Value *UpperBound = nullptr;
  llvm::Value *LowerBound = nullptr;

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  llvm::Instruction *getInsertionLocation(void) const;

  bool hasBoundsMaterialized(void) const;

  SplayWitness(llvm::Value *WitnessValue, llvm::Instruction *Location);

  static bool classof(const Witness *W) { return W->getKind() == WK_Splay; }

private:
  llvm::Instruction *Location;
};

class SplayMechanism : public InstrumentationMechanism {
public:
  SplayMechanism(GlobalConfig &cfg) : InstrumentationMechanism(cfg) {}

  virtual void insertWitness(ITarget &Target) const override;

  virtual void relocCloneWitness(Witness &W, ITarget &Target) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::Value *getFailFunction(void) const override;

  virtual llvm::Value *getExtCheckCounterFunction(void) const override;

  virtual llvm::Value *getVerboseFailFunction(void) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessPhi(ITarget &Target) const override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const override;

  virtual bool initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Splay"; }

private:
  llvm::Value *GlobalAllocFunction = nullptr;
  llvm::Value *AllocFunction = nullptr;
  llvm::Value *CheckInboundsFunction = nullptr;
  llvm::Value *CheckDereferenceFunction = nullptr;
  llvm::Value *GetUpperBoundFunction = nullptr;
  llvm::Value *GetLowerBoundFunction = nullptr;
  llvm::Value *FailFunction = nullptr;
  llvm::Value *ExtCheckCounterFunction = nullptr;
  llvm::Value *VerboseFailFunction = nullptr;
  llvm::Value *WarningFunction = nullptr;

  llvm::Value *ConfigFunction = nullptr;

  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

  // Map mapping location/instrumentee tuples to materialized lower and upper bounds
  std::map<const std::pair<const llvm::Instruction*, const llvm::Value*>, std::pair<llvm::Value*, llvm::Value *>> MaterializedBounds;

  void initTypes(llvm::LLVMContext &Ctx);
  void insertFunctionDeclarations(llvm::Module &M);
  void setupInitCall(llvm::Module &M);
  void setupGlobals(llvm::Module &M);
  void instrumentAlloca(llvm::Module &M, llvm::AllocaInst *AI);
};

} // namespace meminstrument

#endif
