//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_NONEPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_NONEPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/Config.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class NonePolicy : public InstrumentationPolicy {
public:
  virtual void classifyTargets(std::vector<std::shared_ptr<ITarget>> &Dest,
                               llvm::Instruction *Loc) override;

  virtual const char *getName(void) const override { return "None"; }

  NonePolicy(GlobalConfig &cfg, const llvm::DataLayout &) : InstrumentationPolicy(cfg) {}
};

} // namespace meminstrument

#endif
