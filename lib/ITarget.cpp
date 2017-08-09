//===------------ ITarget.cpp -- MemSafety Instrumentation ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITarget.h"

using namespace llvm;
using namespace meminstrument;

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool CheckTemporalFlag,
                 bool RequiresExplicitBounds)
    : Instrumentee(Instrumentee), Location(Location), AccessSize(AccessSize),
      CheckUpperBoundFlag(CheckUpperBoundFlag),
      CheckLowerBoundFlag(CheckLowerBoundFlag),
      CheckTemporalFlag(CheckTemporalFlag),
      RequiresExplicitBounds(RequiresExplicitBounds), BoundWitness(nullptr) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool RequiresExplicitBounds)
    : ITarget(Instrumentee, Location, AccessSize, CheckUpperBoundFlag,
              CheckLowerBoundFlag, false, RequiresExplicitBounds) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool RequiresExplicitBounds)
    : ITarget(Instrumentee, Location, AccessSize, true, true,
              RequiresExplicitBounds) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize)
    : ITarget(Instrumentee, Location, AccessSize, true, true, false) {}

void ITarget::joinFlags(const ITarget &other) {
  AccessSize = std::max(AccessSize, other.AccessSize);
  CheckUpperBoundFlag = CheckUpperBoundFlag || other.CheckUpperBoundFlag;
  CheckLowerBoundFlag = CheckLowerBoundFlag || other.CheckLowerBoundFlag;
  CheckTemporalFlag = CheckTemporalFlag || other.CheckTemporalFlag;
}

bool ITarget::hasWitness(void) const { return BoundWitness.get() == nullptr; }

llvm::raw_ostream &meminstrument::operator<<(llvm::raw_ostream &Stream,
                                             const ITarget &IT) {
  std::string LocName = IT.Location->getName().str();
  if (LocName.empty()) {
    switch (IT.Location->getOpcode()) {
    case llvm::Instruction::Store:
      LocName = "[store]";
      break;

    case llvm::Instruction::Ret:
      LocName = "[ret]";
      break;

    case llvm::Instruction::Br:
      LocName = "[br]";
      break;

    case llvm::Instruction::Switch:
      LocName = "[switch]";
      break;

    default:
      LocName = "[unknown]";
    }
  }

  Stream << "<" << IT.Instrumentee->getName() << ", ";
  auto *BB = IT.Location->getParent();
  Stream << BB->getName() << "::" << LocName << ", ";
  Stream << IT.AccessSize << "B, ";
  if (IT.CheckUpperBoundFlag) {
    Stream << "u";
  } else {
    Stream << "_";
  }
  if (IT.CheckLowerBoundFlag) {
    Stream << "l";
  } else {
    Stream << "_";
  }
  if (IT.CheckTemporalFlag) {
    Stream << "t";
  } else {
    Stream << "_";
  }
  Stream << ">";
  return Stream;
}
