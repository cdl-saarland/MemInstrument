
/*
Value *BeforeOutflowPolicy::createWitness(Value *Instrumentee) {
  if (auto *I = dyn_cast<Instruction>(Instrumentee)) {
    IRBuilder<> Builder(I->getNextNode());
    // insert code after the source instruction
    switch (I->getOpcode()) {
    case Instruction::Alloca:
    case Instruction::Call:
    case Instruction::Load:
      // the assumption is that the validity of these has already been checked
      return insertGetBoundWitness(Builder, I);

    case Instruction::PHI: {
      auto *PtrPhi = cast<PHINode>(I);
      unsigned NumOperands = PtrPhi->getNumIncomingValues();
      auto *NewType = getWitnessType();
      auto *NewPhi = Builder.CreatePHI(NewType, NumOperands); // TODO give name
      for (unsigned i = 0; i < NumOperands; ++i) {
        auto *InVal = PtrPhi->getIncomingValue(i);
        auto *InBB = PtrPhi->getIncomingBlock(i);
        auto *NewInVal = createWitness(InVal);
        NewPhi->addIncoming(NewInVal, InBB);
      }
      return NewPhi;
    }

    case Instruction::Select: {
      auto *PtrSelect = cast<SelectInst>(I);
      auto *Condition = PtrSelect->getCondition();

      auto *TrueVal = createWitness(PtrSelect->getTrueValue());
      auto *FalseVal = createWitness(PtrSelect->getFalseValue());

      Builder.CreateSelect(Condition, TrueVal, FalseVal); // TODO give name
    }

    case Instruction::GetElementPtr: {
      // pointer arithmetic doesn't change the pointer bounds
      auto *Operand = cast<GetElementPtrInst>(I);
      return createWitness(Operand->getPointerOperand());
    }

    case Instruction::IntToPtr: {
      // FIXME is this what we want?
      return insertGetBoundWitness(Builder, I);
    }

    case Instruction::BitCast: {
      // bit casts don't change the pointer bounds
      auto *Operand = cast<BitCastInst>(I);
      return createWitness(Operand->getOperand(0));
    }

    default:
      llvm_unreachable("Unsupported instruction!");
    }

  } else if (isa<Argument>(Instrumentee)) {
    // should have already been checked at the call site
    return insertGetBoundWitness(Builder, Instrumentee);
  } else if (isa<GlobalValue>(Instrumentee)) {
    llvm_unreachable("Global Values are not yet supported!"); // FIXME
    // return insertGetBoundWitness(Builder, Instrumentee);
  } else {
    // TODO constexpr
    llvm_unreachable("Unsupported value operand!");
  }
}

void BeforeOutflowPolicy::instrumentFunction(llvm::Function &Func,
                                             std::vector<ITarget> &Targets) {
  for (auto &BB : Func) {
    for (auto &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        handleAlloca(AI);
      }
    }
  }
  // TODO when to change allocas?

  for (const auto &IT : Targets) {
    if (IT.CheckTemporalFlag)
      llvm_unreachable("Temporal checks are not supported by this policy!");

    if (!(IT.CheckLowerBoundFlag || IT.CheckUpperBoundFlag))
      continue; // nothing to do!

    auto *Witness = createWitness(IT.Instrumentee);
    IRBuilder<> Builder(IT.Location);

    if (IT.CheckLowerBoundFlag && IT.CheckUpperBoundFlag) {
      insertCheckBoundWitness(Builder, IT.Instrumentee, Witness);
    } else if (IT.CheckUpperBoundFlag) {
      insertCheckBoundWitnessUpper(Builder, IT.Instrumentee, Witness);
    } else {
      insertCheckBoundWitnessLower(Builder, IT.Instrumentee, Witness);
    }
  }
}
*/
