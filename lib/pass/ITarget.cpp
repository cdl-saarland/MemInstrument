//===- ITargets.cpp - Instrumentation Targets -----------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Dominators.h"
#include "llvm/Support/raw_ostream.h"

#include "meminstrument/pass/Util.h"

using namespace llvm;
using namespace meminstrument;

//===----------------------------------------------------------------------===//
//                 Implementation of ITargetBuilder
//===----------------------------------------------------------------------===//

ITargetPtr ITargetBuilder::createCallCheckTarget(Value *instrumentee,
                                                 CallBase *call) {
  return std::make_shared<CallCheckIT>(instrumentee, call);
}

ITargetPtr ITargetBuilder::createSpatialCheckTarget(Value *Instrumentee,
                                                    Instruction *Location,
                                                    bool checkUpper,
                                                    bool checkLower) {
  Module *M = Location->getModule();
  const auto &DL = M->getDataLayout();
  size_t acc_size = getPointerAccessSize(DL, Instrumentee);
  return ITargetBuilder::createSpatialCheckTarget(
      Instrumentee, Location, acc_size, checkUpper, checkLower);
}

ITargetPtr ITargetBuilder::createSpatialCheckTarget(Value *instrumentee,
                                                    Instruction *location,
                                                    size_t size,
                                                    bool checkUpper,
                                                    bool checkLower) {
  return std::make_shared<ConstSizeCheckIT>(instrumentee, location, size,
                                            checkUpper, checkLower);
}

ITargetPtr ITargetBuilder::createSpatialCheckTarget(Value *instrumentee,
                                                    Instruction *location,
                                                    Value *size,
                                                    bool checkUpper,
                                                    bool checkLower) {
  return std::make_shared<VarSizeCheckIT>(instrumentee, location, size,
                                          checkUpper, checkLower);
}

ITargetPtr ITargetBuilder::createArgInvariantTarget(Value *instrumentee,
                                                    CallBase *call,
                                                    unsigned argNum) {
  return std::make_shared<ArgInvariantIT>(instrumentee, call, argNum);
}

ITargetPtr ITargetBuilder::createCallInvariantTarget(CallBase *call) {
  return std::make_shared<CallInvariantIT>(call);
}

ITargetPtr ITargetBuilder::createCallInvariantTarget(
    CallBase *call, const std::map<unsigned, Value *> &requiredArgs) {
  return std::make_shared<CallInvariantIT>(call, requiredArgs);
}

ITargetPtr ITargetBuilder::createValInvariantTarget(Value *instrumentee,
                                                    Instruction *location) {
  return std::make_shared<ValInvariantIT>(instrumentee, location);
}

ITargetPtr ITargetBuilder::createBoundsTarget(Value *instrumentee,
                                              Instruction *location,
                                              bool checkUpper,
                                              bool checkLower) {
  return std::make_shared<BoundsIT>(instrumentee, location, checkUpper,
                                    checkLower);
}

ITargetPtr ITargetBuilder::createIntermediateTarget(Value *instrumentee,
                                                    Instruction *location) {
  return std::make_shared<IntermediateIT>(instrumentee, location);
}

size_t ITargetBuilder::getNumITargets(
    const ITargetVector &vec,
    const std::function<bool(const ITarget &)> &predicate) {
  size_t res = 0;
  for (const auto &target : vec) {
    if (predicate(*target)) {
      ++res;
    }
  }
  return res;
}

size_t ITargetBuilder::getNumValidITargets(const ITargetVector &vec) {
  return getNumITargets(vec,
                        [](const ITarget &target) { return target.isValid(); });
}

