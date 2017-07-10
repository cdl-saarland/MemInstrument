//===--- RegisterPasses.cpp - Add the required passes to default passes  --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// Registration of meminstrument pass in the tool chain.
///
//===----------------------------------------------------------------------===//

#include "meminstrument/GatherITargetsPass.h"
#include "meminstrument/MemInstrumentPass.h"
#include "llvm/IR/PassManager.h"

using namespace meminstrument;
using namespace llvm;

namespace meminstrument {
static RegisterPass<MemInstrumentPass>
    RegisterMemInstrumentPass("meminstrument", "MemInstrument",
                              false,  // CFGOnly
                              false); // isAnalysis

static RegisterPass<GatherITargetsPass>
    RegisterGatherITargetsPass("gatheritargets", "GatherITargets",
                               true,  // CFGOnly
                               true); // isAnalysis
}
