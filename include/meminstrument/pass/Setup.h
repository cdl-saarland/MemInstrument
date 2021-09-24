//===- meminstrument/Setup.h - Module Setup Steps ---------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Provides initial transformations required for all instrumentation
/// mechanisms.
///
//===----------------------------------------------------------------------===//
#ifndef MEMINSTRUMENT_PASS_SETUP_H
#define MEMINSTRUMENT_PASS_SETUP_H

#include "llvm/IR/Module.h"

namespace meminstrument {

/// Make some initial transformations that are required by all instrumentations.
/// * Transforms function with byval arguments
/// * Instructions in the code that are generated as bookkeeping for varargs are
/// labeled as such
/// * If certain functions should be ignored (given in a file as command line
/// argument), mark them such that they are no instrumented later on
void prepareModule(llvm::Module &);

} // namespace meminstrument

#endif
