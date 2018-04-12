//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_PASS_EXTERNALCHECKSINTERFACE_H
#define MEMINSTRUMENT_PASS_EXTERNALCHECKSINTERFACE_H

#include "meminstrument/Config.h"
#include "meminstrument/pass/ITarget.h"
#include "llvm/IR/Function.h"

namespace meminstrument {

/// \brief Interface to implement when designing external checks
///
/// In a module pass, inherit from this interface to implement external checks
/// in the meminstrument pipeline. The pass has to be required by the
/// MemInstrumentPass.
///
/// The functions declared here allow to require explicit bound values for
/// memory regions from the run-time system and to use them.
///
/// These functions can be called by the meminstrument pass (when given
/// appropriate cli flags). They are then executed at the following positions in
/// the pipeline:
///
///  +-------------------+
///  | ITarget Gathering |
///  +-------------------+
///           ||
///           \/
///  +-------------------+
///  | ITarget Filtering |
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
class ExternalChecksInterface {
public:
  /// \brief Adjust the required ITargets for the external check generation
  ///
  /// New ITargets (i.e. shared pointers to them) may be added here, most
  /// likely with the RequiresExplicitBounds flag and the appropriate
  /// Check*Flag set.
  /// For removing ITargets that will become obsolete because of the placed
  /// external checks, you can either mark them here by calling their
  /// invalidate() method or by setting nosanitize metadata at the
  /// corresponding access instructions in the runOnModule method of your pass.
  virtual void updateITargetsForFunction(GlobalConfig &CFG, ITargetVector &Vec,
                                         llvm::Function &F) = 0;

  /// \brief create external checks
  ///
  /// Use this function to create the external checks based on the previously
  /// inserted ITargets. You may use their BoundWitness to get llvm Values for
  /// the bounds that are valid to use at the location of the ITarget.
  ///
  /// It might be desirable to store the necessary ITargets per function in
  /// some private data structure of the pass in the updateITargetsForFunction
  /// method to make use them here.
  virtual void materializeExternalChecksForFunction(GlobalConfig &CFG,
                                                    ITargetVector &Vec,
                                                    llvm::Function &F) = 0;

  virtual ~ExternalChecksInterface(void);
};
} // namespace meminstrument

#endif
