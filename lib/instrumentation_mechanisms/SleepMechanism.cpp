//===- SleepMechanism.cpp - Mechanism Simulating Checks -------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/SleepMechanism.h"

#include "meminstrument/Config.h"
#include "meminstrument/pass/Util.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace meminstrument;

cl::OptionCategory SleepInstrumentationCat("Sleep Instrumentation Options");

cl::opt<int> DefaultTime("mi-sleep-time-default",
                         cl::desc("default time that operations should take as "
                                  "a noop if not overwritten"),
                         cl::cat(SleepInstrumentationCat), cl::init(0));

cl::opt<int>
    GenBoundsTime("mi-sleep-time-gen-bounds",
                  cl::desc("time that generating bounds should take as a noop"),
                  cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int> DerefCheckTime(
    "mi-sleep-time-deref-check",
    cl::desc("time that a dereference check should take as a noop"),
    cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int> InvarCheckTime(
    "mi-sleep-time-invar-check",
    cl::desc("time that an inbounds check should take as a noop"),
    cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int> StackAllocTime(
    "mi-sleep-time-stack-alloc",
    cl::desc("time that a stack allocation should take as a noop"),
    cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int>
    HeapAllocTime("mi-sleep-time-heap-alloc",
                  cl::desc("time that a heap allocation should take as a noop"),
                  cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int> GlobalAllocTime(
    "mi-sleep-time-global-alloc",
    cl::desc("time that a global allocation should take as a noop"),
    cl::cat(SleepInstrumentationCat), cl::init(-1));

cl::opt<int> HeapFreeTime(
    "mi-sleep-time-heap-free",
    cl::desc("time that a heap deallocation should take as a noop"),
    cl::cat(SleepInstrumentationCat), cl::init(-1));

uint32_t getSleepValOrDefault(int32_t v) {
  if (v != -1) {
    return v;
  } else {
    return DefaultTime;
  }
}

#define GET_SLEEP_METHOD(x)                                                    \
  uint32_t getSleep##x##Time(void) { return getSleepValOrDefault(x##Time); }

GET_SLEEP_METHOD(GenBounds)
GET_SLEEP_METHOD(DerefCheck)
GET_SLEEP_METHOD(InvarCheck)
GET_SLEEP_METHOD(StackAlloc)
GET_SLEEP_METHOD(HeapAlloc)
GET_SLEEP_METHOD(GlobalAlloc)
GET_SLEEP_METHOD(HeapFree)

void SleepMechanism::initialize(Module &module) {

  // Let splay set up first
  SplayMechanism::initialize(module);

  // TODO add some infos
  auto *voidTy = Type::getVoidTy(module.getContext());
  auto configFunction =
      insertFunDecl(module, "__mi_config", voidTy, SizeType, SizeType);
  auto funTy = FunctionType::get(voidTy, false);
  auto *fun = Function::Create(funTy, GlobalValue::WeakAnyLinkage,
                               "__mi_init_callback__", &module);
  setNoInstrument(fun);

  auto *block = BasicBlock::Create(module.getContext(), "bb", fun, 0);
  IRBuilder<> builder(block);

  Constant *TimeVal = nullptr;
  Constant *IndexVal = nullptr;

#define ADD_TIME_VAL(i, x)                                                     \
  IndexVal = ConstantInt::get(SizeType, i);                                    \
  TimeVal = ConstantInt::get(SizeType, getSleep##x##Time());                   \
  insertCall(builder, configFunction, std::vector<Value *>{IndexVal, TimeVal});

  ADD_TIME_VAL(0, DerefCheck)
  ADD_TIME_VAL(1, InvarCheck)
  ADD_TIME_VAL(2, GenBounds)
  ADD_TIME_VAL(3, StackAlloc)
  ADD_TIME_VAL(4, HeapAlloc)
  ADD_TIME_VAL(5, GlobalAlloc)
  ADD_TIME_VAL(6, HeapFree)
  builder.CreateRetVoid();
}
