//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_ITARGETFILTERS_H
#define MEMINSTRUMENT_PASS_ITARGETFILTERS_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

namespace meminstrument {

class ITargetFilter {
public:
  ITargetFilter(llvm::Pass *P) : ParentPass(P) {}

  virtual ~ITargetFilter(void) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const = 0;

protected:
  llvm::Pass *ParentPass;
};

class DominanceFilter : public ITargetFilter {
public:
  DominanceFilter(llvm::Pass *P) : ITargetFilter(P) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const override;
};

class AnnotationFilter : public ITargetFilter {
public:
  AnnotationFilter(llvm::Pass *P) : ITargetFilter(P) {}

  virtual void filterForFunction(llvm::Function &F,
                                 ITargetVector &Vec) const override;
};

void filterITargets(llvm::Pass *P, ITargetVector &Vec, llvm::Function &F);

}

#endif
