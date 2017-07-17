//===--- meminstrument/ITargetProvider.h -- MemSafety Instr. --*- C++ -*---===//
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

#ifndef MEMINSTRUMENT_ITARGETPROVIDERPASS_H
#define MEMINSTRUMENT_ITARGETPROVIDERPASS_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"

namespace meminstrument {

/// TODO document
class ITargetProvider {
public:
  ITargetProvider(void);

  void initializeEmpty(void);
  void connectToProvider(ITargetProvider& Provider);

  // TODO this might be slightly cooler with an iterator
  std::vector<ITarget>& getITargetsForFunction(llvm::Function* F);

  void addITarget(const ITarget& Target);

  virtual ~ITargetProvider(void) {}

private:
  typedef llvm::ValueMap<llvm::Function*, std::vector<ITarget>> MapType;
  std::shared_ptr<MapType> TargetMap;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGETPROVIDERPASS_H
