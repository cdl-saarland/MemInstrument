//===- AfterInflowStrategy.cpp - After Inflow Strategy --------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/AfterInflowStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/Support/CommandLine.h"

#include <set>

#ifdef DEBUG_WG_SIMPLIFY
#include <sstream>
#endif

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {
STATISTIC(NumITargetsRemovedWGSimplify, "The # of inbounds targets discarded "
                                        "because of witness graph information");

cl::opt<bool> NoShareBoundsOpt(
    "mi-no-bounds-share",
    cl::desc(
        "Materialize bounds so that multiple targets can have the same bounds"),
    cl::init(false));

cl::opt<bool> OptimizeWitnessGraph(
    "mi-ais-optimize-witness-graph",
    cl::desc("Optimize the witnessgraph. Remove invariants whenever a pointer "
             "value did not change since its last check/inflow."),
    cl::init(false));

} // namespace

void AfterInflowStrategy::getPointerOperands(std::vector<Value *> &Results,
                                             Constant *C) const {
  assert(C->getType()->isPointerTy() &&
         "getPointerOperands() called for non-pointer constant!");

  if (auto *GV = dyn_cast<GlobalValue>(C)) {
    Results.push_back(GV);
    return;
  }

  if (auto *CD = dyn_cast<ConstantData>(C)) {
    Results.push_back(CD);
    return;
  }

  if (auto *CE = dyn_cast<ConstantExpr>(C)) {
    switch (CE->getOpcode()) {
    case Instruction::GetElementPtr:
      getPointerOperands(Results, CE->getOperand(0)); // pointer argument
      break;
    case Instruction::BitCast:
      getPointerOperands(Results, CE->getOperand(0)); // pointer argument
      break;
    case Instruction::IntToPtr:
      Results.push_back(CE);
      break;
    default:
      llvm_unreachable("Unexpected constant expression encountered.");
    }

    return;
  }

  llvm_unreachable("Unexpected constant value encountered.");
}

