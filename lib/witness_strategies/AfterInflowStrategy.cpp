//===---------------------------------------------------------------------===///
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#include "meminstrument/witness_strategies/AfterInflowStrategy.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

#include <set>
#include <sstream>

#include "meminstrument/pass/Util.h"

using namespace meminstrument;
using namespace llvm;

namespace {
STATISTIC(NumITargetsRemovedWGSimplify, "The # of inbounds targets discarded "
                                        "because of witness graph information");

STATISTIC(NumPtrVectorInstructions, "The # of vector operations on pointers "
                                    "encountered");

STATISTIC(NumUnsupportedConstExprs, "The # of unsupported constant expressions "
                                    "encountered");

STATISTIC(NumUnsupportedConstVals, "The # of unsupported constant values other "
                                    "than constant expressions encountered");

void getPointerOperands(std::vector<Value *> &Results, llvm::Constant *C) {
  assert(C->getType()->isPointerTy() && "getPointerOperands() called for non-pointer constant!");

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
    case Instruction::Select:
      getPointerOperands(Results, CE->getOperand(1)); // true operand
      getPointerOperands(Results, CE->getOperand(2)); // false operand
      llvm_unreachable("Select constant expression!");
      break;
    case Instruction::BitCast:
      getPointerOperands(Results, CE->getOperand(0)); // pointer argument
      break;
    default:
      ++NumUnsupportedConstExprs;
      errs() << "Unsupported constant expression:\n" << *CE << "\n\n";
      llvm_unreachable("Unsupported constant expression!");
    }

    return;
  }

  ++NumUnsupportedConstVals;
  errs() << "Unsupported constant value:\n" << *C << "\n\n";
  llvm_unreachable("Unsupported constant value!");
}
} // namespace

void AfterInflowStrategy::addRequired(WitnessGraphNode *Node) const {
  if (Node->HasAllRequirements) {
    return;
  }

  Node->HasAllRequirements = true;

  auto &WG = Node->Graph;

  auto &Target = Node->Target;
  if (auto *I = dyn_cast<Instruction>(Target->Instrumentee)) {
    switch (I->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::LandingPad:
    case Instruction::ExtractValue:
    case Instruction::Load:
    case Instruction::IntToPtr: {
      // Introduce a target without requirements for these values. We assume
      // that these are valid pointers.
      requireSource(Node, I, I->getNextNode());
      return;
    }
    case Instruction::Invoke: {
      // We cannot insert witnesses after invokes as these are terminators.
      // Therefore, use the first non-phi instruction of the 'normal' successor.
      auto *II = dyn_cast<InvokeInst>(I);
      auto *Loc = II->getNormalDest()->getFirstNonPHI();
      requireSource(Node, I, Loc);
      return;
    }

    case Instruction::PHI: {
      // Introduce a target for the phi. This breaks loops consistently.
      // Setting the location to I (and not after I) makes sure that this
      // particular internal Node is only "gotten" when handling Phis here.
      auto *PhiNode = getInternalNode(WG, I, I);
      Node->addRequirement(PhiNode);
      if (PhiNode->HasAllRequirements) {
        return;
      }
      PhiNode->HasAllRequirements = true;

      // The phi target requires witnesses of its incoming values.
      auto *PtrPhi = cast<PHINode>(I);
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
      auto *PtrSelect = cast<SelectInst>(I);
      requireRecursively(Node, PtrSelect->getTrueValue(), I);
      requireRecursively(Node, PtrSelect->getFalseValue(), I);
      return;
    }

    case Instruction::GetElementPtr: {
      // A GEP target requires only the witness of its argument.
      auto *Operand = cast<GetElementPtrInst>(I)->getPointerOperand();
      requireRecursively(Node, Operand, I);
      return;
    }

    case Instruction::BitCast: {
      // A bitcast target requires only the witness of its argument.
      auto *Operand = cast<BitCastInst>(I)->getOperand(0);
      requireRecursively(Node, Operand, I);
      return;
    }

    case Instruction::ExtractElement:
    case Instruction::InsertElement:
    case Instruction::ShuffleVector:
      ++NumPtrVectorInstructions; // fallthrough
    default:
      errs() << "Unsupported instruction:\n" << *I << "\n\n";
      llvm_unreachable("Unsupported instruction!");
    }
  }

  if (isa<Argument>(Target->Instrumentee)) {
    // Generate witnesses for arguments right when we need them. This might be
    // inefficient.
    return;
  }

  if (auto *C = dyn_cast<Constant>(Target->Instrumentee)) {
    std::vector<Value *> Pointers;
    getPointerOperands(Pointers, C);
    for (auto *V : Pointers) {
      // Generate witnesses for globals and constants right when we need them.
      // This might be inefficient.
      requireSource(Node, V, Target->Location);
      // TODO this location might be sub-optimal
      // TODO do we really want to have witnesses for constant null pointers?
    }
    return;
  }

  errs() << "Unsupported value operand:\n" << *Target->Instrumentee << "\n\n";
  llvm_unreachable("Unsupported value operand!");
}

