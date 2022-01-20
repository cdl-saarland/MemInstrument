//===- Setup.cpp - Main Instrumentation Pass ------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO
///
//===----------------------------------------------------------------------===//
#include "meminstrument/pass/Setup.h"

#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"

#include <string>

using namespace llvm;
using namespace meminstrument;

STATISTIC(NumVarArgs, "The # of function using varargs or a va_list");
STATISTIC(NumVarArgMetadataLoadStore,
          "The # of loads and stores tagged as varargs metadata management");

// Number of byval arguments converted.
STATISTIC(ByValArgsConverted, "Number of byval arguments converted at calls");

STATISTIC(NumFunctionsSkipped,
          "Number of functions ignored due to the given ignore file");

STATISTIC(NumPtrObfuscationStores,
          "Number of integer stores that actually store pointers");

STATISTIC(NumPtrObfuscationLoads,
          "Number of integer loads that actually load pointers");

STATISTIC(
    LoadStoreObfuscationPatternReplaced,
    "Number of load/store pairs of ints which are actually pointers replaced");

STATISTIC(ObfuscatedStoreWasNotPtrToInt,
          "Number of stores which where obfuscated but no ptrtoint casts");

STATISTIC(ObfuscatedStorePtrToInt,
          "Number of stores which where obfuscated by ptrtoint casts");

STATISTIC(IntroducedPtrToIntCasts, "Number of ptrtoint casts introduced when "
                                   "obfuscated pointer loads were converted");

STATISTIC(RemovedIntToPtrCast,
          "Number of inttoptr casts removed by directly loading a pointer");

cl::opt<bool> LabelAccesses(
    "mi-label-accesses",
    cl::desc("Add unique ids as metadata to load an store instructions"),
    cl::init(false));

cl::opt<bool> TransformObfuscatedPointer(
    "mi-transform-obfuscated-ptr-load-store",
    cl::desc("Transform obfuscated ptr loads/stores"), cl::init(true));

cl::opt<bool> ConvertByValArgs("mi-convert-byval-args",
                               cl::desc("Transform byval arguments"),
                               cl::init(true));

cl::opt<std::string> FunctionsToSkip(
    "mi-ignored-functions-file",
    cl::desc("Do not instrument functions listed in the given file. The file "
             "should contain one function name per line."));

/// Mark instruction that are only there to manage varargs and do not actually
/// belong to the source code. Currently, va_arg is rarely (not at all?) used in
/// LLVM, and its implementation leaks into the code. We cannot check if this
/// handling properly works and we therefore skip these instructions.
bool markVarargInsts(Function &fun) {

  // Determine which value contains the metadata for vararg accesses
  SmallVector<Value *, 2> handles = getVarArgHandles(fun);
  if (handles.empty()) {
    // If there are not varargs, we have nothing to do.
    return false;
  }

  // Determine loads and stores to this data structure.
  SmallVector<std::pair<Instruction *, unsigned>, 12> worklist;
  for (auto *handle : handles) {

    DEBUG_WITH_TYPE("MIVarArg",
                    { dbgs() << "Vararg handle: " << *handle << "\n"; });

    // Mark the handle itself `noinstrument` if possible.
    if (auto inst = dyn_cast<Instruction>(handle)) {
      setVarArgHandling(inst);
    }

    // Initialize the work list with all direct users of the vargargs
    for (auto *user : handle->users()) {
      if (auto inst = dyn_cast<Instruction>(user)) {
        worklist.push_back(std::make_pair(inst, 2));
      }
    }
  }

  while (!worklist.empty()) {
    Instruction *entry;
    unsigned propLevel;
    std::tie(entry, propLevel) = worklist.pop_back_val();

    // Label vararg specific calls.
    if (auto *intrInst = dyn_cast<IntrinsicInst>(entry)) {
      switch (intrInst->getIntrinsicID()) {
      case Intrinsic::vastart:
      case Intrinsic::vacopy:
      case Intrinsic::vaend: {
        // Mark the call as vararg related
        setVarArgHandling(intrInst);
        break;
      }
      default:
        break;
      }
      continue;
    }

    // Don't label calls just because varargs are handed over.
    if (isa<CallBase>(entry)) {
      continue;
    }

    // If the instruction is already marked as no instrument, ignore it
    if (hasNoInstrument(entry) || hasVarArgHandling(entry)) {
      continue;
    }

    // Collect some statistics on how many loads/stores belong to the vararg
    // metadata handling
    if (isa<LoadInst>(entry) || isa<StoreInst>(entry)) {
      ++NumVarArgMetadataLoadStore;
    }

    DEBUG_WITH_TYPE("MIVarArg", {
      dbgs() << "Vararg handle (Lvl " << propLevel << "): " << *entry << "\n";
    });

    // Don't instrument this vararg related instruction or value
    setVarArgHandling(entry);

    // The vararg metadata has two levels of metadata loads, decrease the level
    // when encountering a load
    if (isa<LoadInst>(entry)) {
      assert(propLevel > 0);
      propLevel--;

      // This load does actually load arguments that are handed over to the
      // function, mark it.
      if (propLevel == 0) {
        setVarArgLoadArg(entry);
      }
    }

    // Stop following users when the propagation level hits zero
    if (propLevel == 0) {
      continue;
    }

    // Add all users to the worklist
    for (auto *user : entry->users()) {
      if (auto inst = dyn_cast<Instruction>(user)) {
        worklist.push_back(std::make_pair(inst, propLevel));
      }
    }
  }

  return true;
}

