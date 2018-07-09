//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/NoopMechanism.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "meminstrument/Config.h"

#include "meminstrument/pass/Util.h"

STATISTIC(NoopMechanismAnnotated, "The number of full bound checks placed");

using namespace llvm;
using namespace meminstrument;


namespace {

cl::opt<int> DefaultTime("mi-noop-time-default",
                            cl::desc("default time in microseconds that operations should take in the noop mechanism if not overwritten"),
                            cl::init(0) // default
);


cl::opt<int> GenWitnessTime("mi-noop-time-gen-witness",
                            cl::desc("time in microseconds that generating witnesses should take in the noop mechanism"),
                            cl::init(-1) // default
);

cl::opt<int> GenBoundsTime("mi-noop-time-gen-bounds",
                            cl::desc("time in microseconds that generating bounds should take in the noop mechanism"),
                            cl::init(-1) // default
);

cl::opt<int> CheckTime("mi-noop-time-check",
                            cl::desc("time in microseconds that an inbounds/dereference check should take in the noop mechanism"),
                            cl::init(-1) // default
);

// cl::opt<int> AllocaTime("mi-noop-time-alloca",
//                             cl::desc("time in microseconds that an alloca should take in the noop mechanism"),
//                             cl::init(-1) // default
// );

// cl::opt<int> HeapAllocTime("mi-noop-time-heapalloc",
//                             cl::desc("time in microseconds that a heap allocation should take in the noop mechanism"),
//                             cl::init(-1) // default
// );

// cl::opt<int> HeapFreeTime("mi-noop-time-heapfree",
//                             cl::desc("time in microseconds that a heap deallocation should take in the noop mechanism"),
//                             cl::init(-1) // default
// );

uint32_t getValOrDefault(int32_t v) {
  if (v != -1) {
    return v;
  } else {
    return DefaultTime;
  }
}
}


llvm::Value *NoopWitness::getLowerBound(void) const { return Lower; }

llvm::Value *NoopWitness::getUpperBound(void) const { return Upper; }

NoopWitness::NoopWitness(llvm::Instruction *Location, llvm::Value *Lower, llvm::Value *Upper) :
  Witness(WK_Noop), Location(Location), Lower(Lower), Upper(Upper) {}

llvm::Instruction *NoopWitness::getInsertionLocation() const {
  auto *Res = Location;
  while (isa<PHINode>(Res)) {
    Res = Res->getNextNode();
  }
  return Res;
}

bool NoopWitness::hasBoundsMaterialized(void) const {
  return hasBounds;
}

void NoopWitness::setBoundsMaterialized(void) {
  hasBounds = true;
}

void NoopMechanism::insertSleepCall(Instruction *Loc, uint32_t USecs) const {
  if (USecs == 0) {
    return;
  }
  IRBuilder<> Builder(Loc);
  auto USecVal = ConstantInt::get(I32Type, USecs);
  insertCall(Builder, SleepFunction, USecVal);
}

void NoopMechanism::insertWitness(ITarget &Target) const {
  assert(Target.isValid());
  insertSleepCall(Target.getLocation(), gen_witness_time);
  Target.setBoundWitness(
      std::make_shared<NoopWitness>(Target.getLocation(), LowerBoundVal, UpperBoundVal));
}

void NoopMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());

  insertSleepCall(Target.getLocation(), check_time);

  ++NoopMechanismAnnotated;
}

void NoopMechanism::materializeBounds(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.requiresExplicitBounds());

  if (!Target.hasBoundWitness()) {
    insertSleepCall(Target.getLocation(), gen_bounds_time);
    ++NoopMechanismAnnotated;
    return;
  }

  auto *Witness = cast<NoopWitness>(Target.getBoundWitness().get());

  if (Witness->hasBoundsMaterialized()) {
    return;
  }

  insertSleepCall(Witness->getInsertionLocation(), gen_bounds_time);
  Witness->setBoundsMaterialized();
  ++NoopMechanismAnnotated;
}

llvm::Constant *NoopMechanism::getFailFunction(void) const {
  return FailFunction;
}

bool NoopMechanism::initialize(llvm::Module &M) {
  auto &Ctx = M.getContext();

  I32Type = Type::getInt32Ty(Ctx);

  SizeType = Type::getInt64Ty(Ctx);

  llvm::Type *PtrArgType = Type::getInt8PtrTy(Ctx);

  size_t numbits = 64;

  llvm::APInt lowerVal = APInt::getNullValue(numbits);
  LowerBoundVal = ConstantExpr::getIntToPtr(ConstantInt::get(SizeType, lowerVal), PtrArgType);

  llvm::APInt upperVal = APInt::getAllOnesValue(numbits);
  UpperBoundVal = ConstantExpr::getIntToPtr(ConstantInt::get(SizeType, upperVal), PtrArgType);

  llvm::AttributeList NoReturnAttr = llvm::AttributeList::get(
      Ctx, llvm::AttributeList::FunctionIndex, llvm::Attribute::NoReturn);
  FailFunction =
      M.getOrInsertFunction("abort", NoReturnAttr, Type::getVoidTy(Ctx));

  SleepFunction = M.getOrInsertFunction("usleep", I32Type, I32Type);

  gen_witness_time = getValOrDefault(GenWitnessTime);
  gen_bounds_time = getValOrDefault(GenBoundsTime);
  check_time = getValOrDefault(CheckTime);
  // alloca_time = getValOrDefault(AllocaTime);
  // heapalloc_time = getValOrDefault(HeapAllocTime);
  // heapfree_time = getValOrDefault(HeapFreeTime);

  // TODO add sleeps for allocas and calls to memory management functions
  // for (auto &BB : F) {
  //   for (auto &I : BB) {
  //     if (isa<AllocaInst>(I)) {
  //       insertSleepCall(&I, alloca_time);
  //     } else if (auto CI = dyn_Cast<CallInst>(&I)) {
  //       // TODO use target library info for finding malloc/free/... calls
  //     }
  //   }
  // }

  return true;
}

std::shared_ptr<Witness> NoopMechanism::insertWitnessPhi(ITarget &Target) const {
  assert(Target.isValid());
  auto *Phi = cast<PHINode>(Target.getInstrumentee());
  Target.setBoundWitness(std::make_shared<NoopWitness>(Phi, LowerBoundVal, UpperBoundVal));
  return Target.getBoundWitness();
}

void NoopMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &,
                                            std::shared_ptr<Witness> &,
                                            llvm::BasicBlock *) const {
}

std::shared_ptr<Witness>
NoopMechanism::insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &,
                                   std::shared_ptr<Witness> &) const {
  assert(Target.isValid());
  auto *Sel = cast<SelectInst>(Target.getInstrumentee());
  Target.setBoundWitness(std::make_shared<NoopWitness>(Sel, LowerBoundVal, UpperBoundVal));
  return Target.getBoundWitness();
}