void AfterInflowStrategy::createWitness(InstrumentationMechanism &IM,
                                        WitnessGraphNode *Node) const {
  if (Node->Target->hasWitness()) {
    // We already handled this node.
    return;
  }

  if (Node->getRequiredNodes().size() == 0) {
    // We assume that this Node corresponds to a valid pointer, so we create a
    // new witness for it.
    IM.insertWitness(*(Node->Target));
    return;
  }

  if (Node->getRequiredNodes().size() == 1) {
    // We just use the witness of the single requirement, nothing to combine.
    auto *Requirement = Node->getRequiredNodes()[0];
    createWitness(IM, Requirement);
    Node->Target->BoundWitness = Requirement->Target->BoundWitness;
    return;
  }

  auto *Instrumentee = Node->Target->Instrumentee;
  if (auto *Phi = dyn_cast<PHINode>(Instrumentee)) {
    // Insert new phis that use the witnesses of the operands of the
    // instrumentee. This has to happen in two separate steps to break loops.
    assert(Node->getRequiredNodes().size() == Phi->getNumIncomingValues());

    // First insert phis with no arguments.
    auto PhiWitness = IM.insertWitnessPhi(*(Node->Target));

    // Now, add the corresponding incoming values for the operands.
    unsigned int i = 0;
    for (auto *ReqNode : Node->getRequiredNodes()) {
      createWitness(IM, ReqNode);
      auto *BB = Phi->getIncomingBlock(i);
      IM.addIncomingWitnessToPhi(PhiWitness, ReqNode->Target->BoundWitness, BB);
      i++;
    }
    return;
  }

  if (isa<SelectInst>(Instrumentee)) {
    // Insert new selects that use the witnesses of the operands of the
    // instrumentee.
    assert(Node->getRequiredNodes().size() == 2);

    for (auto *ReqNode : Node->getRequiredNodes()) {
      createWitness(IM, ReqNode);
    }

    IM.insertWitnessSelect(*(Node->Target),
                           Node->getRequiredNodes()[0]->Target->BoundWitness,
                           Node->getRequiredNodes()[1]->Target->BoundWitness);

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

  if (auto GEP = dyn_cast<GetElementPtrInst>(N->Target->Location)) {
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

void AfterInflowStrategy::simplifyWitnessGraph(WitnessGraph &WG) const {

  // Check whether the value of the instrumentee is definitely the same as
  // when we extracted its witness. In this case, we can skip inbounds checks
  // for out-flowing pointers (as we assume the values to be valid anyway).
  for (auto *External : WG.getExternalNodes()) {
    auto &T = External->Target;
    if (T->isValid() && !(T->CheckUpperBoundFlag || T->CheckLowerBoundFlag)) {
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
