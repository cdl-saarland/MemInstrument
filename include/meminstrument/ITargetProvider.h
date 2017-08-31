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
  virtual ~ITargetProvider(void) {}

protected:
  void initializeEmpty(void);
  void connectToProvider(ITargetProvider *Provider);

  std::vector<std::shared_ptr<ITarget>> &
  getITargetsForFunction(llvm::Function *F);

private:
  typedef llvm::ValueMap<llvm::Function *,
                         std::vector<std::shared_ptr<ITarget>>>
      MapType;
  std::shared_ptr<MapType> TargetMap;
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGETPROVIDERPASS_H
