//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_RUNTIMESTATMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_RUNTIMESTATMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

struct RuntimeStatWitness : public Witness {
  RuntimeStatWitness(void);

  virtual llvm::Value *getLowerBound(void) const override;

  virtual llvm::Value *getUpperBound(void) const override;

  static bool classof(const Witness *W) {
    return W->getKind() == WK_RuntimeStat;
  }
};

class RuntimeStatMechanism : public InstrumentationMechanism {
public:
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

  virtual const char *getName(void) const {
    return "RuntimeStat";
  }

private:
  llvm::Type *SizeType;
  llvm::GlobalVariable *StatArray = nullptr;

  llvm::Constant *StatIncFunction = nullptr;

  const uint64_t LoadIdx = 1;
  const uint64_t StoreIdx = 2;
  const uint64_t NoSanLoadIdx = 3;
  const uint64_t NoSanStoreIdx = 4;
};

} // namespace meminstrument

#endif
