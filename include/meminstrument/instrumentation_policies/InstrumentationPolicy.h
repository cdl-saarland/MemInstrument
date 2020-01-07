//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H
#define MEMINSTRUMENT_INSTRUMENTATION_POLICIES_INSTRUMENTATIONPOLICY_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

class GlobalConfig;

/// Subclasses of this abstract class describe where memory safety checks
/// should be placed.
/// This class is not concerned with how these checks are implemented, refer to
/// InstrumentationMechanism for this issue.
class InstrumentationPolicy {
public:
  /// Add the ITargets necessary at instruction Loc to ensure memory safety to
  /// Dest.
  virtual void classifyTargets(std::vector<std::shared_ptr<ITarget>> &Dest,
                               llvm::Instruction *Loc) = 0;

  /// Returns the name of the instrumentation policy for printing and easy
  /// recognition.
  virtual const char *getName(void) const = 0;

  virtual ~InstrumentationPolicy(void) {}

protected:
  GlobalConfig &_CFG;

  InstrumentationPolicy(GlobalConfig &cfg);

  /// Helper function to check whether Ptr is a Value with type pointer
  /// pointing to some type with a size. If this is not the case, an error is
  /// noted and false is returned.
  bool validateSize(llvm::Value *Ptr);
};

} // namespace meminstrument

#endif
