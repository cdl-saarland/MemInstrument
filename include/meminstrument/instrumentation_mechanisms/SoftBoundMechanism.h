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

  virtual void insertWitness(ITarget &) const override;

  virtual void relocCloneWitness(Witness &, ITarget &) const override;

  virtual void insertCheck(ITarget &) const override;

  virtual void materializeBounds(ITarget &) override;

  virtual auto getFailFunction() const -> llvm::Value * override;

  virtual auto getExtCheckCounterFunction() const -> llvm::Value * override;

  virtual auto getVerboseFailFunction() const -> llvm::Value * override;

  virtual auto insertWitnessPhi(ITarget &) const
      -> std::shared_ptr<Witness> override;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const override;

  virtual auto insertWitnessSelect(ITarget &Target,
                                   std::shared_ptr<Witness> &TrueWitness,
                                   std::shared_ptr<Witness> &FalseWitness) const
      -> std::shared_ptr<Witness> override;

  virtual void initialize(llvm::Module &) override;

  virtual auto getName() const -> const char * override;
};

//===----------------------------------------------------------------------===//
// TODO: Different file would be nicer

class SoftBoundWitness : public Witness {

public:
  virtual auto getLowerBound() const -> llvm::Value * override;

  virtual auto getUpperBound() const -> llvm::Value * override;

  static bool classof(const Witness *W);
};

} // namespace meminstrument

#endif