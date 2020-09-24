//===- meminstrument/PointerBoundsPolicy.h --- TODO -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// A policy for a pointer-based (i.e. not object-based) memory safety
/// instrumentation.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_POINTERBOUNDSPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_POINTERBOUNDSPOLICY_H

#include "meminstrument/instrumentation_policies/InstrumentationPolicy.h"

namespace meminstrument {

class PointerBoundsPolicy : public InstrumentationPolicy {

public:
  PointerBoundsPolicy(GlobalConfig &);

  auto getName() const -> const char * override;

  void classifyTargets(std::vector<std::shared_ptr<ITarget>> &,
                       llvm::Instruction *) override;

private:
  void addCallTargets(std::vector<std::shared_ptr<ITarget>> &,
                      llvm::CallInst *);
};

} // namespace meminstrument

#endif