/// Transform the given call byval arguments (if any) to hand over a pointer
/// to the call site allocated stuff.
void transformCallByValArgs(CallBase &call, IRBuilder<> &builder) {

  for (unsigned i = 0; i < call.getNumArgOperands(); i++) {
    // Find the byval attributes
    if (call.isByValArgument(i)) {

      ++ByValArgsConverted;

      auto *callArg = call.getArgOperand(i);
      assert(callArg->getType()->isPointerTy());
      auto callArgElemType = callArg->getType()->getPointerElementType();

      // Allocate memory for the value about to be copied
      builder.SetInsertPoint(
          &(*call.getCaller()->getEntryBlock().getFirstInsertionPt()));
      auto alloc = builder.CreateAlloca(callArgElemType);
      alloc->setName("byval.alloc");

      // Copy the value into the alloca
      builder.SetInsertPoint(&call);
      auto size =
          call.getModule()->getDataLayout().getTypeAllocSize(callArgElemType);
      auto cpy = builder.CreateMemCpy(alloc, alloc->getAlign(), callArg,
                                      alloc->getAlign(), size);
      // TODO We cannot mark this memcpy "noinstrument" as it might be relevant
      // to propagate metadata for some instrumentations. However, a check on
      // dest is redundant, as we generate the dest ourselves with a valid size
      // (hopefully).
      if (!cpy->getType()->isVoidTy()) {
        cpy->setName("byval.cpy");
      }

      // Replace the old argument with the newly copied one and make sure it is
      // no longer classified as byval.
      call.setArgOperand(i, alloc);
      call.removeParamAttr(i, Attribute::AttrKind::ByVal);
    }
  }

  return;
}

/// Transform all functions that have byval arguments to functions without
/// byval arguments (allocate memory for them at every call site).
void transformByValFunctions(Module &module) {

  IRBuilder<> builder(module.getContext());
  for (Function &fun : module) {

    // Remove byval attributes from functions
    for (Argument &arg : fun.args()) {
      if (arg.hasByValAttr()) {
        // Remove the attribute
        arg.removeAttr(Attribute::AttrKind::ByVal);
      }
    }

    if (fun.isDeclaration()) {
      continue;
    }

    // Remove byval arguments from calls
    for (auto &block : fun) {
      for (auto &inst : block) {
        if (auto *cb = dyn_cast<CallBase>(&inst)) {
          if (cb->hasByValArgument()) {
            transformCallByValArgs(*cb, builder);
          }
        }
      }
    }
  }
}

void labelAccesses(Function &F) {
  // This heavily relies on clang and llvm behaving deterministically
  // (which may or may not be the case)
  uint64_t idx = 0;
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isa<StoreInst>(&I) || isa<LoadInst>(&I)) {
        setAccessID(&I, idx++);
      }
    }
  }
}

