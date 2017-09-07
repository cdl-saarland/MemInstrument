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

namespace {
bool flagSubsumes(const ITarget &i1, const ITarget &i2) {
  return
      //  i1.HasConstAccessSize && i2.HasConstAccessSize &&
      // (i1.AccessSize >= i2.AccessSize) &&
      (i1.CheckUpperBoundFlag >= i2.CheckUpperBoundFlag) &&
      (i1.CheckLowerBoundFlag >= i2.CheckLowerBoundFlag) &&
      (i1.CheckTemporalFlag >= i2.CheckTemporalFlag) &&
      (i1.RequiresExplicitBounds >= i2.RequiresExplicitBounds);
}
}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool CheckTemporalFlag,
                 bool RequiresExplicitBounds)
    : Instrumentee(Instrumentee), Location(Location), AccessSize(AccessSize),
      HasConstAccessSize(true), AccessSizeVal(nullptr),
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

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 Value *AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool CheckTemporalFlag,
                 bool RequiresExplicitBounds)
    : Instrumentee(Instrumentee), Location(Location), AccessSize(0),
      HasConstAccessSize(false), AccessSizeVal(AccessSize),
      CheckUpperBoundFlag(CheckUpperBoundFlag),
      CheckLowerBoundFlag(CheckLowerBoundFlag),
      CheckTemporalFlag(CheckTemporalFlag),
      RequiresExplicitBounds(RequiresExplicitBounds), BoundWitness(nullptr) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 Value *AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool RequiresExplicitBounds)
    : ITarget(Instrumentee, Location, AccessSize, CheckUpperBoundFlag,
              CheckLowerBoundFlag, false, RequiresExplicitBounds) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 Value *AccessSize, bool RequiresExplicitBounds)
    : ITarget(Instrumentee, Location, AccessSize, true, true,
              RequiresExplicitBounds) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 Value *AccessSize)
    : ITarget(Instrumentee, Location, AccessSize, true, true, false) {}

bool ITarget::subsumes(const ITarget &other) const {
  return (Instrumentee == other.Instrumentee) &&
         ((HasConstAccessSize && other.HasConstAccessSize) ||
          AccessSizeVal == other.AccessSizeVal) &&
         (AccessSize >= other.AccessSize) && flagSubsumes(*this, other);
}

bool ITarget::joinFlags(const ITarget &other) {
  bool Changed = !flagSubsumes(*this, other);

  // AccessSize = std::max(AccessSize, other.AccessSize);
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
  if (IT.HasConstAccessSize) {
    Stream << IT.AccessSize << "B, ";
  } else {
    Stream << "xB, ";
  }
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
