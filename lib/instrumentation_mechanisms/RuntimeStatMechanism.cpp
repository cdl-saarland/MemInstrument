//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/pass/Util.h"

using namespace llvm;
using namespace meminstrument;

llvm::Value *RuntimeStatWitness::getLowerBound(void) const { return nullptr; }

llvm::Value *RuntimeStatWitness::getUpperBound(void) const { return nullptr; }

RuntimeStatWitness::RuntimeStatWitness(void) : Witness(WK_RuntimeStat) {}

void RuntimeStatMechanism::insertWitness(ITarget &Target) const {
  Target.BoundWitness = std::make_shared<RuntimeStatWitness>();
}

void RuntimeStatMechanism::insertCheck(ITarget &Target) const {
  IRBuilder<> Builder(Target.Location);
  // TODO

  auto *Witness = cast<RuntimeStatWitness>(Target.BoundWitness.get());
  auto *WitnessVal = Witness->WitnessValue;
  auto *CastVal = insertCast(PtrArgType, Target.Instrumentee, Builder);
  auto *Size = Target.HasConstAccessSize
                   ? ConstantInt::get(SizeType, Target.AccessSize)
                   : Target.AccessSizeVal;

  insertCall(Builder, CheckAccessFunction, CastVal, WitnessVal, Size);
}

void RuntimeStatMechanism::materializeBounds(ITarget &Target) const {
  llvm_unreachable("Explicit bounds are not supported by this mechanism!");
}

llvm::Constant *RuntimeStatMechanism::getFailFunction(void) const {
  llvm_unreachable("FailFunction calls are not supported by this mechanism!");
  return nullptr;
}

// std::unique_ptr<std::vector<Function *>>
// InstrumentationMechanism::registerCtors(
//     Module &M, ArrayRef<std::pair<StringRef, int>> List) {
//   // Declare and register a list of functions with priorities as static
//   // constructors. This is done in llvm by adding the functions to a special
//   // global variable with the name "llvm.global_ctors" with appending linkage.
//   auto &Ctx = M.getContext();
//   auto FunTy = FunctionType::get(Type::getVoidTy(Ctx), false);
//   size_t NumElements = List.size();
//
//   Type *ComponentTypes[3];
//   ComponentTypes[0] = Type::getInt32Ty(Ctx);      // priority
//   ComponentTypes[1] = PointerType::get(FunTy, 0); // the actual function
//   ComponentTypes[2] = Type::getInt8PtrTy(Ctx);
//   // an optional globalvalue to connect with (here unnecessary)
//
//   auto *ElemType = StructType::get(Ctx, ComponentTypes, false);
//   std::vector<Constant *> ArrInits;
//   std::unique_ptr<std::vector<Function *>> Functions(
//       new std::vector<Function *>());
//
//   for (auto &P : List) {
//     auto *Fun =
//         Function::Create(FunTy, GlobalValue::InternalLinkage, P.first, &M);
//     setNoInstrument(Fun);
//
//     Constant *ComponentConsts[3];
//     ComponentConsts[0] =
//         Constant::getIntegerValue(ComponentTypes[0], APInt(32, P.second));
//     ComponentConsts[1] = Fun;
//     ComponentConsts[2] = Constant::getNullValue(ComponentTypes[2]);
//
//     ArrInits.push_back(ConstantStruct::get(ElemType, ComponentConsts));
//     Functions->push_back(Fun);
//   }
//
//   if (auto *GV = M.getNamedGlobal("llvm.global_ctors")) {
//     // If the special variable already exists, we have to remove it and add all
//     // its arguments to our new global variable.
//     if (GV->hasInitializer()) {
//       auto *Init = GV->getInitializer();
//       if (!Init->isZeroValue()) {
//         // Trivial static constructors can lead to a just zero-initialized
//         // global_ctors variable, we can just replace it then.
//         auto *InitArr = dyn_cast<ConstantArray>(Init);
//         assert(InitArr && "Constructor initializer is not a ConstantArray!");
//         for (size_t i = 0; i < InitArr->getNumOperands(); ++i) {
//           ArrInits.push_back(InitArr->getAggregateElement(i));
//           NumElements++;
//         }
//       }
//     }
//     GV->eraseFromParent();
//   }
//
//   assert(!M.getNamedGlobal("llvm.global_ctors") &&
//          "Failed to remove existing ctor variable!");
//
//   auto *ArrType = ArrayType::get(ElemType, NumElements);
//   auto *GV = new GlobalVariable(
//       M, ArrType, #<{(|isConstant|)}># true, GlobalValue::AppendingLinkage,
//       ConstantArray::get(ArrType, ArrInits), "llvm.global_ctors");
//   setNoInstrument(GV);
//   return Functions;
// }
bool RuntimeStatMechanism::initialize(llvm::Module &M) {
  auto &Ctx = M.getContext();

  Type *ComponentTypes[2];
  ComponentTypes[0] = Type::getInt64Ty(Ctx);
  ComponentTypes[1] = Type::getInt8PtrTy(Ctx);

  auto *ElemType = StructType::get(Ctx, ComponentTypes, false);
  std::vector<Constant *> ArrInits;

  for (auto& F : M) {
    for (auto& BB : F) {
      for (auto& I : BB) {
        auto Name = F.getName() + "::" + BB.getName() + "::" + I.getName();
        Constant *ComponentConsts[2];
        ComponentConsts[0] = ConstantInt::get(ComponentTypes[0], 0);
        ComponentConsts[1] = insertStringLiteral(M, Name);
        ArrInits.push_back(ConstantStruct::get(ElemType, ComponentConsts));
      }
    }
  }

  auto *ArrType = ArrayType::get(ElemType, ArrInits.size());
  auto *GV = new GlobalVariable(
      M, ArrType, /*isConstant*/ false, GlobalValue::InternalLinkage,
      ConstantArray::get(ArrType, ArrInits), "__runtime_stat_table");
  setNoInstrument(GV);

  return true;
}

std::shared_ptr<Witness>
RuntimeStatMechanism::insertWitnessPhi(ITarget &) const {
  llvm_unreachable("Phis are not supported by this mechanism!")
  return shared_ptr<Witness>(nullptr);
}

void RuntimeStatMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &,
                                             std::shared_ptr<Witness> &,
                                             llvm::BasicBlock *) const {
  llvm_unreachable("Phis are not supported by this mechanism!")
}

std::shared_ptr<Witness> RuntimeStatMechanism::insertWitnessSelect(
  llvm_unreachable("Selects are not supported by this mechanism!")
  return shared_ptr<Witness>(nullptr);
}

