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

bool ITarget::isCheck() const {
  return is(Kind::ConstSizeCheck) || is(Kind::VarSizeCheck);
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
  assert(is(Kind::ConstSizeCheck));
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

void ITarget::setBoundWitness(std::shared_ptr<Witness> BoundWitness) {
  _BoundWitness = BoundWitness;
}

bool ITarget::isValid(void) const {
  return not _Invalidated;
}

void ITarget::invalidate(void) {
  _Invalidated = true;
}


bool ITarget::subsumes(const ITarget &other) const {
  assert(isValid());
  assert(other.isValid());

  if (getInstrumentee() != other.getInstrumentee())
    return false;

  switch (getKind()) {
    case Kind::ConstSizeCheck:
      return (other.getKind() == Kind::ConstSizeCheck) &&
        (getAccessSize() >= other.getAccessSize());

    case Kind::VarSizeCheck:
      return (other.getKind() == Kind::VarSizeCheck) &&
        (getAccessSizeVal() == other.getAccessSizeVal());

    case Kind::Invariant:
      switch (other.getKind()) {
        case Kind::ConstSizeCheck:
        case Kind::VarSizeCheck:
        case Kind::Invariant:
          return true;
      }
  }
  return false;
}

bool ITarget::joinFlags(const ITarget &other) {
  assert(isValid());
  assert(other.isValid());
  assert(is(Kind::Intermediate))
  bool Changed = false;

#define MERGE_FLAG(X) \
  if (X != other.X) { \
    X = X || other.X \
    Changed = true; \
  }

  MERGE_FLAG(_CheckUpperBoundFlag);
  MERGE_FLAG(_CheckLowerBoundFlag);
  MERGE_FLAG(_CheckTemporalFlag);
  MERGE_FLAG(_RequiresExplicitBounds);
#undef MERGE_FLAG

  return Changed;
}

static ITargetPtr createBoundsTarget(llvm::Value* Instrumentee, llvm::Value* Location) {
  ITarget *R = new ITarget(Kind::Bounds);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_RequiresExplicitBounds = true;
  return std::shared_ptr<ITarget>(R);
}

static ITargetPtr createInvariantTarget(llvm::Value* Instrumentee, llvm::Value* Location) {
  ITarget *R = new ITarget(Kind::Invariant);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  return std::shared_ptr<ITarget>(R);
}

static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, size_t Size) {
  ITarget *R = new ITarget(Kind::ConstSizeCheck);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_AccessSize = Size;
  R->_CheckUpperBoundFlag = true;
  R->_CheckLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

static ITargetPtr createSpatialCheckTarget(llvm::Value* Instrumentee, llvm::Value* Location, llvm::Value *Size) {
  ITarget *R = new ITarget(Kind::VarSizeCheck);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_AccessSizeVal = Size;
  R->_CheckUpperBoundFlag = true;
  R->_CheckLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

static ITargetPtr createIntermediateTarget(llvm::Value* Instrumentee, llvm::Value* Location) {
  ITarget *R = new ITarget(Kind::Intermediate);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  return std::shared_ptr<ITarget>(R);
}
static ITargetPtr createIntermediateTarget(llvm::Value* Instrumentee, llvm::Value* Location, const ITarget &other) {
  ITarget *R = new ITarget(Kind::Intermediate);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_CheckUpperBoundFlag = other._CheckUpperBoundFlag;
  R->_CheckLowerBoundFlag = other._CheckLowerBoundFlag;
  R->_CheckTemporalFlag = other._CheckTemporalFlag;
  //TODO explicit bounds?
  return std::shared_ptr<ITarget>(R);
}

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

  Stream << '<';
  switch (IT._Kind) {
    case Kind::Bounds:
      Stream << "bounds"
      break;
    case Kind::ConstSizeCheck:
      Stream << "dereference check with constant size " << IT.getAccessSize();
      break;
    case Kind::VarSizeCheck:
      Stream << "dereference check with variable size " << *IT.getAccessSizeVal();
      break;
    case Kind::Intermediate:
      Stream << "intermediate target";
    case Kind::Invariant:
      Stream << "invariant check";
  }
  Stream << " for " << IT.getInstrumentee()->getName(); << " at ";
  IT.printLocation(Stream);
  Stream << '>';

  return Stream;
}
