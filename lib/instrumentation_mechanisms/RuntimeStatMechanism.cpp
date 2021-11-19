//===- RuntimeStatMechanism.cpp - Runtime Statistics ----------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
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

Value *RuntimeStatWitness::getLowerBound(void) const { return nullptr; }

Value *RuntimeStatWitness::getUpperBound(void) const { return nullptr; }

RuntimeStatWitness::RuntimeStatWitness(void) : Witness(WK_RuntimeStat) {}

void RuntimeStatMechanism::insertWitnesses(ITarget &Target) const {
  assert(Target.isValid());
  // There should be no targets without an instrumentee
  assert(Target.hasInstrumentee());

  auto instrumentee = Target.getInstrumentee();

  if (!instrumentee->getType()->isAggregateType()) {
    Target.setSingleBoundWitness(std::make_shared<RuntimeStatWitness>());
    return;
  }

  if (isa<CallBase>(instrumentee) || isa<LandingPadInst>(instrumentee)) {
    // Find all locations of pointer values in the aggregate type
    auto indices = computePointerIndices(instrumentee->getType());
    for (auto index : indices) {
      Target.setBoundWitness(std::make_shared<RuntimeStatWitness>(), index);
    }
    return;
  }

  // The only aggregates that do not need a source are those that are constant
  assert(isa<Constant>(instrumentee));
  auto con = cast<Constant>(instrumentee);

  auto indexToPtr =
      InstrumentationMechanism::getAggregatePointerIndicesAndValues(con);
  for (auto KV : indexToPtr) {
    Target.setBoundWitness(std::make_shared<RuntimeStatWitness>(), KV.first);
  }
}

WitnessPtr RuntimeStatMechanism::getRelocatedClone(const Witness &,
                                                   Instruction *) const {
  return std::make_shared<RuntimeStatWitness>();
}

void RuntimeStatMechanism::insertCheck(ITarget &Target) const {
  assert(Target.isValid());
  assert(Target.isCheck());
  IRBuilder<> Builder(Target.getLocation());

  uint64_t idx = 0;
  if (Verbose) {
    const auto &It = StringMap.find(Target.getLocation());
    assert(It != StringMap.end() &&
           "RT instrumentation required for unknown instruction!");
    if (Target.getLocation()->getMetadata(markString)) {
      ++RTStatNumNoSan;
    } else {
      ++RTStatNumWild;
    }
    idx = It->second.idx;
  } else {
    if (isa<LoadInst>(Target.getLocation()) &&
        Target.getLocation()->getMetadata(markString)) {
      idx = 5 * NoSanLoadIdx;
      ++RTStatNumNoSanLoads;
      ++RTStatNumNoSan;
    } else if (isa<StoreInst>(Target.getLocation()) &&
               Target.getLocation()->getMetadata(markString)) {
      idx = 5 * NoSanStoreIdx;
      ++RTStatNumNoSanStores;
      ++RTStatNumNoSan;
    } else if (isa<LoadInst>(Target.getLocation())) {
      idx = 5 * LoadIdx;
      ++RTStatNumNormalLoads;
      ++RTStatNumWild;
    } else if (isa<StoreInst>(Target.getLocation())) {
      idx = 5 * StoreIdx;
      ++RTStatNumNormalStores;
      ++RTStatNumWild;
    }
    if (Target.getLocation()->getFunction()->getMetadata("PMDAprecise")) {
      idx += PMDApreciseIdx;
      ++RTStatNumPMDAprecise;
    } else if (Target.getLocation()->getFunction()->getMetadata(
                   "PMDAsummary")) {
      idx += PMDAsummaryIdx;
      ++RTStatNumPMDAsummary;
    } else if (Target.getLocation()->getFunction()->getMetadata("PMDAlocal")) {
      idx += PMDAlocalIdx;
      ++RTStatNumPMDAlocal;
    } else if (Target.getLocation()->getFunction()->getMetadata("PMDAbad")) {
      idx += PMDAbadIdx;
      ++RTStatNumPMDAbad;
    }
  }
  auto *tableID = Builder.CreateLoad(StatTableID, "stat_table_id");
  insertCall(Builder, StatIncFunction,
             std::vector<Value *>{tableID, ConstantInt::get(SizeType, idx)});
}

