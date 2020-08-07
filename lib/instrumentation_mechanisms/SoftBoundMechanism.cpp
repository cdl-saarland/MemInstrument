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

using namespace llvm;
using namespace meminstrument;

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
  llvm_unreachable("TODO implement");
}

auto SoftBoundMechanism::getName() const -> const char * { return "SoftBound"; }

//===----------------------------------------------------------------------===//
//                   Implementation of SoftBoundWitness
//                 (TODO: different file would be nicer)
//===----------------------------------------------------------------------===//

auto SoftBoundWitness::getLowerBound() const -> Value * {
  llvm_unreachable("TODO implement");
}

auto SoftBoundWitness::getUpperBound() const -> Value * {
  llvm_unreachable("TODO implement");
}

bool SoftBoundWitness::classof(const Witness *W) {
  return W->getKind() == WK_SoftBound;
}