void AfterInflowStrategy::addRequired(WitnessGraphNode *Node) const {
  if (Node->HasAllRequirements) {
    return;
  }

  Node->HasAllRequirements = true;

  auto &Target = Node->Target;
  // Call invariant targets do not have requirements
  if (auto callIT = dyn_cast<CallInvariantIT>(Target)) {
    for (auto elem : callIT->getRequiredArgs()) {
      requireRecursively(Node, elem.second, callIT->getCall());
    }
    return;
  }

  assert(Target->hasInstrumentee());
  Instruction *Instrumentee = dyn_cast<Instruction>(Target->getInstrumentee());

  auto &WG = Node->Graph;

  if (Instrumentee) {
    switch (Instrumentee->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::LandingPad:
    case Instruction::IntToPtr: {
      // Introduce a target without requirements for these values. We assume
      // that these are valid pointers.
      requireSource(Node, Instrumentee, Instrumentee->getNextNode());
      return;
    }
    case Instruction::Load: {
      auto load = dyn_cast<LoadInst>(Instrumentee);
      // If this load is not actually loading from the varargs handed over to
      // the function, require bounds for its source structure
      if (!hasVarArgLoadArg(load) && hasVarArgHandling(load) &&
          !isVarArgMetadataType(load->getType())) {
        requireRecursively(Node, load->getPointerOperand(), load);
        return;
      }
      // Require bounds for loads of pointers
      requireSource(Node, Instrumentee, Instrumentee->getNextNode());
      return;
    }
    case Instruction::Invoke: {
      // We cannot insert witnesses after invokes as these are terminators.
      // Therefore, use the first non-phi instruction of the 'normal' successor.
      auto *II = dyn_cast<InvokeInst>(Instrumentee);
      auto *Loc = &(*II->getNormalDest()->getFirstInsertionPt());
      requireSource(Node, II, Loc);
      return;
    }

    case Instruction::PHI: {
      // Introduce a target for the phi. This breaks loops consistently.
      // Setting the location to I (and not after I) makes sure that this
      // particular internal Node is only "gotten" when handling Phis here.
      auto *PhiNode = getInternalNode(WG, Instrumentee, Instrumentee);
      Node->addRequirement(PhiNode);
      if (PhiNode->HasAllRequirements) {
        return;
      }
      PhiNode->HasAllRequirements = true;

      // The phi target requires witnesses of its incoming values.
      auto *PtrPhi = cast<PHINode>(Instrumentee);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);
        requireRecursively(PhiNode, InVal, &InBB->back());
      }
      return;
    }

    case Instruction::Select: {
      // A select target needs witnesses of both its arguments.
      auto *PtrSelect = cast<SelectInst>(Instrumentee);
      requireRecursively(Node, PtrSelect->getTrueValue(), Instrumentee);
      requireRecursively(Node, PtrSelect->getFalseValue(), Instrumentee);
      return;
    }

    case Instruction::GetElementPtr: {
      // A GEP target requires only the witness of its argument.
      auto *Operand =
          cast<GetElementPtrInst>(Instrumentee)->getPointerOperand();
      requireRecursively(Node, Operand, Instrumentee);
      return;
    }

    case Instruction::BitCast: {
      // A bitcast target requires only the witness of its argument.
      auto *Operand = cast<BitCastInst>(Instrumentee)->getOperand(0);
      requireRecursively(Node, Operand, Instrumentee);
      return;
    }

    case Instruction::ExtractValue: {
      auto *eVal = cast<ExtractValueInst>(Instrumentee);
      requireRecursively(Node, eVal->getAggregateOperand(), eVal);
      return;
    }

    case Instruction::InsertValue: {
      auto *iVal = cast<InsertValueInst>(Instrumentee);
      requireRecursively(Node, iVal->getAggregateOperand(), iVal);
      if (iVal->getInsertedValueOperand()->getType()->isPointerTy()) {
        requireRecursively(Node, iVal->getInsertedValueOperand(), iVal);
      }
      return;
    }

    case Instruction::ExtractElement:
    case Instruction::InsertElement:
    case Instruction::ShuffleVector:
      MemInstrumentError::report(
          "Vector instructions on pointers are unsupported. Found: ",
          Instrumentee);
    default:
      llvm_unreachable("Unknown instruction encountered.");
    }
  }

  // Get the location right at the beginning of the function. We want to place
  // witnesses for arguments, global values and constants here.
  auto *Fun = Target->getLocation()->getFunction();
  auto *EntryLoc = &(*Fun->getEntryBlock().getFirstInsertionPt());

  auto *toInstrument = Target->getInstrumentee();

  if (isa<Argument>(toInstrument)) {
    requireSource(Node, toInstrument, EntryLoc);
    return;
  }

  if (auto *C = dyn_cast<Constant>(toInstrument)) {

    if (C->getType()->isAggregateType()) {
      requireSource(Node, toInstrument, Target->getLocation());
      return;
    }

    if (isa<UndefValue>(toInstrument)) {
      requireSource(Node, toInstrument, Target->getLocation());
      return;
    }

    std::vector<Value *> Pointers;
    getPointerOperands(Pointers, C);
    for (auto *V : Pointers) {
      // Generate witnesses for globals and constants at the beginning of the
      // function.
      // TODO There might be better locations for this witness, maybe experiment
      // with some others to see what is best.
      requireSource(Node, V, EntryLoc);
    }
    return;
  }

  llvm_unreachable("Unexpected value operand encountered.");
}

