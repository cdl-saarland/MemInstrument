//===--- meminstrument/InstrumentationPolicy.h -- TODO doku ----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATIONPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATIONPOLICY_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<ITarget> &dest,
                              llvm::Instruction *loc) = 0;

  virtual void instrumentFunction(llvm::Function *func,
                                  std::vector<ITarget> &targets) = 0;

  virtual ~InstrumentationPolicy() {}
};

class BeforeOutflowPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<ITarget> &dest,
                              llvm::Instruction *loc) override;

  virtual void instrumentFunction(llvm::Function *func,
                                  std::vector<ITarget> &targets) override;

  BeforeOutflowPolicy(const llvm::DataLayout &dl) : datalayout(dl) {}

private:
  const llvm::DataLayout &datalayout;
  size_t getPointerAccessSize(llvm::Value *v);

  llvm::Value *createWitness(llvm::Value *instrumentee);
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONPOLICY_H
