//===--------- ITargetProvider.cpp -- MemSafety Instrumentation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===///
/// \file TODO doku
//===----------------------------------------------------------------------===//

#include "meminstrument/ITargetProvider.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "meminstrument"

using namespace meminstrument;
using namespace llvm;

ITargetProvider::ITargetProvider(void) : TargetMap(nullptr) {}

void ITargetProvider::initializeEmpty(void) {
  TargetMap = std::make_shared<MapType>();
}

void ITargetProvider::connectToProvider(ITargetProvider *Provider) {
  TargetMap = Provider->TargetMap;
}

std::vector<ITarget> &
ITargetProvider::getITargetsForFunction(llvm::Function *F) {
  TargetMap->lookup(F);
  return (*TargetMap)[F];
}

void ITargetProvider::addITarget(const ITarget &Target) {
  auto *F = Target.Location->getParent()->getParent();
  TargetMap->lookup(F);
  (*TargetMap)[F].push_back(Target);
}
