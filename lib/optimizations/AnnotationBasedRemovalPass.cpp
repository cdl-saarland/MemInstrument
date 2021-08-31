//===- AnnotationBasedRemovalPass.cpp - Remove Targets by Annotation ------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "meminstrument/optimizations/AnnotationBasedRemovalPass.h"

#include "meminstrument/optimizations/PerfData.h"
#include "meminstrument/pass/Util.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace meminstrument;

cl::opt<std::string> RemovalAnnotationString(
    "mi-opt-annotation-marker-string",
    cl::desc(
        "annotation that marks instructions for which no check is required"),
    cl::init("nosanitize"));

STATISTIC(AnnotationCheckRemoved, "The # checks filtered by annotation");

//===--------------------------- ModulePass -------------------------------===//

char AnnotationBasedRemovalPass::ID = 0;

AnnotationBasedRemovalPass::AnnotationBasedRemovalPass() : ModulePass(ID) {}

bool AnnotationBasedRemovalPass::runOnModule(Module &module) {
  LLVM_DEBUG(dbgs() << "Running Annotation Based Removal Pass\n";);
  return false;
}

void AnnotationBasedRemovalPass::getAnalysisUsage(
    AnalysisUsage &analysisUsage) const {
  analysisUsage.setPreservesAll();
}

void AnnotationBasedRemovalPass::print(raw_ostream &stream,
                                       const Module *module) const {
  stream << "Annotation Based Removal Pass on `" << module->getName()
         << "`\n\n";
}

//===--------------------- OptimizationInterface --------------------------===//

void AnnotationBasedRemovalPass::updateITargetsForFunction(
    MemInstrumentPass &, ITargetVector &targets, Function &) {
  for (auto &target : targets) {

    if (!target->isValid()) {
      continue;
    }

    auto *loc = target->getLocation();
    if (loc->getMetadata(RemovalAnnotationString) && target->isCheck()) {
      ++AnnotationCheckRemoved;
      target->invalidate();
    }
  }
}
