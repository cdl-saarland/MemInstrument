//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

#include "meminstrument/instrumentation_mechanisms/DummyMechanism.h"
#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"
#include "meminstrument/instrumentation_mechanisms/SplayMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/pass/Util.h"

using namespace llvm;
using namespace meminstrument;

std::unique_ptr<std::vector<Function *>>
InstrumentationMechanism::registerCtors(
    Module &M, ArrayRef<std::pair<StringRef, int>> List) {
  // Declare and register a list of functions with priorities as static
  // constructors. This is done in llvm by adding the functions to a special
  // global variable with the name "llvm.global_ctors" with appending linkage.
  auto &Ctx = M.getContext();
  auto FunTy = FunctionType::get(Type::getVoidTy(Ctx), false);
  size_t NumElements = List.size();

  Type *ComponentTypes[3];
  ComponentTypes[0] = Type::getInt32Ty(Ctx);      // priority
  ComponentTypes[1] = PointerType::get(FunTy, 0); // the actual function
  ComponentTypes[2] = Type::getInt8PtrTy(Ctx);
  // an optional globalvalue to connect with (here unnecessary)

  auto *ElemType = StructType::get(Ctx, ComponentTypes, false);
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

  if (auto *GV = M.getNamedGlobal("llvm.global_ctors")) {
    // If the special variable already exists, we have to remove it and add all
    // its arguments to our new global variable.
    if (GV->hasInitializer()) {
      auto *Init = GV->getInitializer();
      if (!Init->isZeroValue()) {
        // Trivial static constructors can lead to a just zero-initialized
        // global_ctors variable, we can just replace it then.
        auto *InitArr = dyn_cast<ConstantArray>(Init);
        assert(InitArr && "Constructor initializer is not a ConstantArray!");
        for (size_t i = 0; i < InitArr->getNumOperands(); ++i) {
          ArrInits.push_back(InitArr->getAggregateElement(i));
          NumElements++;
        }
      }
    }
    GV->eraseFromParent();
  }

  assert(!M.getNamedGlobal("llvm.global_ctors") &&
         "Failed to remove existing ctor variable!");

  auto *ArrType = ArrayType::get(ElemType, NumElements);
  auto *GV = new GlobalVariable(
      M, ArrType, /*isConstant*/ true, GlobalValue::AppendingLinkage,
      ConstantArray::get(ArrType, ArrInits), "llvm.global_ctors");
  setNoInstrument(GV);
  return Functions;
}

GlobalVariable *InstrumentationMechanism::insertStringLiteral(Module &M,
                                                              StringRef Str) {
  auto &Ctx = M.getContext();
  auto CharType = Type::getInt8Ty(Ctx);
  size_t NumElements = Str.size() + 1;

  auto *ArrType = ArrayType::get(CharType, NumElements);
  std::vector<Constant *> ArrInits;

  for (size_t i = 0; i < NumElements - 1; ++i) {
    auto *Val = Constant::getIntegerValue(CharType, APInt(8, Str[i]));

    ArrInits.push_back(Val);
  }
  ArrInits.push_back(Constant::getIntegerValue(CharType, APInt(8, 0)));

  auto *GV = new GlobalVariable(
      M, ArrType, /*isConstant*/ true, GlobalValue::InternalLinkage,
      ConstantArray::get(ArrType, ArrInits), "stringliteral." + Str);
  setNoInstrument(GV);
  return GV;
}

llvm::Constant *InstrumentationMechanism::insertFunDecl_impl(
    std::vector<llvm::Type *> &Vec, llvm::Module &M, llvm::StringRef Name,
    llvm::AttributeList AList, llvm::Type *RetTy) {
  auto *FunTy = FunctionType::get(RetTy, Vec, /*isVarArg*/ false);
  auto *Res = M.getOrInsertFunction(Name, FunTy, AList);
  setNoInstrument(Res);
  return Res;
}

llvm::Instruction *InstrumentationMechanism::insertCall_impl(
    std::vector<llvm::Value *> &Vec, llvm::IRBuilder<> &B, llvm::Constant *Fun,
    llvm::Twine &Name) {
  auto *Res = B.CreateCall(Fun, Vec, Name);
  setNoInstrument(Res);
  return Res;
}

llvm::Value *InstrumentationMechanism::insertCast(llvm::Type *DestType,
                                                  llvm::Value *FromVal,
                                                  llvm::IRBuilder<> &Builder,
                                                  llvm::StringRef Suffix) {
  auto *Res = Builder.CreateBitCast(FromVal, DestType, FromVal->getName() + Suffix);
  if (auto *I = dyn_cast<Instruction>(Res)) {
    setNoInstrument(I);
  }
  return Res;
}

llvm::Value *InstrumentationMechanism::insertCast(llvm::Type *DestType,
                                                  llvm::Value *FromVal,
                                                  llvm::IRBuilder<> &Builder) {
  return insertCast(DestType, FromVal, Builder, "_casted");
}

llvm::Value *InstrumentationMechanism::insertCast(llvm::Type *DestType,
                                                  llvm::Value *FromVal,
                                                  llvm::Instruction *Location) {
  IRBuilder<> Builder(Location);
  return insertCast(DestType, FromVal, Builder);
}

llvm::Value *InstrumentationMechanism::insertCast(llvm::Type *DestType,
                                                  llvm::Value *FromVal,
                                                  llvm::Instruction *Location,
                                                  llvm::StringRef Suffix) {
  IRBuilder<> Builder(Location);
  return insertCast(DestType, FromVal, Builder, Suffix);
}