void AfterInflowStrategy::createWitness(InstrumentationMechanism &IM,
                                        WitnessGraphNode *Node) const {

  auto &Target = Node->Target;

  if (Target->hasBoundWitnesses()) {
    // We already handled this node.
    return;
  }

  if (Target->needsNoBoundWitnesses()) {
    // No witness required.
    return;
  }

  if (Node->getRequiredNodes().size() == 0) {
    // We assume that this Node corresponds to a valid pointer, so we create a
    // new witness for it.
    IM.insertWitnesses(*(Target));
    return;
  }

  // In case this call invariant needs bound witnesses, insert all of them
  if (auto callIT = dyn_cast<CallInvariantIT>(Target)) {

    WitnessMap witnesses;
    for (auto requirement : Node->getRequiredNodes()) {

      // Create the required witness
      createWitness(IM, requirement);

      // Map the instrumentee to the ones required by the call
      auto requirementTarget = requirement->Target;
      auto witness = requirementTarget->getSingleBoundWitness();
      auto requirementInstrumentee = requirementTarget->getInstrumentee();

      for (auto &elem : callIT->getRequiredArgs()) {
        if (elem.second == requirementInstrumentee) {
          // Add the witness at the correct location in the witness map
          // Note that we need to iterate over all elements in case the same
          // instrumentee is required for multiple indices
          witnesses[elem.first] = witness;
        }
      }
      Target->setBoundWitnesses(witnesses);
    }
    return;
  }

  if (Node->getRequiredNodes().size() == 1) {
    // We just use the witness of the single requirement, nothing to combine.
    auto *Requirement = Node->getRequiredNodes()[0];
    createWitness(IM, Requirement);
    auto witnesses = Requirement->Target->getBoundWitnesses();
    // Extract value instructions shrink the witnesses from a vector to a single
    // witness
    if (Target->hasInstrumentee()) {
      auto instrumentee = Target->getInstrumentee();
      if (auto eValInst = dyn_cast<ExtractValueInst>(instrumentee)) {
        WitnessMap singleWit;
        auto indices = eValInst->getIndices();
        assert(indices.size() == 1);
        singleWit[0] = witnesses[indices[0]];
        witnesses = singleWit;
      }
    }

    if (NoShareBoundsOpt) {
      WitnessMap newWitnesses;
      for (auto &KV : witnesses) {
        auto newW = IM.getRelocatedClone(*KV.second, Target->getLocation());
        newWitnesses[KV.first] = newW;
      }
      Target->setBoundWitnesses(newWitnesses);
    } else {
      Target->setBoundWitnesses(witnesses);
    }
    return;
  }

  assert(Target->hasInstrumentee());
  Instruction *Instrumentee = dyn_cast<Instruction>(Target->getInstrumentee());
  assert(Instrumentee);

  if (InsertValueInst *iValInst = dyn_cast<InsertValueInst>(Instrumentee)) {
    assert(Node->getRequiredNodes().size() == 2);

    auto &reqNodes = Node->getRequiredNodes();
    for (auto *reqNode : reqNodes) {
      createWitness(IM, reqNode);
    }

    auto valOp = iValInst->getInsertedValueOperand();
    auto targetOne = reqNodes[0]->Target;
    auto targetTwo = reqNodes[1]->Target;

    // One of the two targets has to provide the bounds for the inserted value,
    // determine which one
    ITargetPtr valOpTarget =
        targetOne->getInstrumentee() == valOp ? targetOne : targetTwo;
    ITargetPtr aggrTarget =
        targetOne->getInstrumentee() == valOp ? targetTwo : targetOne;

    assert(valOpTarget->getInstrumentee() == valOp);

    // Consider only simple aggregates
    auto indices = iValInst->getIndices();
    assert(indices.size() == 1);
    auto index = indices[0];

    // Request all bounds of values in the aggregate known so far
    auto previousWitnesses = aggrTarget->getBoundWitnesses();

    // Update the bound witness for the value stored to the aggregate with this
    // instruction and store it to the current target.
    previousWitnesses[index] = valOpTarget->getSingleBoundWitness();
    Target->setBoundWitnesses(previousWitnesses);
    return;
  }

  if (auto *Phi = dyn_cast<PHINode>(Instrumentee)) {
    auto &ReqNodes = Node->getRequiredNodes();
    // Insert new phis that use the witnesses of the operands of the
    // instrumentee. This has to happen in two separate steps to break loops.
    assert(Node->getRequiredNodes().size() == Phi->getNumIncomingValues());

    SmallVector<unsigned, 1> indices;
    indices.push_back(0);
    if (Phi->getType()->isAggregateType()) {
      indices = InstrumentationMechanism::computePointerIndices(Phi->getType());
    }

    for (auto index : indices) {
      // Insert phis with no arguments.
      auto PhiWitness = IM.getWitnessPhi(Phi);
      Target->setBoundWitness(PhiWitness, index);
    }

    // Now, add the corresponding incoming values for the operands.
    unsigned int i = 0;
    for (auto *ReqNode : ReqNodes) {
      createWitness(IM, ReqNode);
      auto *BB = Phi->getIncomingBlock(i);
      for (auto &KV : ReqNode->Target->getBoundWitnesses()) {
        // Look up the corresponding phi
        auto PhiWitness = Target->getBoundWitness(KV.first);
        IM.addIncomingWitnessToPhi(PhiWitness, KV.second, BB);
      }
      i++;
    }
    return;
  }

  if (auto sel = dyn_cast<SelectInst>(Instrumentee)) {
    // Insert new selects that use the witnesses of the operands of the
    // instrumentee.
    assert(Node->getRequiredNodes().size() == 2);

    for (auto *ReqNode : Node->getRequiredNodes()) {
      createWitness(IM, ReqNode);
    }

    auto target1 = Node->getRequiredNodes()[0]->Target;
    auto target2 = Node->getRequiredNodes()[1]->Target;
    assert(target1->getBoundWitnesses().size() ==
           target2->getBoundWitnesses().size());
    for (const auto &KV : target1->getBoundWitnesses()) {
      auto wt1 = KV.second;
      auto wt2 = target2->getBoundWitness(KV.first);
      auto WSel = IM.getWitnessSelect(sel, wt1, wt2);
      Target->setBoundWitness(WSel, KV.first);
    }

    return;
  }

  // There should be no way to end up here.
  llvm_unreachable("Invalid Node");
}

