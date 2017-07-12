//===------------ ITarget.cpp -- MemSafety Instrumentation ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITarget.h"

using namespace llvm;
using namespace meminstrument;

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag, bool CheckTemporalFlag)
    : Instrumentee(Instrumentee), Location(Location), AccessSize(AccessSize),
      CheckUpperBoundFlag(CheckUpperBoundFlag),
      CheckLowerBoundFlag(CheckLowerBoundFlag),
      CheckTemporalFlag(CheckTemporalFlag) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize, bool CheckUpperBoundFlag,
                 bool CheckLowerBoundFlag)
    : ITarget(Instrumentee, Location, AccessSize, CheckUpperBoundFlag,
              CheckLowerBoundFlag, false) {}

ITarget::ITarget(llvm::Value *Instrumentee, llvm::Instruction *Location,
                 size_t AccessSize)
    : ITarget(Instrumentee, Location, AccessSize, true, true) {}

llvm::raw_ostream &meminstrument::operator<<(llvm::raw_ostream &Stream,
                                             const ITarget &IT) {
  std::string LocName = IT.Location->getName().str();
  if (LocName.empty()) {
    switch (IT.Location->getOpcode()) {
    case llvm::Instruction::Store:
      LocName = "[store]";
      break;

    case llvm::Instruction::Ret:
      LocName = "[ret]";
      break;

    default:
      LocName = "[unnamed]";
    }
  }

  Stream << "<" << IT.Instrumentee->getName() << ", ";
  auto *BB = IT.Location->getParent();
  Stream << BB->getParent()->getName() << "::" << BB->getName()
         << "::" << LocName << ", ";
  Stream << IT.AccessSize << " bytes, ";
  if (IT.CheckUpperBoundFlag) {
    Stream << "u";
  } else {
    Stream << "_";
  }
  if (IT.CheckLowerBoundFlag) {
    Stream << "l";
  } else {
    Stream << "_";
  }
  if (IT.CheckTemporalFlag) {
    Stream << "t";
  } else {
    Stream << "_";
  }
  Stream << ">";
  return Stream;
}
