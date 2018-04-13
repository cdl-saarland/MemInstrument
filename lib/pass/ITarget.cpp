//===---------- pass/ITarget.cpp -- MemSafety Instrumentation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITarget.h"

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
} // namespace


bool ITarget::is(Kind k) const {
  return this->getKind() == k;
}

Kind ITarget::getKind(void) const {
  return _Kind;
}

llvm::Value *ITarget::getInstrumentee(void) const {
  assert(isValid());
  return _Instrumentee;
}

llvm::Instruction *ITarget::getLocation(void) const {
  assert(isValid());
  return _Location;
}

size_t ITarget::getAccessSize(void) const {
  assert(isValid());
  assert(is(Kind::Check));
  return _AccessSize;
}

llvm::Value *ITarget::getAccessSizeVal(void) const {
  assert(isValid());
  assert(is(Kind::VarSizeCheck));
  return _AccessSizeVal;
}

bool ITarget::hasUpperBoundFlag(void) const {
  assert(isValid());
  return _CheckUpperBoundFlag;
}

bool ITarget::hasLowerBoundFlag(void) const {
  assert(isValid());
  return _CheckLowerBoundFlag;
}

bool ITarget::hasTemporalFlag(void) const {
  assert(isValid());
  return _CheckTemporalFlag;
}

bool ITarget::requiresExplicitBounds(void) const {
  assert(isValid());
  return _RequiresExplicitBounds;
}

bool ITarget::hasBoundWitness(void) const {
  assert(isValid());
  return _BoundWitness.get() != nullptr;
}

Witness &ITarget::getBoundWitness(void) {
  assert(isValid());
  assert(hasBoundWitness());
  return *_BoundWitness;
}

bool ITarget::isValid(void) const {
  return not _Invalidated;
}

void ITarget::invalidate(void) {
  _Invalidated = true;
}


bool ITarget::subsumes(const ITarget &other) const {
  assert(false); // TODO ?
  return false;
}

bool ITarget::joinFlags(const ITarget &other) {
  assert(false); // TODO ?
  return false;
}

static ITargetPtr createBoundsTarget(llvm::Value* Instrumentee, llvm::Value* Location) {
  ITarget *R = new ITarget(Kind::Bounds);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_RequiresExplicitBounds = true;
  return std::shared_ptr<ITarget>(R);
}

static ITargetPtr createInvariantTarget(llvm::Value* Instrumentee, llvm::Value* Location);

static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, size_t Size);

static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, llvm::Value *Size);

static ITargetPtr createIntermediateTarget(llvm::Value* Instrumentee, llvm::Value* Location, const ITarget &other);




ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool CheckTemporalFlag,
                 bool RequiresExplicitBounds)
    : Instrumentee(Instrumentee), Location(Location), AccessSize(AccessSize),
      HasConstAccessSize(true), AccessSizeVal(nullptr),
      CheckUpperBoundFlag(CheckUpperBoundFlag),
      CheckLowerBoundFlag(CheckLowerBoundFlag),
      CheckTemporalFlag(CheckTemporalFlag),
      RequiresExplicitBounds(RequiresExplicitBounds), BoundWitness(nullptr) {
  assert(Instrumentee);
  assert(Location);
}

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
      RequiresExplicitBounds(RequiresExplicitBounds), BoundWitness(nullptr) {
  assert(Instrumentee);
  assert(Location);
  assert(AccessSize);
}

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

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location)
    : ITarget(Instrumentee, Location, nullptr, true, true, false) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 bool RequiresExplicitBounds)
    : ITarget(Instrumentee, Location, nullptr, true, true,
              RequiresExplicitBounds) {}

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
  if (!IT.isValid()) {
    Stream << "<invalidated itarget>";
    return Stream;
  }

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
