//===-------- SoftBoundMechanism.cpp - Implementation of SoftBound --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// Implementation of Instrumentation Mechanism interface for SoftBound.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/SoftBoundMechanism.h"

#include "meminstrument/Config.h"
#include "meminstrument/instrumentation_mechanisms/softbound/InternalSoftBoundConfig.h"
#include "meminstrument/instrumentation_mechanisms/softbound/RunTimePrototypes.h"

using namespace llvm;
using namespace meminstrument;
using namespace softbound;

//===----------------------------------------------------------------------===//
//                   Implementation of SoftBoundMechanism
//===----------------------------------------------------------------------===//

SoftBoundMechanism::SoftBoundMechanism(GlobalConfig &config)
    : InstrumentationMechanism(config) {}

void SoftBoundMechanism::insertWitness(ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::relocCloneWitness(Witness &W, ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::insertCheck(ITarget &Target) const {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::materializeBounds(ITarget &Target) {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getExtCheckCounterFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getVerboseFailFunction() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::insertWitnessPhi(ITarget &Target) const
    -> std::shared_ptr<Witness> {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::addIncomingWitnessToPhi(
    std::shared_ptr<Witness> &Phi, std::shared_ptr<Witness> &Incoming,
    BasicBlock *InBB) const {
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::insertWitnessSelect(
    ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
    std::shared_ptr<Witness> &FalseWitness) const -> std::shared_ptr<Witness> {
  llvm_unreachable("TODO implement");
}

void SoftBoundMechanism::initialize(Module &M) {

  // Currenlty only spatial safety is implemented, don't do anything if temporal
  // memory safety is requested.
  if (!InternalSoftBoundConfig::ensureOnlySpatial()) {
    globalConfig.noteError();
    return;
  }

  // Insert the declarations for basic metadata and check functions
  insertFunDecls(M);

  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getName() const -> const char * { return "SoftBound"; }

//===-------------------------- private -----------------------------------===//

/// Insert the declarations for SoftBound metadata propagation functions
void SoftBoundMechanism::insertFunDecls(Module &M) {

  PrototypeInserter protoInserter(M);
  protoInserter.insertRunTimeProtoypes();
}
