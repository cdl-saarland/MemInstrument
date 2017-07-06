//===--------------- meminstrument/ITarget.h -- TODO doku ------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_ITARGET_H
#define MEMINSTRUMENT_ITARGET_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

namespace meminstrument {

/// TODO document
///
// TODO maybe make ITarget abstract and have one subclass for this and one for
// certain expressions
struct ITarget {
  /// value that should be checked by the instrumentation
  llvm::Value* instrumentee;

  /// instruction before which the instrumentee should be checked
  llvm::Instruction* location;

  /// access size in bytes that should be checked starting from the instrumentee
  size_t accessSize;

  /// indicator whether instrumentee should be checked against its upper bound
  bool checkUpperBound;

  /// indicator whether instrumentee should be checked against its lower bound
  bool checkLowerBound;

  /// indicator whether temporal safety of instrumentee should be checked
  bool checkTemporal;

  ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub, bool lb,
      bool temp);
  // ITarget(llvm::Value* i, llvm::Instruction* loc, bool ub, bool lb, bool temp);
  ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz, bool ub, bool lb);
  // ITarget(llvm::Value* i, llvm::Instruction* loc, bool ub, bool lb);
  ITarget(llvm::Value* i, llvm::Instruction* loc, size_t sz);
  // ITarget(llvm::Value* i, llvm::Instruction* loc);

  friend llvm::raw_ostream& operator<< (llvm::raw_ostream& stream, const ITarget& it) {
    stream << "<" << it.instrumentee->getName().str() << ", "
      << it.location->getName().str() << ", " << it.accessSize << " bytes, ";
    if (it.checkUpperBound) {
      stream << "u";
    }
    if (it.checkLowerBound) {
      stream << "l";
    }
    if (it.checkTemporal) {
      stream << "t";
    }
    stream << ">";
    return stream;
  }
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_ITARGET_H
