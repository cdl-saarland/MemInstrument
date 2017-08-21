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

bool ITarget::joinFlags(const ITarget &other) {
  bool Changed = AccessSize < other.AccessSize ||
                 CheckUpperBoundFlag < other.CheckUpperBoundFlag ||
                 CheckLowerBoundFlag < other.CheckLowerBoundFlag ||
                 CheckTemporalFlag < other.CheckTemporalFlag ||
                 RequiresExplicitBounds < other.RequiresExplicitBounds;

  AccessSize = std::max(AccessSize, other.AccessSize);
  CheckUpperBoundFlag = CheckUpperBoundFlag || other.CheckUpperBoundFlag;
  CheckLowerBoundFlag = CheckLowerBoundFlag || other.CheckLowerBoundFlag;
  CheckTemporalFlag = CheckTemporalFlag || other.CheckTemporalFlag;
  RequiresExplicitBounds =
      RequiresExplicitBounds || other.RequiresExplicitBounds;
  return Changed;
}

bool ITarget::hasWitness(void) const { return BoundWitness.get() != nullptr; }

void ITarget::printLocation(llvm::raw_ostream &Stream) const {
  std::string LocName = this->Location->getName().str();
  if (LocName.empty()) {
    switch (this->Location->getOpcode()) {
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

    case llvm::Instruction::Call:
      LocName = "[anon call]";
      break;

    default:
      LocName = "[unknown]";
    }
  }
  auto *BB = this->Location->getParent();
  Stream << BB->getName() << "::" << LocName;
}

llvm::raw_ostream &meminstrument::operator<<(llvm::raw_ostream &Stream,
                                             const ITarget &IT) {
  Stream << "<" << IT.Instrumentee->getName() << ", ";
  IT.printLocation(Stream);
  Stream << ", ";
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
