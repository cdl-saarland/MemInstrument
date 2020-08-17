//===------- meminstrument/RunTimeProtoypes.h - SoftBound -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// TODO
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