bool ITargetBuilder::validateITargets(const DominatorTree &dt,
                                      const ITargetVector &vec) {
  for (const auto &t : vec) {
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

//===----------------------------------------------------------------------===//
//                 Implementation of ITarget
//===----------------------------------------------------------------------===//

ITarget::ITargetKind ITarget::getKind() const { return kind; }

bool ITarget::isCheck() const { return isa<CheckIT>(this); }

bool ITarget::isInvariant() const { return isa<InvariantIT>(this); }

bool ITarget::hasInstrumentee() const {
  assert(isValid());
  return instrumentee;
}

Value *ITarget::getInstrumentee() const {
  assert(isValid());
  assert(hasInstrumentee());
  return instrumentee;
}

Instruction *ITarget::getLocation() const {
  assert(isValid());
  return location;
}

bool ITarget::hasUpperBoundFlag() const {
  assert(isValid());
  return checkUpperBoundFlag;
}

bool ITarget::hasLowerBoundFlag() const {
  assert(isValid());
  return checkLowerBoundFlag;
}

bool ITarget::hasTemporalFlag() const {
  assert(isValid());
  return checkTemporalFlag;
}

bool ITarget::requiresExplicitBounds() const {
  assert(isValid());
  return false;
}

bool ITarget::hasBoundWitnesses() const {
  assert(isValid());
  return !boundWitnesses.empty();
}

bool ITarget::needsNoBoundWitnesses() const { return false; }

bool ITarget::hasWitnessesIfNeeded() const {
  return needsNoBoundWitnesses() || hasBoundWitnesses();
}

WitnessPtr ITarget::getSingleBoundWitness() const {
  assert(isValid());
  assert(boundWitnesses.size() == 1);
  return boundWitnesses.at(0);
}

WitnessPtr ITarget::getBoundWitness(unsigned index) const {
  assert(isValid());
  return boundWitnesses.at(index);
}

WitnessMap ITarget::getBoundWitnesses() const {
  assert(isValid());
  return boundWitnesses;
}

void ITarget::setSingleBoundWitness(WitnessPtr boundWitness) {
  assert(boundWitnesses.size() <= 1);
  boundWitnesses[0] = boundWitness;
}

void ITarget::setBoundWitness(WitnessPtr boundWitness, unsigned index) {
  boundWitnesses[index] = boundWitness;
}

void ITarget::setBoundWitnesses(const WitnessMap &bWitnesses) {
  boundWitnesses = bWitnesses;
}

bool ITarget::isValid() const { return not invalidated; }

void ITarget::invalidate() { invalidated = true; }

ITarget::ITarget(ITargetKind k, Value *instrumentee, Instruction *location,
                 bool checkUpper, bool checkLower, bool checkTemporal)
    : kind(k), instrumentee(instrumentee), location(location),
      checkUpperBoundFlag(checkUpper), checkLowerBoundFlag(checkLower),
      checkTemporalFlag(checkTemporal), invalidated(false), boundWitnesses({}) {
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

raw_ostream &meminstrument::operator<<(raw_ostream &Stream, const ITarget &IT) {
  if (!IT.isValid()) {
    Stream << "<invalidated itarget>";
    return Stream;
  }

  Stream << '<';
  IT.dump(Stream);

  if (IT.hasInstrumentee() && IT.getInstrumentee()->hasName()) {
    Stream << " for " << IT.getInstrumentee()->getName();
  }
  Stream << " at ";
  IT.printLocation(Stream);
  if (IT.hasBoundWitnesses()) {
    Stream << " with witness";
  }
  Stream << '>';

  return Stream;
}

bool ITarget::equals(const ITarget &Other) const {
  assert(isValid() && Other.isValid());
  return kind == Other.kind && instrumentee == Other.instrumentee &&
         location == Other.location &&
         checkUpperBoundFlag == Other.checkUpperBoundFlag &&
         checkLowerBoundFlag == Other.checkLowerBoundFlag &&
         checkTemporalFlag == Other.checkTemporalFlag;
}

//===----------------------------------------------------------------------===//
//                 Implementation of CheckIT
//===----------------------------------------------------------------------===//

CheckIT::CheckIT(ITargetKind kind, Value *instrumentee, Instruction *location,
                 bool checkUpper, bool checkLower, bool checkTemporal)
    : ITarget(kind, instrumentee, location, checkUpper, checkLower,
              checkTemporal) {
  assert(kind >= ITargetKind::ITK_CallCheck &&
         kind <= ITargetKind::ITK_VarSizeCheck);
}

bool CheckIT::classof(const ITarget *T) {
  return T->getKind() >= ITargetKind::ITK_CallCheck &&
         T->getKind() <= ITargetKind::ITK_VarSizeCheck;
}

//===----------------------------------------------------------------------===//
//                 Implementation of CallCheckIT
//===----------------------------------------------------------------------===//

CallCheckIT::CallCheckIT(Value *instrumentee, CallBase *call)
    : CheckIT(ITargetKind::ITK_CallCheck, instrumentee, call,
              /*checkUpper*/ true, /*checkLower*/ true,
              /*checkTemporal*/ false) {}

CallBase *CallCheckIT::getCall() const {
  assert(isValid());
  return cast<CallBase>(location);
}

void CallCheckIT::dump(raw_ostream &os) const { os << "call check"; }

bool CallCheckIT::operator==(const ITarget &other) const {
  return equals(other);
}

bool CallCheckIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_CallCheck;
}

//===----------------------------------------------------------------------===//
//                 Implementation of ConstSizeCheckIT
//===----------------------------------------------------------------------===//

ConstSizeCheckIT::ConstSizeCheckIT(Value *instrumentee, Instruction *location,
                                   size_t accessSize, bool checkUpper,
                                   bool checkLower)
    : CheckIT(ITargetKind::ITK_ConstSizeCheck, instrumentee, location,
              checkUpper, checkLower, /*checkTemporal*/ false),
      accessSize(accessSize) {
  assert(checkUpper || checkLower);
}

size_t ConstSizeCheckIT::getAccessSize() const {
  assert(isValid());
  return accessSize;
}

void ConstSizeCheckIT::dump(raw_ostream &os) const {
  os << "dereference check with constant size " << getAccessSize() << "B";
}

bool ConstSizeCheckIT::operator==(const ITarget &other) const {
  return equals(other) &&
         accessSize == cast<ConstSizeCheckIT>(other).getAccessSize();
}

bool ConstSizeCheckIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_ConstSizeCheck;
}

//===----------------------------------------------------------------------===//
//                 Implementation of VarSizeCheckIT
//===----------------------------------------------------------------------===//

VarSizeCheckIT::VarSizeCheckIT(Value *instrumentee, Instruction *location,
                               Value *accessSizeVal, bool checkUpper,
                               bool checkLower)
    : CheckIT(ITargetKind::ITK_VarSizeCheck, instrumentee, location, checkUpper,
              checkLower, /*checkTemporal*/ false),
      accessSizeVal(accessSizeVal) {
  assert(checkUpper || checkLower);
}

Value *VarSizeCheckIT::getAccessSizeVal() const {
  assert(isValid());
  return accessSizeVal;
}

void VarSizeCheckIT::dump(raw_ostream &os) const {
  os << "dereference check with variable size " << *getAccessSizeVal();
}

bool VarSizeCheckIT::operator==(const ITarget &other) const {
  return equals(other) &&
         accessSizeVal == cast<VarSizeCheckIT>(other).getAccessSizeVal();
}

bool VarSizeCheckIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_VarSizeCheck;
}

