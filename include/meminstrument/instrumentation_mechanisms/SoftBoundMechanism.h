//===----- meminstrument/SoftBoundMechanism.h - SoftBound -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// SoftBound memory safety instrumentation originally developed by Santosh
/// Nagarakatte, Jianzhou Zhao, Milo M. K. Martin and Steve Zdancewic.
///
/// Publications, links to the original implementation etc. can be found here:
/// https://www.cs.rutgers.edu/~santosh.nagarakatte/softbound/
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUNDMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_SOFTBOUNDMECHANISM_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"

namespace meminstrument {

class SoftBoundMechanism : public InstrumentationMechanism {
public:
  SoftBoundMechanism(GlobalConfig &);

  virtual void initialize(llvm::Module &) override;

  virtual void insertWitness(ITarget &) const override;

  virtual void relocCloneWitness(Witness &, ITarget &) const override;

  virtual auto insertWitnessPhi(ITarget &) const
      -> std::shared_ptr<Witness> override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual auto insertWitnessSelect(ITarget &Target,
                                   std::shared_ptr<Witness> &TrueWitness,
                                   std::shared_ptr<Witness> &FalseWitness) const
      -> std::shared_ptr<Witness> override;

  virtual void materializeBounds(ITarget &) override;

  virtual void insertCheck(ITarget &) const override;

  virtual auto getFailFunction() const -> llvm::Value * override;

  virtual auto getVerboseFailFunction() const -> llvm::Value * override;

  virtual auto getExtCheckCounterFunction() const -> llvm::Value * override;

  virtual auto getName() const -> const char * override;

private:
  /// Insert the declarations for SoftBound metadata propagation functions
  void insertFunDecls(llvm::Module &);
};

} // namespace meminstrument

#endif