void markFunctionsToSkip(Module &module) {

  // Check if there is something to do
  if (FunctionsToSkip.empty()) {
    return;
  }

  // Open to file
  ErrorOr<std::unique_ptr<MemoryBuffer>> fileContent =
      MemoryBuffer::getFile(FunctionsToSkip);

  if (auto errorCode = fileContent.getError()) {
    LLVM_DEBUG(dbgs() << "An error occurred while opening the file with "
                         "functions to skip:\n"
                      << errorCode.message() << "\n";);
    return;
  }

  // Read the function names from the file
  const auto &content = fileContent.get();
  llvm::SmallVector<std::string, 5> toIgnore;
  for (auto lineIt = line_iterator(*content.get(), true, '#');
       !lineIt.is_at_end(); lineIt++) {
    auto funName = lineIt->trim();
    LLVM_DEBUG(dbgs() << "Read function name: " << funName << "\n";);
    toIgnore.push_back(funName.str());
  }

  // Mark the functions in this module such that they will not be instrumented
  for (auto &fun : module) {
    if (fun.isDeclaration()) {
      continue;
    }

    if (!fun.hasName()) {
      continue;
    }

    auto name = fun.getName();
    if (std::find(toIgnore.begin(), toIgnore.end(), name) != toIgnore.end()) {
      ++NumFunctionsSkipped;
      LLVM_DEBUG(dbgs() << "Marked function noinstrument: " << name << "\n";);
      setNoInstrument(&fun);
    }
  }
}

bool isPtrToPtrTy(const Type *toCheck) {

  if (auto ptrTy = dyn_cast<PointerType>(toCheck)) {
    if (ptrTy->getElementType()->isPointerTy()) {
      return true;
    }
  }

  return false;
}

bool isUserWithCastingPotential(const Value *val) {
  return isa<IntToPtrInst>(val) || isa<BitCastInst>(val) || isa<PHINode>(val);
}

bool valCastedBeforeLoad(const LoadInst *inst) {

  // Check if the location that is loaded from is used as ** somewhere else
  const auto *ptrOp = inst->getPointerOperand();

  // Maybe the pointer operand itself is a bitcast from ** to i64*
  if (auto bitCast = dyn_cast<BitCastInst>(ptrOp)) {
    if (isPtrToPtrTy(bitCast->getSrcTy())) {
      return true;
    }
    auto bitCastSrc = bitCast->getOperand(0);
    for (const auto *useSrc : bitCastSrc->users()) {
      if (isUserWithCastingPotential(useSrc) &&
          isPtrToPtrTy(useSrc->getType())) {
        return true;
      }
      // TODO Guess phis could show up here again as well
      // This should follow bitcasts indefintely
    }
  }

  // Check the users, they might cast it to a pointer later on
  for (const auto *user : ptrOp->users()) {
    if (const auto *bitCast = dyn_cast<BitCastInst>(user)) {
      if (isPtrToPtrTy(bitCast->getSrcTy())) {
        return true;
      }
    }
  }

  return false;
}

// Find out if the load looking like this:
//  ... = load i64 from i64*
// or call looking like this:
//  ... = call i64 ...
// is actually loading a pointer value/returning a pointer value (approximated
// by the use of the value as pointer somewhere else)
bool valIsActuallyPtr(const Instruction *inst) {
  assert(inst->getType()->isIntegerTy());

  SmallVector<const Value *, 15> workList;
  for (auto *user : inst->users()) {
    if (isUserWithCastingPotential(user)) {
      workList.push_back(user);
    }
  }

  llvm::DenseSet<const Value *> seen;
  while (!workList.empty()) {
    const Value *item = workList.back();
    workList.pop_back();

    if (seen.contains(item)) {
      continue;
    }
    seen.insert(item);

    if (auto phi = dyn_cast<PHINode>(item)) {
      // See if phi value is casted later
      workList.push_back(phi);
    }

    // Look through bitcasts
    if (auto *bitCast = dyn_cast<BitCastInst>(item)) {
      for (auto *user : bitCast->users()) {
        if (isUserWithCastingPotential(user)) {
          workList.push_back(user);
        }
      }
    }

    // The same holds for int to ptr casts
    if (isa<IntToPtrInst>(item)) {
      return true;
    }

    // This is incomplete, e.g. people could do arithmetic on the integer value
    // and cast it later, don't support this for now
  }
  return false;
}

