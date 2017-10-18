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

  uint64_t idx = 0;
  if (isa<LoadInst>(Target.Location)) {
    idx = LoadIdx;
  } else if (isa<StoreInst>(Target.Location)) {
    idx = StoreIdx;
  }
  if (Target.Location->getMetadata("nosanitize")) {
    idx += 2;
  }
  insertCall(Builder, StatIncFunction, ConstantInt::get(SizeType, idx));
}

void RuntimeStatMechanism::materializeBounds(ITarget &Target) const {
  llvm_unreachable("Explicit bounds are not supported by this mechanism!");
}

llvm::Constant *RuntimeStatMechanism::getFailFunction(void) const {
  llvm_unreachable("FailFunction calls are not supported by this mechanism!");
  return nullptr;
}

bool RuntimeStatMechanism::initialize(llvm::Module &M) {
  auto &Ctx = M.getContext();

  SizeType = Type::getInt64Ty(Ctx);
  auto* StringType = Type::getInt8PtrTy(Ctx);
  auto* VoidTy = Type::getVoidTy(Ctx);

  StatIncFunction = insertFunDecl(M, "__mi_stat_inc", VoidTy, SizeType);
  llvm::Constant* InitFun = insertFunDecl(M, "__mi_stat_init", VoidTy, SizeType);
  llvm::Constant* InitEntryFun = insertFunDecl(M, "__mi_stat_init_entry", VoidTy, SizeType, StringType);

  auto Fun = registerCtors(M, std::make_pair<StringRef, int>("__mi_stat_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);

  insertCall(Builder, InitFun, ConstantInt::get(SizeType, 5));

  llvm::Value *Str = insertStringLiteral(M, "others");
  Str = insertCast(StringType, Str, Builder);
  insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, 0), Str);

  Str = insertStringLiteral(M, "normal loads");
  Str = insertCast(StringType, Str, Builder);
  insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, LoadIdx), Str);

  Str = insertStringLiteral(M, "normal stores");
  Str = insertCast(StringType, Str, Builder);
  insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, StoreIdx), Str);

  Str = insertStringLiteral(M, "nosanitize loads");
  Str = insertCast(StringType, Str, Builder);
  insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, NoSanLoadIdx), Str);

  Str = insertStringLiteral(M, "nosanitize stores");
  Str = insertCast(StringType, Str, Builder);
  insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, NoSanStoreIdx), Str);

  Builder.CreateRetVoid();

  return true;
}

std::shared_ptr<Witness>
RuntimeStatMechanism::insertWitnessPhi(ITarget &) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
  return std::shared_ptr<Witness>(nullptr);
}

void RuntimeStatMechanism::addIncomingWitnessToPhi(std::shared_ptr<Witness> &,
                                             std::shared_ptr<Witness> &,
                                             llvm::BasicBlock *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
}

std::shared_ptr<Witness> RuntimeStatMechanism::insertWitnessSelect(ITarget &, std::shared_ptr<Witness> &,
                      std::shared_ptr<Witness> &) const {
  llvm_unreachable("Selects are not supported by this mechanism!");
  return std::shared_ptr<Witness>(nullptr);
}