//===----------------------------------------------------------------------===//
//                 Implementation of InvariantIT
//===----------------------------------------------------------------------===//

InvariantIT::InvariantIT(ITargetKind kind, Value *instrumentee,
                         Instruction *location)
    : ITarget(kind, instrumentee, location, /*checkUpper*/ false,
              /*checkLower*/ false,
              /*checkTemporal*/ false) {
  assert(kind >= ITargetKind::ITK_ArgInvariant &&
         kind <= ITargetKind::ITK_ValInvariant);
}

bool InvariantIT::classof(const ITarget *T) {
  return T->getKind() >= ITargetKind::ITK_ArgInvariant &&
         T->getKind() <= ITargetKind::ITK_ValInvariant;
}

//===----------------------------------------------------------------------===//
//                 Implementation of ArgInvariantIT
//===----------------------------------------------------------------------===//

ArgInvariantIT::ArgInvariantIT(Value *instrumentee, CallBase *call,
                               unsigned argNum)
    : InvariantIT(ITargetKind::ITK_ArgInvariant, instrumentee, call),
      argNum(argNum) {}

unsigned ArgInvariantIT::getArgNum() const {
  assert(isValid());
  return argNum;
}

CallBase *ArgInvariantIT::getCall() const {
  assert(isValid());
  return cast<CallBase>(location);
}

void ArgInvariantIT::dump(raw_ostream &os) const {
  os << "argument " << argNum << " invariant";
}

bool ArgInvariantIT::operator==(const ITarget &other) const {
  return equals(other) && argNum == cast<ArgInvariantIT>(other).getArgNum();
}

bool ArgInvariantIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_ArgInvariant;
}

//===----------------------------------------------------------------------===//
//                 Implementation of CallInvariantIT
//===----------------------------------------------------------------------===//

