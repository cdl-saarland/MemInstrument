//===- meminstrument/OptimizationInterface.h - Optimizations ----*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Interface to implement optimizations for all supported memory safety
/// instrumentations.
///
/// For example, a check elimination optimization can implement it to filter out
/// checks that can never fail at runtime. Check optimizations that require
/// bounds information from the instrumentation can require them through this
/// interface, filter out instrumentation checks, and place their own checks.
/// Another use case is the optimization of metadata propagation of the
/// instrumentation.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_OPTIMIZATION_OPTIMIZATIONINTERFACE_H
#define MEMINSTRUMENT_OPTIMIZATION_OPTIMIZATIONINTERFACE_H

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/MemInstrumentPass.h"
#include "llvm/IR/Function.h"

namespace meminstrument {

/// \brief Interface to implement when designing optimizations
///
/// In a module pass, inherit from this interface to implement optimizations
/// in the meminstrument pipeline. The pass has to be required by the
/// MemInstrumentPass.
///
/// An optimizations that proves checks redundant can simply remove ITargets
/// in the updateITargetForFunction() method, and no instrumentation is placed
/// for the filtered target. In addition, the functions declared here allow to
/// require explicit bound values for memory regions from the run-time system
/// and to use them. This is useful to generate own checks at preferred
/// program locations. At the point where
/// materializeExternalChecksForFunction() is called, these bounds are
/// available to the optimization, and checks can be generated.
///
/// These functions can be called by the meminstrument pass (when given
/// appropriate cli flags). They are then executed at the following positions
/// in the pipeline:
///
///  +-----------------+
///  | prepareModule() |
///  +-----------------+
///           ||
///           \/
///  +-------------------+
///  | ITarget Gathering |
///  +-------------------+
///           ||
///           \/
///  +-----------------------------+
///  | updateITargetsForFunction() |
///  +-----------------------------+
///           ||
///           \/
///  +--------------------+
///  | Witness Generation |
///  +--------------------+
///           ||
///           \/
///  +----------------------------------------+
///  | materializeExternalChecksForFunction() |
///  +----------------------------------------+
///           ||
///           \/
///  +-------------------+
///  | Checks Generation |
///  +-------------------+
///
class OptimizationInterface {
public:
  /// \brief Perform preparing steps on the module
  ///
  /// A possible use case for this function is to insert new functions or
  /// clone existing ones. This interface provides a noop default
  /// implementation in case the function is not required.
  ///
  /// \returns true if the module is modified during the execution of this
  ///   function
  virtual bool prepareModule(MemInstrumentPass &, llvm::Module &);

  /// \brief Adjust the required ITargets for the optimization
  ///
  /// Mark targets invalid to communicate the information that
  /// no code should be generated for them. It is also valid to add new
  /// ITargets.
  ///
  /// Use this function if the optimization works interprocedurally and cannot
  /// optimize without knowledge on all ITargets. Otherwise, simply implement
  /// `updateITargetsForFunction`.
  virtual void
  updateITargetsForModule(MemInstrumentPass &,
                          std::map<llvm::Function *, ITargetVector> &);

  /// \brief Adjust the required ITargets for the optimization
  ///
  /// Mark targets invalid in this function to communicate the information that
  /// no code should be generated for them.
  ///
  /// New ITargets (i.e. shared pointers to them) may be added here, most likely
  /// BoundsITs that tell the instrumentation to place explicit bounds in the
  /// program for use by the optimization.
  virtual void updateITargetsForFunction(MemInstrumentPass &, ITargetVector &,
                                         llvm::Function &);

  /// \brief Create external checks
  ///
  /// Use this function to create the external checks based on the previously
  /// inserted ITargets. You may use their BoundWitness to get llvm values for
  /// the bounds that are valid to use at the location of the ITarget.
  ///
  /// It might be desirable to store the necessary ITargets per function in
  /// some private data structure of the pass in the updateITargetsForFunction
  /// method to make use of them here.
  virtual void materializeExternalChecksForFunction(MemInstrumentPass &,
                                                    ITargetVector &,
                                                    llvm::Function &);

  virtual ~OptimizationInterface() = 0;
};
} // namespace meminstrument

#endif