auto getSrcIfHasUsageAsPtr(Value *val) -> Value * {

  SmallVector<Value *, 15> workList;
  workList.push_back(val);

  llvm::DenseSet<Value *> seen;
  while (!workList.empty()) {
    Value *item = workList.back();
    workList.pop_back();

    if (seen.contains(item)) {
      continue;
    }

    if (item->getType()->isPointerTy()) {
      return item;
    }

    if (auto bitCastInst = dyn_cast<BitCastInst>(item)) {
      workList.push_back(bitCastInst->getOperand(0));
      for (auto *bitCastUser : bitCastInst->users()) {
        // Make sure not to add users such als calls, loads, etc. that do not
        // have the potential to change the type
        if (isUserWithCastingPotential(bitCastUser)) {
          workList.push_back(bitCastUser);
        }
      }
    }

    if (auto inst = dyn_cast<Instruction>(item)) {
      if (isa<CallInst>(inst) || isa<LoadInst>(inst)) {
        if (valIsActuallyPtr(inst)) {
          // The value returned from the call or load is used as a pointer
          // later. We can cast it and transform the store to store a pointer.
          return inst;
        }

        // In case of a load, the location we load from might actually be
        // holding a pointer, but it was casted before the load. Check for this
        // as well.
        if (auto loadInst = dyn_cast<LoadInst>(inst)) {
          if (valCastedBeforeLoad(loadInst)) {
            return inst;
          }
        }
      }
    }

    if (auto phi = dyn_cast<PHINode>(item)) {
      for (auto *phiUser : phi->users()) {
        if (isUserWithCastingPotential(phiUser)) {
          workList.push_back(phiUser);
        }
      }
    }

    seen.insert(item);
  }
  return nullptr;
}

auto createStoreLocationCast(IRBuilder<> &builder, Value *toCast,
                             Type *toCastTo) -> Value * {
  return builder.CreateBitCast(toCast, PointerType::getUnqual(toCastTo));
}

bool isIntegerTypeWithPtrWidth(const DataLayout &DL, const Type *ty) {
  if (!ty->isIntegerTy()) {
    return false;
  }
  return ty->getIntegerBitWidth() == DL.getPointerSizeInBits();
}

void transformObfuscatedLoad(LoadInst *load) {

  IRBuilder<> builder(load);
  // Create a new load with the correct type
  // We load an i64 from a i64*, so first create a bitcast of the source to i8**
  auto ptrToPtrCast = builder.CreateBitCast(
      load->getPointerOperand(),
      PointerType::getUnqual(PointerType::getInt8PtrTy(load->getContext())));

  auto ptrLoad = builder.CreateLoad(ptrToPtrCast, "mi.deobfuscated.ptr.load");

  // A user might have several occurrences of values that need to be replaced
  // (e.g. phis), use triples to collect the information
  DenseSet<std::tuple<User *, Value *, Value *>> replaceInfo;
  DenseSet<Instruction *> removeQueue;

  // Adapt users to use the new load
  for (auto *user : load->users()) {

    builder.SetInsertPoint(ptrLoad->getNextNode());

    // Check if the users require a i64 or cast the value immediately anyway
    if (auto intToPtrCast = dyn_cast<IntToPtrInst>(user)) {
      auto ptrTy = user->getType();

      Value *toReplaceWith = ptrLoad;
      // We might need to cast our i8* to the other pointer type
      if (ptrTy != ptrLoad->getType()) {
        toReplaceWith = builder.CreateBitCast(ptrLoad, ptrTy);
      }

      for (auto intToPtrCastUser : intToPtrCast->users()) {
        replaceInfo.insert(
            std::make_tuple(intToPtrCastUser, intToPtrCast, toReplaceWith));
      }
      RemovedIntToPtrCast++;
      removeQueue.insert(intToPtrCast);
      continue;
    }

    if (auto storeInst = dyn_cast<StoreInst>(user)) {
      auto storeLoc = createStoreLocationCast(
          builder, storeInst->getPointerOperand(), ptrLoad->getType());
      builder.CreateStore(ptrLoad, storeLoc);
      removeQueue.insert(storeInst);
      LoadStoreObfuscationPatternReplaced++;
      continue;
    }

    // Place a cast if the user really requires an integer
    auto ptrToInt = builder.CreatePtrToInt(ptrLoad, load->getType());
    replaceInfo.insert(std::make_tuple(user, load, ptrToInt));
    IntroducedPtrToIntCasts++;
  }

  for (auto &entry : replaceInfo) {
    std::get<0>(entry)->replaceUsesOfWith(std::get<1>(entry),
                                          std::get<2>(entry));
  }

  // Drop the old stores/intToPtr casts
  for (auto *inst : removeQueue) {
    inst->removeFromParent();
    inst->deleteValue();
  }

  // This load should have no more users
  assert(load->getNumUses() == 0);

  // Drop the old load
  load->removeFromParent();
  load->deleteValue();
}

