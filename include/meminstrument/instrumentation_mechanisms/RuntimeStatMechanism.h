//===- meminstrument/RuntimeStatMechanism.h - Statistics --------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_RUNTIMESTATMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_RUNTIMESTATMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/ValueMap.h"

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
  RuntimeStatMechanism(GlobalConfig &cfg) : InstrumentationMechanism(cfg) {}

  virtual void insertWitnesses(ITarget &Target) const override;

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

  virtual const char *getName(void) const override { return "RuntimeStat"; }

  virtual ~RuntimeStatMechanism(void) {}

  virtual bool invariantsAreChecks() const override;

private:
  struct StringMapItem {
    StringMapItem(uint64_t Idx, llvm::StringRef Str)
        : idx(Idx), str(Str.str()) {}

    StringMapItem(void) : idx(0), str("") {}

    uint64_t idx;
    std::string str;
  };

  uint64_t populateStringMap(llvm::Module &M);

  llvm::Type *SizeType;

  llvm::FunctionCallee StatIncFunction = nullptr;

  llvm::Constant *StatTableID = nullptr;

  llvm::ValueMap<llvm::Instruction *, StringMapItem> StringMap;

  const uint64_t LoadIdx = 1;
  const uint64_t StoreIdx = 2;
  const uint64_t NoSanLoadIdx = 3;
  const uint64_t NoSanStoreIdx = 4;

  const uint64_t PMDApreciseIdx = 1;
  const uint64_t PMDAsummaryIdx = 2;
  const uint64_t PMDAlocalIdx = 3;
  const uint64_t PMDAbadIdx = 4;

  bool Verbose = false;
};

} // namespace meminstrument

#endif
