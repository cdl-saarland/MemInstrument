//===---- InstrumentationMechanism.cpp -- MemSafety Instrumentation -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/InstrumentationMechanism.h"

#include "meminstrument/DummyMechanism.h"
#include "meminstrument/SplayMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/Util.h"

using namespace llvm;
using namespace meminstrument;

namespace {

enum InstrumentationMechanismKind {
  IM_dummy,
  IM_splay,
};

cl::opt<InstrumentationMechanismKind> InstrumentationMechanismOpt(
    "memsafety-imechanism",
    cl::desc("Choose InstructionMechanism: (default: dummy)"),
    cl::values(clEnumValN(IM_dummy, "dummy",
                          "only insert dummy calls for instrumentation")),
    cl::values(clEnumValN(IM_splay, "splay",
                          "use splay tree for instrumentation")),
    cl::init(IM_dummy) // default
    );

std::unique_ptr<InstrumentationMechanism> GlobalIM(nullptr);
}

InstrumentationMechanism &InstrumentationMechanism::get(void) {
  auto *Res = GlobalIM.get();
  if (Res == nullptr) {
    switch (InstrumentationMechanismOpt) {
    case IM_dummy:
      GlobalIM.reset(new DummyMechanism());
      break;

    case IM_splay:
      GlobalIM.reset(new SplayMechanism());
      break;
    }
    Res = GlobalIM.get();
  }
  return *Res;
}

std::unique_ptr<std::vector<Function *>>
InstrumentationMechanism::registerCtors(
    Module &M, ArrayRef<std::pair<StringRef, int>> List) {
  auto &Ctx = M.getContext();
  auto FunTy = FunctionType::get(Type::getVoidTy(Ctx), false);
  size_t NumElements = List.size();

  Type *ComponentTypes[3];
  ComponentTypes[0] = Type::getInt32Ty(Ctx);
  ComponentTypes[1] = PointerType::get(FunTy, 0);
  ComponentTypes[2] = Type::getInt8PtrTy(Ctx);
  auto *ElemType = StructType::get(Ctx, ComponentTypes, false);
  auto *ArrType = ArrayType::get(ElemType, NumElements);
  std::vector<Constant *> ArrInits;
  std::unique_ptr<std::vector<Function *>> Functions(
      new std::vector<Function *>());

  for (auto &P : List) {
    auto *Fun =
        Function::Create(FunTy, GlobalValue::InternalLinkage, P.first, &M);
    setNoInstrument(Fun);

    Constant *ComponentConsts[3];
    ComponentConsts[0] =
        Constant::getIntegerValue(ComponentTypes[0], APInt(32, P.second));
    ComponentConsts[1] = Fun;
    ComponentConsts[2] = Constant::getNullValue(ComponentTypes[2]);

    ArrInits.push_back(ConstantStruct::get(ElemType, ComponentConsts));
    Functions->push_back(Fun);
  }

  auto *GV = new GlobalVariable(
      M, ArrType, /*isConstant*/ true, GlobalValue::AppendingLinkage,
      ConstantArray::get(ArrType, ArrInits), "llvm.global_ctors");
  setNoInstrument(GV);
  return Functions;
}
