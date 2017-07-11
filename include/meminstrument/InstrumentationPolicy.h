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
  virtual void classifyTargets(std::vector<ITarget> &Dest,
                               llvm::Instruction *Loc) = 0;

  virtual void instrumentFunction(llvm::Function &Func,
                                  std::vector<ITarget> &Targets) = 0;

  virtual ~InstrumentationPolicy() {}
};

class BeforeOutflowPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<ITarget> &Dest,
                               llvm::Instruction *Loc) override;

  virtual void instrumentFunction(llvm::Function &Func,
                                  std::vector<ITarget> &Targets) override;

  BeforeOutflowPolicy(const llvm::DataLayout &DL) : DL(DL) {}

private:
  const llvm::DataLayout &DL;
  size_t getPointerAccessSize(llvm::Value *V);

  llvm::Value *createWitness(llvm::Value *Instrumentee);
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONPOLICY_H
