//===---- meminstrument/Witness.h -- MemSafety Instrumentation  -*- C++ -*-===//
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

#ifndef MEMINSTRUMENT_WITNESS_H
#define MEMINSTRUMENT_WITNESS_H

#include "llvm/IR/Value.h"

namespace meminstrument {

/// TODO document
struct Witness {
  virtual llvm::Value* getLowerBound(void) const = 0;
  virtual llvm::Value* getUpperBound(void) const = 0;

  virtual ~Witness(void) {}
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_WITNESS_H
