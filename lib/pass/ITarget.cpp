//===---------------------------------------------------------------------===///
///
/// \file See corresponding header for more descriptions.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITarget.h"

#include "meminstrument/Config.h"

#include "meminstrument/pass/Util.h"

#include <sstream>

using namespace llvm;
using namespace meminstrument;

size_t meminstrument::getNumITargets(
    const ITargetVector &IV,
    const std::function<bool(const ITarget &)> &Predicate) {
  size_t res = 0;
  for (const auto &IT : IV) {
    if (Predicate(*IT)) {
      ++res;
    }
  }
  return res;
}

size_t meminstrument::getNumValidITargets(const ITargetVector &IV) {
  return getNumITargets(IV, [](const ITarget &IT) { return IT.isValid(); });
}

bool meminstrument::validateITargets(const DominatorTree &dt,
                                     const ITargetVector &IV) {
  for (const auto &t : IV) {
    if (!t->isValid()) {
      continue;
    }
    if (const auto *insn = dyn_cast<Instruction>(t->getInstrumentee())) {
      if (insn == t->getLocation()) {
        return false;
      }
      if (!dt.dominates(insn, t->getLocation())) {
        return false;
      }
    }
  }
  return true;
}

bool ITarget::is(Kind k) const { return this->getKind() == k; }

ITarget::Kind ITarget::getKind(void) const { return _Kind; }

bool ITarget::isCheck() const {
  return is(Kind::ConstSizeCheck) || is(Kind::VarSizeCheck);
}

Value *ITarget::getInstrumentee(void) const {
  assert(isValid());
  return _Instrumentee;
}

Instruction *ITarget::getLocation(void) const {
  assert(isValid());
  return _Location;
}

size_t ITarget::getAccessSize(void) const {
  assert(isValid());
  assert(is(Kind::ConstSizeCheck));
  return _AccessSize;
}

Value *ITarget::getAccessSizeVal(void) const {
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

std::shared_ptr<Witness> ITarget::getBoundWitness(void) {
  assert(isValid());
  assert(hasBoundWitness());
  return _BoundWitness;
}

void ITarget::setBoundWitness(std::shared_ptr<Witness> BoundWitness) {
  _BoundWitness = BoundWitness;
}

bool ITarget::isValid(void) const { return not _Invalidated; }

void ITarget::invalidate(void) { _Invalidated = true; }

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
    default:
      return false;
    }
  default:
    return false;
  }
  return false;
}

bool ITarget::joinFlags(const ITarget &other) {
  assert(isValid());
  assert(other.isValid());
  assert(is(Kind::Intermediate));
  bool Changed = false;

#define MERGE_FLAG(X)                                                          \
  if (X != other.X) {                                                          \
    X = X || other.X;                                                          \
    Changed = true;                                                            \
  }

  MERGE_FLAG(_CheckUpperBoundFlag);
  MERGE_FLAG(_CheckLowerBoundFlag);
  MERGE_FLAG(_CheckTemporalFlag);
  MERGE_FLAG(_RequiresExplicitBounds);
#undef MERGE_FLAG

  return Changed;
}

ITargetPtr ITarget::createBoundsTarget(Value *Instrumentee,
                                       Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Bounds);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_CheckUpperBoundFlag = true;
  R->_CheckLowerBoundFlag = true;
  R->_RequiresExplicitBounds = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createInvariantTarget(Value *Instrumentee,
                                          Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Invariant);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createSpatialCheckTarget(Value *Instrumentee,
                                             Instruction *Location) {
  Module *M = Location->getModule();
  const auto &DL = M->getDataLayout();
  size_t acc_size = getPointerAccessSize(DL, Instrumentee);
  return ITarget::createSpatialCheckTarget(Instrumentee, Location, acc_size);
}

ITargetPtr ITarget::createSpatialCheckTarget(Value *Instrumentee,
                                             Instruction *Location,
                                             size_t Size) {
  ITarget *R = new ITarget(ITarget::Kind::ConstSizeCheck);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_AccessSize = Size;
  R->_CheckUpperBoundFlag = true;
  R->_CheckLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createSpatialCheckTarget(Value *Instrumentee,
                                             Instruction *Location,
                                             Value *Size) {
  ITarget *R = new ITarget(ITarget::Kind::VarSizeCheck);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  R->_AccessSizeVal = Size;
  R->_CheckUpperBoundFlag = true;
  R->_CheckLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createIntermediateTarget(Value *Instrumentee,
                                             Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Intermediate);
  R->_Instrumentee = Instrumentee;
  R->_Location = Location;
  return std::shared_ptr<ITarget>(R);
}

void ITarget::printLocation(raw_ostream &Stream) const {
  auto *L = this->getLocation();
  std::string LocName = L->getName().str();
  if (LocName.empty()) {
    switch (L->getOpcode()) {
    case Instruction::Store:
      LocName = "[store";
      break;

    case Instruction::Ret:
      LocName = "[ret";
      break;

    case Instruction::Br:
      LocName = "[br";
      break;

    case Instruction::Switch:
      LocName = "[switch";
      break;

    case Instruction::Call:
      LocName = "[anon call";
      break;

    default:
      LocName = "[unknown";
    }

    size_t idx = 0;
    for (auto &I : *L->getParent()) {
      if (&I == L) {
        break;
      }
      ++idx;
    }
    std::stringstream ss;
    ss << LocName << "," << idx << "]";
    LocName = ss.str();
  }
  auto *BB = this->getLocation()->getParent();
  Stream << BB->getName() << "::" << LocName;
}

bool ITarget::operator==(const ITarget &Other) const {
  assert(isValid() && Other.isValid());
  return _Instrumentee == Other._Instrumentee && _Location == Other._Location &&
         _Kind == Other._Kind &&
         _CheckUpperBoundFlag == Other._CheckUpperBoundFlag &&
         _CheckLowerBoundFlag == Other._CheckLowerBoundFlag &&
         _CheckTemporalFlag == Other._CheckTemporalFlag &&
         _RequiresExplicitBounds == Other._RequiresExplicitBounds;
}

raw_ostream &meminstrument::operator<<(raw_ostream &Stream, const ITarget &IT) {
  if (!IT.isValid()) {
    Stream << "<invalidated itarget>";
    return Stream;
  }

  Stream << '<';
  switch (IT._Kind) {
  case ITarget::Kind::Bounds:
    Stream << "bounds";
    break;
  case ITarget::Kind::ConstSizeCheck:
    Stream << "dereference check with constant size " << IT.getAccessSize()
           << "B";
    break;
  case ITarget::Kind::VarSizeCheck:
    Stream << "dereference check with variable size " << *IT.getAccessSizeVal();
    break;
  case ITarget::Kind::Intermediate:
    Stream << "intermediate target";
    break;
  case ITarget::Kind::Invariant:
    Stream << "invariant check";
    break;
  }
  Stream << " for " << IT.getInstrumentee()->getName() << " at ";
  IT.printLocation(Stream);
  if (IT.hasBoundWitness()) {
    Stream << " with witness";
  }
  Stream << '>';

  return Stream;
}
