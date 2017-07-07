//===----- InstrumentationMechanism.cpp -- Memory Instrumentation ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/InstrumentationMechanism.h"

#include "llvm/Support/Debug.h"
#include "llvm/IR/Instructions.h"

#define DEBUG_TYPE "meminstrument"

using namespace llvm;
using namespace meminstrument;