CallInvariantIT::CallInvariantIT(CallBase *call)
    : InvariantIT(ITargetKind::ITK_CallInvariant, nullptr, call) {}

CallInvariantIT::CallInvariantIT(
    CallBase *call, const std::map<unsigned, Value *> &requiredArgs)
    : InvariantIT(ITargetKind::ITK_CallInvariant, nullptr, call),
      requiredArgs(requiredArgs) {}

bool CallInvariantIT::needsNoBoundWitnesses() const {
  return requiredArgs.empty();
}

CallBase *CallInvariantIT::getCall() const {
  assert(isValid());
  return cast<CallBase>(location);
}

std::map<unsigned, Value *> CallInvariantIT::getRequiredArgs() const {
  return requiredArgs;
}

void CallInvariantIT::dump(raw_ostream &os) const {
  os << "call invariant";
  const auto *calledFun = getCall()->getCalledFunction();
  if (calledFun && calledFun->hasName()) {
    os << " for " << calledFun->getName();
  }
  if (!requiredArgs.empty()) {
    os << " with " << requiredArgs.size() << " arg(s)";
  }
}

bool CallInvariantIT::operator==(const ITarget &other) const {
  return equals(other);
}

bool CallInvariantIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_CallInvariant;
}

//===----------------------------------------------------------------------===//
//                 Implementation of ValInvariantIT
//===----------------------------------------------------------------------===//

ValInvariantIT::ValInvariantIT(Value *instrumentee, Instruction *location)
    : InvariantIT(ITargetKind::ITK_ValInvariant, instrumentee, location) {}

void ValInvariantIT::dump(raw_ostream &os) const { os << "value invariant"; }

bool ValInvariantIT::operator==(const ITarget &other) const {
  return equals(other);
}

bool ValInvariantIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_ValInvariant;
}

//===----------------------------------------------------------------------===//
//                 Implementation of BoundsIT
//===----------------------------------------------------------------------===//

BoundsIT::BoundsIT(Value *instrumentee, Instruction *location, bool checkUpper,
                   bool checkLower)
    : ITarget(ITargetKind::ITK_Bounds, instrumentee, location, checkUpper,
              checkLower, /*checkTemporal*/ false) {}

bool BoundsIT::requiresExplicitBounds() const { return true; }

void BoundsIT::dump(raw_ostream &os) const { os << "bounds"; }

bool BoundsIT::operator==(const ITarget &other) const { return equals(other); }

bool BoundsIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_Bounds;
}

//===----------------------------------------------------------------------===//
//                 Implementation of IntermediateIT
//===----------------------------------------------------------------------===//

IntermediateIT::IntermediateIT(Value *instrumentee, Instruction *location)
    : ITarget(ITargetKind::ITK_Intermediate, instrumentee, location,
              /*checkUpper*/ false, /*checkLower*/ false,
              /*checkTemporal*/ false) {}

bool IntermediateIT::joinFlags(const ITarget &other) {
  assert(isValid());
  assert(other.isValid());
  bool oldUBF = hasUpperBoundFlag();
  bool oldLBF = hasLowerBoundFlag();
  bool oldTF = hasTemporalFlag();
  bool oldExplBF = requiresExplicitBounds();

  checkUpperBoundFlag = hasUpperBoundFlag() || other.hasUpperBoundFlag();
  checkLowerBoundFlag = hasLowerBoundFlag() || other.hasLowerBoundFlag();
  checkTemporalFlag = hasTemporalFlag() || other.hasTemporalFlag();
  explicitBoundsFlag =
      requiresExplicitBounds() || other.requiresExplicitBounds();

  return oldUBF != checkUpperBoundFlag || oldLBF != checkLowerBoundFlag ||
         oldTF != checkTemporalFlag || oldExplBF != explicitBoundsFlag;
}

bool IntermediateIT::requiresExplicitBounds() const {
  assert(isValid());
  return explicitBoundsFlag;
}

void IntermediateIT::dump(raw_ostream &os) const {
  os << "intermediate target";
}

bool IntermediateIT::operator==(const ITarget &other) const {
  return equals(other) && explicitBoundsFlag == other.requiresExplicitBounds();
}

bool IntermediateIT::classof(const ITarget *T) {
  return T->getKind() == ITargetKind::ITK_Intermediate;
}