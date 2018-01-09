//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_BEFOREOUTFLOWPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_BEFOREOUTFLOWPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class BeforeOutflowPolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<std::shared_ptr<ITarget>> &Dest,
                               llvm::Instruction *Loc) override;

  virtual const char *getName(void) const override { return "BeforeOutflow"; }

  BeforeOutflowPolicy(const llvm::DataLayout &DL) : DL(DL) {}

private:
  const llvm::DataLayout &DL;
};

} // namespace meminstrument

#endif
