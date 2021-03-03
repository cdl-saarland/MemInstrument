//===---------------------------------------------------------------------===///
///
/// \file See corresponding header for more descriptions.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITarget.h"

#include "meminstrument/Config.h"

#include "meminstrument/pass/Util.h"

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
    if (!t->isValid() || !t->hasInstrumentee()) {
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

ITarget::Kind ITarget::getKind(void) const { return kind; }

bool ITarget::isCheck() const {
  return is(Kind::ConstSizeCheck) || is(Kind::VarSizeCheck) ||
         is(Kind::CallCheck);
}

bool ITarget::isInvariant() const {
  return is(Kind::Invariant) || is(Kind::CallInvariant);
}

Value *ITarget::getInstrumentee(void) const {
  assert(isValid());
  assert(hasInstrumentee());
  return instrumentee;
}

Instruction *ITarget::getLocation(void) const {
  assert(isValid());
  return location;
}

size_t ITarget::getAccessSize(void) const {
  assert(isValid());
  assert(is(Kind::ConstSizeCheck));
  return accessSize;
}

Value *ITarget::getAccessSizeVal(void) const {
  assert(isValid());
  assert(is(Kind::VarSizeCheck));
  return accessSizeVal;
}

bool ITarget::hasInstrumentee(void) const {
  assert(isValid());
  return instrumentee;
}

bool ITarget::hasUpperBoundFlag(void) const {
  assert(isValid());
  return checkUpperBoundFlag;
}

bool ITarget::hasLowerBoundFlag(void) const {
  assert(isValid());
  return checkLowerBoundFlag;
}

bool ITarget::hasTemporalFlag(void) const {
  assert(isValid());
  return checkTemporalFlag;
}

bool ITarget::requiresExplicitBounds(void) const {
  assert(isValid());
  return explicitBoundsFlag;
}

bool ITarget::hasBoundWitness(void) const {
  assert(isValid());
  return boundWitness.get() != nullptr;
}

bool ITarget::needsNoBoundWitness(void) const {
  return is(ITarget::Kind::CallInvariant);
}

bool ITarget::hasWitnessIfNeeded(void) const {
  return needsNoBoundWitness() || hasBoundWitness();
}

std::shared_ptr<Witness> ITarget::getBoundWitness(void) const {
  assert(isValid());
  assert(hasBoundWitness());
  return boundWitness;
}

void ITarget::setBoundWitness(std::shared_ptr<Witness> BoundWitness) {
  boundWitness = BoundWitness;
}

bool ITarget::isValid(void) const { return not invalidated; }

void ITarget::invalidate(void) { invalidated = true; }

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

  MERGE_FLAG(checkUpperBoundFlag);
  MERGE_FLAG(checkLowerBoundFlag);
  MERGE_FLAG(checkTemporalFlag);
  MERGE_FLAG(explicitBoundsFlag);
#undef MERGE_FLAG

  return Changed;
}

ITargetPtr ITarget::createBoundsTarget(Value *Instrumentee,
                                       Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Bounds);
  R->instrumentee = Instrumentee;
  R->location = Location;
  R->checkUpperBoundFlag = true;
  R->checkLowerBoundFlag = true;
  R->explicitBoundsFlag = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createInvariantTarget(Value *Instrumentee,
                                          Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Invariant);
  R->instrumentee = Instrumentee;
  R->location = Location;
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
  R->instrumentee = Instrumentee;
  R->location = Location;
  R->accessSize = Size;
  R->checkUpperBoundFlag = true;
  R->checkLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createSpatialCheckTarget(Value *Instrumentee,
                                             Instruction *Location,
                                             Value *Size) {
  ITarget *R = new ITarget(ITarget::Kind::VarSizeCheck);
  R->instrumentee = Instrumentee;
  R->location = Location;
  R->accessSizeVal = Size;
  R->checkUpperBoundFlag = true;
  R->checkLowerBoundFlag = true;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createIntermediateTarget(Value *Instrumentee,
                                             Instruction *Location) {
  ITarget *R = new ITarget(ITarget::Kind::Intermediate);
  R->instrumentee = Instrumentee;
  R->location = Location;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createCallCheckTarget(Value *Instrumentee, CallInst *Call) {
  ITarget *R = new ITarget(ITarget::Kind::CallCheck);
  R->instrumentee = Instrumentee;
  R->location = Call;
  R->checkUpperBoundFlag = false;
  R->checkLowerBoundFlag = false;
  R->checkTemporalFlag = false;
  R->explicitBoundsFlag = false;
  return std::shared_ptr<ITarget>(R);
}

ITargetPtr ITarget::createCallInvariantTarget(CallInst *Call) {
  ITarget *R = new ITarget(ITarget::Kind::CallInvariant);
  R->instrumentee = nullptr;
  R->location = Call;
  R->checkUpperBoundFlag = false;
  R->checkLowerBoundFlag = false;
  R->checkTemporalFlag = false;
  R->explicitBoundsFlag = false;
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
    raw_string_ostream rso(LocName);
    rso << "," << idx << "]";
    LocName = rso.str();
  }
  auto *BB = this->getLocation()->getParent();
  Stream << BB->getName() << "::" << LocName;
}

bool ITarget::operator==(const ITarget &Other) const {
  assert(isValid() && Other.isValid());
  return instrumentee == Other.instrumentee && location == Other.location &&
         kind == Other.kind &&
         checkUpperBoundFlag == Other.checkUpperBoundFlag &&
         checkLowerBoundFlag == Other.checkLowerBoundFlag &&
         checkTemporalFlag == Other.checkTemporalFlag &&
         explicitBoundsFlag == Other.explicitBoundsFlag;
}

raw_ostream &meminstrument::operator<<(raw_ostream &Stream, const ITarget &IT) {
  if (!IT.isValid()) {
    Stream << "<invalidated itarget>";
    return Stream;
  }

  Stream << '<';
  switch (IT.kind) {
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
  case ITarget::Kind::CallCheck:
    Stream << "call check";
    break;
  case ITarget::Kind::CallInvariant:
    Stream << "call invariant";
  }

  if (IT.hasInstrumentee() && IT.getInstrumentee()->hasName()) {
    Stream << " for " << IT.getInstrumentee()->getName();
  }
  Stream << " at ";
  IT.printLocation(Stream);
  if (IT.hasBoundWitness()) {
    Stream << " with witness";
  }
  Stream << '>';

  return Stream;
}
