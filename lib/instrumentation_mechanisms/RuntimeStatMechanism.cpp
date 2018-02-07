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

STATISTIC(RTStatNumNormalLoads, "The static # of unmarked loads");
STATISTIC(RTStatNumNormalStores, "The static # of unmarked stores");

STATISTIC(RTStatNumNoSanLoads, "The static # of marked loads");
STATISTIC(RTStatNumNoSanStores, "The static # of marked stores");

STATISTIC(RTStatNumWild, "The static # of unmarked memory operations");
STATISTIC(RTStatNumNoSan, "The static # of marked memory operations");

STATISTIC(RTStatNumPMDAprecise,
          "The static # of PMDAprecise memory operations");
STATISTIC(RTStatNumPMDAsummary,
          "The static # of PMDAsummary memory operations");
STATISTIC(RTStatNumPMDAlocal, "The static # of PMDAlocal memory operations");
STATISTIC(RTStatNumPMDAbad, "The static # of PMDAbad memory operations");

// TODO this could (and probably should) be made more parametric in the
// statistics to gather

namespace {
const char *markString = "nosanitize";
// const char *markString = "temporallySafe";
} // namespace

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
    if (Target.Location->getMetadata(markString)) {
      ++RTStatNumNoSan;
    } else {
      ++RTStatNumWild;
    }
    idx = It->second.idx;
  } else {
    if (isa<LoadInst>(Target.Location) &&
        Target.Location->getMetadata(markString)) {
      idx = 5 * NoSanLoadIdx;
      ++RTStatNumNoSanLoads;
      ++RTStatNumNoSan;
    } else if (isa<StoreInst>(Target.Location) &&
               Target.Location->getMetadata(markString)) {
      idx = 5 * NoSanStoreIdx;
      ++RTStatNumNoSanStores;
      ++RTStatNumNoSan;
    } else if (isa<LoadInst>(Target.Location)) {
      idx = 5 * LoadIdx;
      ++RTStatNumNormalLoads;
      ++RTStatNumWild;
    } else if (isa<StoreInst>(Target.Location)) {
      idx = 5 * StoreIdx;
      ++RTStatNumNormalStores;
      ++RTStatNumWild;
    }
    if (Target.Location->getFunction()->getMetadata("PMDAprecise")) {
      idx += PMDApreciseIdx;
      ++RTStatNumPMDAprecise;
    } else if (Target.Location->getFunction()->getMetadata("PMDAsummary")) {
      idx += PMDAsummaryIdx;
      ++RTStatNumPMDAsummary;
    } else if (Target.Location->getFunction()->getMetadata("PMDAlocal")) {
      idx += PMDAlocalIdx;
      ++RTStatNumPMDAlocal;
    } else if (Target.Location->getFunction()->getMetadata("PMDAbad")) {
      idx += PMDAbadIdx;
      ++RTStatNumPMDAbad;
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
    if (F.isDeclaration()) {
      continue;
    }

    const char *functionAnnotation = "[missing]";

    if (F.getMetadata("PMDAlocal")) {
      functionAnnotation = "PMDAlocal";
    } else if (F.getMetadata("PMDAprecise")) {
      functionAnnotation = "PMDAprecise";
    } else if (F.getMetadata("PMDAsummary")) {
      functionAnnotation = "PMDAsummary";
    } else if (F.getMetadata("PMDAbad")) {
      functionAnnotation = "PMDAbad";
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        const char *Kind = nullptr;
        if (I.getMetadata(markString)) {
          if (isa<LoadInst>(I)) {
            Kind = "marked load";
          } else if (isa<StoreInst>(I)) {
            Kind = "marked store";
          }
        } else {
          if (isa<LoadInst>(I)) {
            Kind = "unmarked load";
          } else if (isa<StoreInst>(I)) {
            Kind = "unmarked store";
          }
        }

        if (Kind != nullptr) {
          std::string Name;

          if (auto *N = I.getMetadata("mi_access_id")) {
            // if we added labels to accesses, we should use them
            assert(N->getNumOperands() == 1);

            assert(isa<MDString>(N->getOperand(0)));
            auto *Str = cast<MDString>(N->getOperand(0));
            Name += M.getName();
            Name += ",";
            Name += F.getName();
            Name += ",";
            Name += Str->getString();
          } else {
            if (DILocation *Loc = I.getDebugLoc()) {
              unsigned Line = Loc->getLine();
              unsigned Column = Loc->getColumn();
              StringRef File = Loc->getFilename();
              Name = (File + " - l " + std::to_string(Line) + " - c " +
                      std::to_string(Column) + " - ")
                         .str();
            } else {
              Name = std::string("unknown location - ");
            }
            Name += Kind;
            Name += " ";
            Name += functionAnnotation;
            Name += " - ";
            Name += I.getName();
          }

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
      std::string &name = P.second.str;
      llvm::Value *Str = insertStringLiteral(M, name);
      Str = insertCast(StringType, Str, Builder);
      insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, idx), Str);
    }
  } else {
    insertCall(Builder, InitFun, ConstantInt::get(SizeType, 25));

    auto addEntry = [&](uint64_t idx, StringRef text) {
      llvm::Value *Str = insertStringLiteral(M, text.str().c_str());
      Str = insertCast(StringType, Str, Builder);
      insertCall(Builder, InitEntryFun, ConstantInt::get(SizeType, idx), Str);
    };

    addEntry(0, "others");
    addEntry(1, "others1");
    addEntry(2, "others2");
    addEntry(3, "others3");
    addEntry(4, "others4");

    addEntry(5 * LoadIdx, "unmarked loads PMDAmissingdata");
    addEntry(5 * LoadIdx + PMDApreciseIdx, "unmarked loads PMDAprecise");
    addEntry(5 * LoadIdx + PMDAsummaryIdx, "unmarked loads PMDAsummary");
    addEntry(5 * LoadIdx + PMDAlocalIdx, "unmarked loads PMDAlocal");
    addEntry(5 * LoadIdx + PMDAbadIdx, "unmarked loads PMDAbad");

    addEntry(5 * StoreIdx, "unmarked stores PMDAmissingdata");
    addEntry(5 * StoreIdx + PMDApreciseIdx, "unmarked stores PMDAprecise");
    addEntry(5 * StoreIdx + PMDAsummaryIdx, "unmarked stores PMDAsummary");
    addEntry(5 * StoreIdx + PMDAlocalIdx, "unmarked stores PMDAlocal");
    addEntry(5 * StoreIdx + PMDAbadIdx, "unmarked stores PMDAbad");

    addEntry(5 * NoSanLoadIdx, "marked loads PMDAmissingdata");
    addEntry(5 * NoSanLoadIdx + PMDApreciseIdx, "marked loads PMDAprecise");
    addEntry(5 * NoSanLoadIdx + PMDAsummaryIdx, "marked loads PMDAsummary");
    addEntry(5 * NoSanLoadIdx + PMDAlocalIdx, "marked loads PMDAlocal");
    addEntry(5 * NoSanLoadIdx + PMDAbadIdx, "marked loads PMDAbad");

    addEntry(5 * NoSanStoreIdx, "marked stores PMDAmissingdata");
    addEntry(5 * NoSanStoreIdx + PMDApreciseIdx, "marked stores PMDAprecise");
    addEntry(5 * NoSanStoreIdx + PMDAsummaryIdx, "marked stores PMDAsummary");
    addEntry(5 * NoSanStoreIdx + PMDAlocalIdx, "marked stores PMDAlocal");
    addEntry(5 * NoSanStoreIdx + PMDAbadIdx, "marked stores PMDAbad");
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
