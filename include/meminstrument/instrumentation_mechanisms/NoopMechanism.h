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
public:
  NoopWitness(llvm::Instruction *Location, llvm::Value *Lower, llvm::Value *Upper);

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  llvm::Instruction *getInsertionLocation(void) const;

  bool hasBoundsMaterialized(void) const;

  void setBoundsMaterialized(void);

  static bool classof(const Witness *W) { return W->getKind() == WK_Noop; }

private:
  llvm::Instruction *Location = nullptr;

  llvm::Value *Lower = nullptr;
  llvm::Value *Upper = nullptr;

  bool hasBounds = false;
};

class NoopMechanism : public InstrumentationMechanism {
public:
  NoopMechanism(GlobalConfig &CFG) : InstrumentationMechanism(CFG) {}

  virtual void insertWitness(ITarget &Target) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) const override;

  virtual llvm::Constant *getFailFunction(void) const override;

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
  void insertSleepCall(llvm::Instruction *Loc, uint32_t USecs) const;

  llvm::Type *I32Type = nullptr;

  llvm::Type *SizeType = nullptr;

  llvm::Constant *FailFunction = nullptr;

  llvm::Constant *SleepFunction = nullptr;

  llvm::Value *LowerBoundVal = nullptr;
  llvm::Value *UpperBoundVal = nullptr;

  // uint32_t gen_witness_time = 0;
  uint32_t gen_bounds_time = 0;
  uint32_t check_time = 0;
  // uint32_t alloca_time = 0;
  // uint32_t heapalloc_time = 0;
  // uint32_t heapfree_time = 0;
};

} // namespace meminstrument

#endif
