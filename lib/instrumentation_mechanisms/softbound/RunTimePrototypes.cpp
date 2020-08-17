//===--------------------- RunTimePrototypes.cpp - TODO -------------------===//
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

#include "meminstrument/instrumentation_mechanisms/softbound/RunTimePrototypes.h"

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

//===----------------------------------------------------------------------===//
//                   Implementation of PrototypeInserter
//===----------------------------------------------------------------------===//

PrototypeInserter::PrototypeInserter(Module &module) : module(module) {}

void PrototypeInserter::insertRunTimeProtoypes() {

  if (InternalSoftBoundConfig::ensureOnlySpatial()) {
    insertSpatialRunTimeProtoypes();
  }

  if (InternalSoftBoundConfig::ensureOnlyTemporal()) {
    insertTemporalRunTimeProtoypes();
  }

  if (InternalSoftBoundConfig::ensureFullSafety()) {
    // This might need to call the other two in addition
    insertFullSafetyRunTimeProtoypes();
  }

  insertSetupFunctions();
}

//===---------------------------- private -------------------------------===//

void PrototypeInserter::insertSpatialRunTimeProtoypes() {
  llvm_unreachable("TODO implement");
}

void PrototypeInserter::insertTemporalRunTimeProtoypes() {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertFullSafetyRunTimeProtoypes() {
  llvm_unreachable("Temporal Safety is not yet implemented");
}

void PrototypeInserter::insertSetupFunctions() {
  llvm_unreachable("TODO implement");
}
