//===------------ ITarget.cpp -- Memory Instrumentation -------------------===//
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

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub,
    bool lb, bool temp) : instrumentee(i), location(loc), accessSize(sz),
    checkUpperBound(ub), checkLowerBound(lb), checkTemporal(temp) {
}

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub,
    bool lb) : ITarget(i, loc, sz, ub, lb, false) {
}

ITarget::ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz)
  : ITarget(i, loc, sz, true, true) {
}

llvm::raw_ostream& meminstrument::operator<< (llvm::raw_ostream& stream, const ITarget& it) {
    std::string loc_name = it.location->getName().str();
    if (loc_name.empty()) {
      switch (it.location->getOpcode()) {
        case llvm::Instruction::Store:
          loc_name = "[store]";
        break;

        case llvm::Instruction::Ret:
          loc_name = "[ret]";
        break;

        default:
          loc_name = "[unnamed]";
      }
    }

    stream << "<" << it.instrumentee->getName() << ", ";
    auto* bb = it.location->getParent();
    stream << bb->getParent()->getName() << "::" << bb->getName() << "::"
      << loc_name << ", ";
    stream << it.accessSize << " bytes, ";
    if (it.checkUpperBound) {
      stream << "u";
    } else {
      stream << "_";
    }
    if (it.checkLowerBound) {
      stream << "l";
    } else {
      stream << "_";
    }
    if (it.checkTemporal) {
      stream << "t";
    } else {
      stream << "_";
    }
    stream << ">";
    return stream;
  }