void RuntimeStatMechanism::materializeBounds(ITarget &) {
  llvm_unreachable("Explicit bounds are not supported by this mechanism!");
}

FunctionCallee RuntimeStatMechanism::getFailFunction() const {
  llvm_unreachable("FailFunction calls are not supported by this mechanism!");
}

uint64_t RuntimeStatMechanism::populateStringMap(Module &M) {
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

          if (hasAccessID(&I)) {
            // if we added labels to accesses, we should use them
            auto ID = getAccessID(&I);

            Name += M.getName();
            Name += ",";
            Name += F.getName();
            Name += ",";
            Name += std::to_string(ID);
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

void RuntimeStatMechanism::initialize(Module &M) {
  Verbose = globalConfig.hasInstrumentVerbose();
  auto &Ctx = M.getContext();

  SizeType = Type::getInt64Ty(Ctx);
  auto *StringType = Type::getInt8PtrTy(Ctx);
  auto *VoidTy = Type::getVoidTy(Ctx);

  StatIncFunction =
      insertFunDecl(M, "__mi_stat_inc", VoidTy, SizeType, SizeType);

  StatTableID =
      new GlobalVariable(M, SizeType, false, GlobalValue::InternalLinkage,
                         Constant::getNullValue(SizeType), "MI_StatID");

  auto InitFun = insertFunDecl(M, "__mi_stat_init", SizeType, SizeType);
  auto InitEntryFun = insertFunDecl(M, "__mi_stat_init_entry", VoidTy, SizeType,
                                    SizeType, StringType);

  auto Fun =
      registerCtors(M, std::make_pair<StringRef, int>("__mi_stat_setup", 0));

  auto *BB = BasicBlock::Create(Ctx, "bb", (*Fun)[0], 0);
  IRBuilder<> Builder(BB);

  if (Verbose) {
    uint64_t Count = populateStringMap(M);
    auto *call =
        insertCall(Builder, InitFun, ConstantInt::get(SizeType, Count));

    // store call result to global variable StatTableID
    Builder.CreateStore(call, StatTableID);

    for (const auto &P : StringMap) {
      uint64_t idx = P.second.idx;
      std::string &name = P.second.str;
      Value *Str = insertStringLiteral(M, name);
      Str = insertCast(StringType, Str, Builder);
      insertCall(
          Builder, InitEntryFun,
          std::vector<Value *>{call, ConstantInt::get(SizeType, idx), Str});
    }
  } else {
    auto *call = insertCall(Builder, InitFun, ConstantInt::get(SizeType, 25));

    auto addEntry = [&](uint64_t idx, StringRef text) {
      Value *Str = insertStringLiteral(M, text.str().c_str());
      Str = insertCast(StringType, Str, Builder);
      insertCall(
          Builder, InitEntryFun,
          std::vector<Value *>{call, ConstantInt::get(SizeType, idx), Str});
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
}

WitnessPtr RuntimeStatMechanism::getWitnessPhi(PHINode *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
  return nullptr;
}

void RuntimeStatMechanism::addIncomingWitnessToPhi(WitnessPtr &, WitnessPtr &,
                                                   BasicBlock *) const {
  llvm_unreachable("Phis are not supported by this mechanism!");
}

WitnessPtr RuntimeStatMechanism::getWitnessSelect(SelectInst *, WitnessPtr &,
                                                  WitnessPtr &) const {
  llvm_unreachable("Selects are not supported by this mechanism!");
  return nullptr;
}

bool RuntimeStatMechanism::invariantsAreChecks() const { return false; }
