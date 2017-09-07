//===--- meminstrument/BeforeOutflowPolicy.h - MemSafety Instr. -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_BEFOREOUTFLOWPOLICY_H
#define MEMINSTRUMENT_BEFOREOUTFLOWPOLICY_H

#include "meminstrument/InstrumentationPolicy.h"
#include "meminstrument/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class BeforeOutflowPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<std::shared_ptr<ITarget>> &Dest,
                               llvm::Instruction *Loc) override;

  BeforeOutflowPolicy(const llvm::DataLayout &DL) : DL(DL) {}

private:
  const llvm::DataLayout &DL;
  size_t getPointerAccessSize(llvm::Value *V);
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_BEFOREOUTFLOWPOLICY_H

