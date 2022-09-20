//===- meminstrument/SplayMechanism.h - J&K Splay Tree Approach -*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// "Splay", our short name for the memory safety instrumentation presented by
/// Jones & Kelly in "Backwards-Compatible Bounds Checking for Arrays and
/// Pointers in C Programs", is an approach that stores base and bounds of
/// pointers in a splay-tree datastructure. In-bounds checks are performed
/// whenever a pointer is dereferenced, stored to memory, handed over to a
/// function at a call site or the like. Hence, it will not only report
/// out-of-bounds pointer accesses, but also some cases of out-of-bounds pointer
/// arithmetic.
///
/// Find all details in the original paper:
/// https://dblp.org/rec/conf/aadebug/JonesK97
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

  virtual void insertWitnesses(ITarget &Target) const override;

  virtual WitnessPtr
  getRelocatedClone(const Witness &,
                    llvm::Instruction *location) const override;

  virtual void insertCheck(ITarget &Target) const override;

  virtual void materializeBounds(ITarget &Target) override;

  virtual llvm::FunctionCallee getFailFunction(void) const override;

  virtual llvm::FunctionCallee getExtCheckCounterFunction(void) const override;

  virtual llvm::FunctionCallee getVerboseFailFunction(void) const override;

  virtual WitnessPtr getWitnessPhi(llvm::PHINode *) const override;

  virtual void addIncomingWitnessToPhi(WitnessPtr &Phi, WitnessPtr &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual WitnessPtr getWitnessSelect(llvm::SelectInst *,
                                      WitnessPtr &TrueWitness,
                                      WitnessPtr &FalseWitness) const override;

  virtual void initialize(llvm::Module &M) override;

  virtual const char *getName(void) const override { return "Splay"; }

  virtual bool invariantsAreChecks() const override;

protected:
  llvm::Type *WitnessType = nullptr;
  llvm::Type *PtrArgType = nullptr;
  llvm::Type *SizeType = nullptr;

private:
  llvm::FunctionCallee GlobalAllocFunction = nullptr;
  llvm::FunctionCallee AllocFunction = nullptr;
  llvm::FunctionCallee CheckInboundsFunction = nullptr;
  llvm::FunctionCallee CheckDereferenceFunction = nullptr;
  llvm::FunctionCallee GetUpperBoundFunction = nullptr;
  llvm::FunctionCallee GetLowerBoundFunction = nullptr;
  llvm::FunctionCallee ExtCheckCounterFunction = nullptr;

  // Map mapping location/instrumentee/index tuples to materialized lower and
  // upper bounds
  std::map<const std::tuple<const llvm::Instruction *, const llvm::Value *,
                            unsigned>,
           std::pair<llvm::Value *, llvm::Value *>>
      MaterializedBounds;

  void initTypes(llvm::LLVMContext &Ctx);
  void insertFunctionDeclarations(llvm::Module &M);
  void setupGlobals(llvm::Module &M);
  void instrumentAlloca(llvm::Module &M, llvm::AllocaInst *AI);
};

} // namespace meminstrument

#endif
