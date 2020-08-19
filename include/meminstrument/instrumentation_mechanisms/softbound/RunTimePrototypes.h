//===------- meminstrument/RunTimeProtoypes.h - SoftBound -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// The prototype inserter injects run-time library prototypes into LLVM
/// modules. Calls to check functions, metadata propagation functions,
/// initialization functions etc. will be inserted during compilation, the
/// prototypes ensure that the module compiles although there is no definition
/// of the function. The definitions are in the C run-time library, which is
/// build stand-alone and linked later.
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H

#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"

#include "llvm/IR/Module.h"

namespace meminstrument {

namespace softbound {

class PrototypeInserter {

public:
  PrototypeInserter(llvm::Module &);

  void insertRunTimeProtoypes();

private:
  llvm::Module &module;

  void insertSpatialRunTimeProtoypes();

  void insertTemporalRunTimeProtoypes();

  void insertFullSafetyRunTimeProtoypes();

  void insertSetupFunctions();
};
} // namespace softbound

} // namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUND_RUNTIMEPROTOTYPES_H