// #define DEBUG_WG_SIMPLIFY 1

namespace {
void updateWitnessNode(std::map<WitnessGraphNode *, WitnessGraphNode *> &ReqMap,
                       WitnessGraphNode *N) {
  assert(N != nullptr);
  WitnessGraphNode *ValBefore = ReqMap.at(N);
  if (N->getRequiredNodes().size() == 0) {
    ReqMap.at(N) = N;
  } else {
    WitnessGraphNode *Reference = nullptr;
    bool AllSame = true;

    for (auto &N2 : N->getRequiredNodes()) {
      auto *Req = ReqMap.at(N2);
      if (Reference == nullptr) {
        Reference = Req;
      } else if (Req != nullptr && Reference != Req) {
        AllSame = false;
        break;
      }
    }

    if (N->Target->isValid() && N->Target->hasInstrumentee() &&
        isa<ExtractValueInst>(N->Target->getInstrumentee())) {
      Reference = N;
    }

    if (AllSame) {
      ReqMap.at(N) = Reference;
    } else {
      ReqMap.at(N) = N;
    }
  }

#ifdef DEBUG_WG_SIMPLIFY
  static int cnt = 0;
  std::stringstream ss;
  ss << "simplifyStep." << ++cnt << ".dot";
  N->Graph.dumpDotGraph(ss.str(), ReqMap);
#endif

  if (ReqMap.at(N) != ValBefore) {
    for (auto *other : N->getRequiringNodes()) {
      updateWitnessNode(ReqMap, other);
    }
  }
}

bool didNotChangeSinceWitness(std::set<WitnessGraphNode *> &Seen,
                              WitnessGraphNode *N) {
  if (Seen.find(N) != Seen.end()) {
    return true;
  }

  Seen.insert(N);

  // In case we reached the source for this pointer, we can assume that the
  // invariant holds.
  if (isa<SourceIT>(N->Target)) {
    return true;
  }

  if (auto GEP = dyn_cast<GetElementPtrInst>(N->Target->getLocation())) {
    if (!GEP->hasAllZeroIndices()) {
      return false;
    }
  }

  for (auto *req : N->getRequiredNodes()) {
    bool res = didNotChangeSinceWitness(Seen, req);
    if (!res) {
      return false;
    }
  }

  return true;
}
} // namespace

void AfterInflowStrategy::simplifyWitnessGraph(InstrumentationMechanism &IM,
                                               WitnessGraph &WG) const {

  if (!OptimizeWitnessGraph) {
    return;
  }

  if (!IM.invariantsAreChecks()) {
    MemInstrumentError::report("The instrumentation mechanism `" +
                               Twine(IM.getName()) +
                               "` does not support optimizing invariants the "
                               "same way as checks. Don't "
                               "use -mi-ais-optimize-witness-graph.");
  }

  // Check whether the value of the instrumentee is definitely the same as
  // when we extracted its witness. In this case, we can skip inbounds checks
  // for out-flowing pointers (as we assume the values to be valid anyway).
  for (auto *External : WG.getExternalNodes()) {
    auto &T = External->Target;
    if (T->isValid() && T->isInvariant()) {
      std::set<WitnessGraphNode *> Seen;
      if (didNotChangeSinceWitness(Seen, External)) {
        External->Target->invalidate();
        ++NumITargetsRemovedWGSimplify;
      }
    }
  }

  // Eliminate nodes that correspond to superfluous PHIs, i.e. that always have
  // equal operands.
  std::map<WitnessGraphNode *, WitnessGraphNode *> ReqMap;
  std::vector<WitnessGraphNode *> Roots;

  WG.map([&](WitnessGraphNode *N) {
    ReqMap.insert(std::make_pair(N, nullptr));
    if (N->getRequiredNodes().size() == 0) {
      Roots.push_back(N);
    }
  });

  for (auto *N : Roots) {
    updateWitnessNode(ReqMap, N);
  }

  WG.map([&](WitnessGraphNode *N) {
    auto *Req = ReqMap.at(N);
    assert(Req != nullptr);

    if (Req != N) {
      N->clearRequirements();
      N->addRequirement(Req);
    }
  });
  WG.removeDeadNodes();

#ifdef DEBUG_WG_SIMPLIFY
  bool StillHasPhis = false;
  WG.map([&](WitnessGraphNode *N) {
    if (N->getRequiredNodes().size() > 1) {
      StillHasPhis = true;
    }
  });

  if (StillHasPhis) {
    static int cnt = 0;
    std::stringstream ss;
    ss << "phis." << ++cnt << ".dot";
    WG.dumpDotGraph(ss.str());
  }
#endif
}
