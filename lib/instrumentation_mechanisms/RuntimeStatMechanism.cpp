//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/instrumentation_mechanisms/RuntimeStatMechanism.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "meminstrument/Config.h"

#include "meminstrument/pass/Util.h"

STATISTIC(RTStatNumNormalLoads, "The static # of normal loads");
STATISTIC(RTStatNumNormalStores, "The static # of normal stores");

STATISTIC(RTStatNumNoSanLoads, "The static # of nosanitize loads");
STATISTIC(RTStatNumNoSanStores, "The static # of nosanitize stores");

STATISTIC(RTStatNumWild, "The static # of wild memory operations");
STATISTIC(RTStatNumNoSan, "The static # of nosanitize memory operations");

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
  if (Verbose) {
    const auto &It = StringMap.find(Target.Location);
    if (It == StringMap.end()) {
      llvm_unreachable("RT instrumentation required for unknown instruction!");
    }
    if (Target.Location->getMetadata("nosanitize")) {
      ++RTStatNumNoSan;
    } else {
      ++RTStatNumWild;
    }
    idx = It->second.idx;
  } else {
    if (isa<LoadInst>(Target.Location) &&
        Target.Location->getMetadata("nosanitize")) {
      idx = NoSanLoadIdx;
      ++RTStatNumNoSanLoads;
      ++RTStatNumNoSan;
    } else if (isa<StoreInst>(Target.Location) &&
               Target.Location->getMetadata("nosanitize")) {
      idx = NoSanStoreIdx;
      ++RTStatNumNoSanStores;
      ++RTStatNumNoSan;
    } else if (isa<LoadInst>(Target.Location)) {
      idx = LoadIdx;
      ++RTStatNumNormalLoads;
      ++RTStatNumWild;
    } else if (isa<StoreInst>(Target.Location)) {
      idx = StoreIdx;
      ++RTStatNumNormalStores;
      ++RTStatNumWild;
    }
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

uint64_t RuntimeStatMechanism::populateStringMap(llvm::Module &M) {
  uint64_t Counter = 0;
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        const char *Kind = nullptr;
        if (I.getMetadata("nosanitize")) {
          if (isa<LoadInst>(I)) {
            Kind = "nosanitize load";
          } else if (isa<StoreInst>(I)) {
            Kind = "nosanitize store";
          }
        } else {
          if (isa<LoadInst>(I)) {
            Kind = "wild load";
          } else if (isa<StoreInst>(I)) {
            Kind = "wild store";
          }
        }

        if (Kind != nullptr) {
          std::string Name;
          if (DILocation *Loc = I.getDebugLoc()) {
            unsigned Line = Loc->getLine();
            unsigned Column = Loc->getColumn();
            StringRef File = Loc->getFilename();
            Name = (File + " - l " + std::to_string(Line) + " - c " + std::to_string(Column) + " - ").str();
          } else {
            Name = std::string("unknown location - ");
          }
          Name += Kind;
          Name += " - ";
          Name += I.getName();
          StringMap.insert(std::make_pair(&I, StringMapItem(Counter, Name)));
          Counter++;
        }
      }
    }
  }
  return Counter;
}

bool RuntimeStatMechanism::initialize(llvm::Module &M) {
  Verbose = GlobalConfig::get(M).hasInstrumentVerbose();
  auto &Ctx = M.getContext();

  SizeType = Type::getInt64Ty(Ctx);
  auto *StringType = Type::getInt8PtrTy(Ctx);
  auto *VoidTy = Type::getVoidTy(Ctx);

  StatIncFunction = insertFunDecl(M, "__mi_stat_inc", VoidTy, SizeType);
  llvm::Constant *InitFun =
      insertFunDecl(M, "__mi_stat_init", VoidTy, SizeType);
  llvm::Constant *InitEntryFun =
      insertFunDecl(M, "__mi_stat_init_entry", VoidTy, SizeType, StringType);

  auto Fun =
      registerCtors(M, std::make_pair<StringRef, int>("__mi_stat_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);

  if (Verbose) {
    uint64_t Count = populateStringMap(M);
    insertCall(Builder, InitFun, ConstantInt::get(SizeType, Count));

    for (const auto &P : StringMap) {
      uint64_t idx = P.second.idx;
      std::string& name = P.second.str;
      llvm::Value *Str = insertStringLiteral(M, name);
      Str = insertCast(StringType, Str, Builder);
      insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, idx), Str);
    }
  } else {
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
    insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, NoSanLoadIdx),
               Str);

    Str = insertStringLiteral(M, "nosanitize stores");
    Str = insertCast(StringType, Str, Builder);
    insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, NoSanStoreIdx),
               Str);
  }

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

std::shared_ptr<Witness>
RuntimeStatMechanism::insertWitnessSelect(ITarget &, std::shared_ptr<Witness> &,
                                          std::shared_ptr<Witness> &) const {
  llvm_unreachable("Selects are not supported by this mechanism!");
  return std::shared_ptr<Witness>(nullptr);
}