void transformObfuscatedStore(StoreInst *store, Value *val) {

  // Load/Store pattern should already be replaced
  assert(!isa<LoadInst>(val));

  if (auto ptrToInt = dyn_cast<PtrToIntInst>(val)) {
    IRBuilder<> builder(store);
    auto srcType = ptrToInt->getSrcTy();
    auto storeLoc =
        createStoreLocationCast(builder, store->getPointerOperand(), srcType);
    builder.CreateStore(ptrToInt->getOperand(0), storeLoc);

    ObfuscatedStorePtrToInt++;
    // Drop the old store
    store->removeFromParent();
    store->deleteValue();
    return;
  }

  ObfuscatedStoreWasNotPtrToInt++;
}

// This is a best-effort to deobfuscate stores of i64 which should be pointer
// stores (and loads of the same kind). It is not complete, but hopefully sound.
void transformPointerObfuscations(Function &fun) {

  const auto &DL = fun.getParent()->getDataLayout();

  for (auto &block : fun) {
    std::map<StoreInst *, Value *> obfuscatedStores;
    DenseSet<LoadInst *> obfuscatedLoads;
    for (auto &inst : block) {

      if (auto *loadInst = dyn_cast<LoadInst>(&inst)) {

        // Ignore loads of non-pointer width integers
        if (!isIntegerTypeWithPtrWidth(DL, loadInst->getType())) {
          continue;
        }

        if (valCastedBeforeLoad(loadInst)) {
          NumPtrObfuscationLoads++;
          obfuscatedLoads.insert(loadInst);
        }

        if (valIsActuallyPtr(loadInst)) {
          NumPtrObfuscationLoads++;
          obfuscatedLoads.insert(loadInst);
        }
      }

      if (auto *storeInst = dyn_cast<StoreInst>(&inst)) {
        auto *toBeStored = storeInst->getValueOperand();

        // Ignore stores of non-pointer width integers
        if (!isIntegerTypeWithPtrWidth(DL, toBeStored->getType())) {
          continue;
        }

        // We found an i64 store, check if the same value was used somewhere
        // with a pointer type
        if (auto foundItem = getSrcIfHasUsageAsPtr(toBeStored)) {
          NumPtrObfuscationStores++;
          obfuscatedStores[storeInst] = foundItem;
        }
      }
    }
    DEBUG_WITH_TYPE("MIPtrObf", {
      if (!obfuscatedStores.empty() || !obfuscatedLoads.empty()) {
        dbgs() << "Resulting obfuscated pointer loads/stores:\n";
        for (const auto &entry : obfuscatedStores) {
          dbgs() << *entry.first << "\t" << *entry.second << "\n";
        }
        for (auto &loadInst : obfuscatedLoads) {
          dbgs() << *loadInst << "\n";
        }
        dbgs() << "In Function:\n" << fun << "\n\n";
      }
    });

    for (auto &loadInst : obfuscatedLoads) {
      // Check if there is a (store, load) entry in the store map, and if so,
      // remove it.
      StoreInst *locatedStore = nullptr;
      for (auto &entry : obfuscatedStores) {
        if (entry.second == loadInst) {
          locatedStore = entry.first;
          break;
        }
      }
      if (locatedStore) {
        obfuscatedStores.erase(locatedStore);
      }

      transformObfuscatedLoad(loadInst);
    }

    for (auto &entry : obfuscatedStores) {
      transformObfuscatedStore(entry.first, entry.second);
    }

    DEBUG_WITH_TYPE("MIPtrObf", {
      if (!obfuscatedStores.empty() || !obfuscatedLoads.empty()) {
        dbgs() << "Tranformed function:\n" << fun << "\n\n";
      }
    });
  }
}

void meminstrument::prepareModule(Module &module) {

  for (auto &fun : module) {
    if (fun.isDeclaration()) {
      continue;
    }

    if (LabelAccesses) {
      labelAccesses(fun);
    }

    // If the function has varargs or a va_list, mark bookkeeping loads and
    // stores for the vararg handling as such.
    if (markVarargInsts(fun)) {
      NumVarArgs++;
    }

    if (TransformObfuscatedPointer) {
      // Check if some values are used as pointers and are than stored(/loaded)
      // as i64 later. Replace these cases to consistently use pointers.
      transformPointerObfuscations(fun);
    }
  }

  if (ConvertByValArgs) {
    // Remove `byval` attributes by allocating them at call sites
    transformByValFunctions(module);
  }

  // Mark functions that should not be instrumented
  markFunctionsToSkip(module);
}
