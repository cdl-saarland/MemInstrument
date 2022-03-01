//===- meminstrument/WitnessStrategy.h - Strategy Interface -----*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The witness strategy is an interface to define how the witness graph is
/// built.
/// The strategy determines at which locations code is placed to derive or
/// propagate witness information (e.g. pointer bounds).
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_WITNESS_STRATEGIES_WITNESSSTRATEGY_H
#define MEMINSTRUMENT_WITNESS_STRATEGIES_WITNESSSTRATEGY_H

#include "meminstrument/instrumentation_mechanisms/InstrumentationMechanism.h"
#include "meminstrument/pass/ITarget.h"
#include "meminstrument/pass/WitnessGraph.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace meminstrument {

struct WitnessGraphNode;

class WitnessGraph;

/// Subclasses of this abstract class describe how Witnesses are propagated
/// through a function for an instrumentation. This includes finding the
/// relevant locations for code that involves Witnesses and optional
/// optimizations to the propagation of Witnesses.
/// This class is closely connected to the WitnessGraph, which is constructed
/// according to the WitnessStrategy.
class WitnessStrategy {
public:
  virtual ~WitnessStrategy(void) {}

  /// Add the necessary WitnessGraphNodes for Witnesses for Node. This is
  /// likely calls requireRecursively(), which adds witnesses for all dataflow
  /// predecessors of Node by calling addRequired on them.
  virtual void addRequired(WitnessGraphNode *Node) const = 0;

  /// Optional method to simplify a WitnessGraph once constructed, e.g. by
  /// removing superfluous phis.
  virtual void simplifyWitnessGraph(InstrumentationMechanism &IM,
                                    WitnessGraph &WG) const {}

  /// Use the InstrumentationMechanism to materialize the witness for Node.
  virtual void createWitness(InstrumentationMechanism &IM,
                             WitnessGraphNode *Node) const = 0;

  /// Returns the name of the witness strategy for printing and easy
  /// recognition.
  virtual const char *getName(void) const = 0;

protected:
  /// Helper function that adds a new internal node for Req at Loc as a
  /// requirement for Node and calls addRequired() on the new node.
  /// Intended to be called by an implementation of addRequired().
  void requireRecursively(WitnessGraphNode *Node, llvm::Value *Req,
                          llvm::Instruction *Loc) const;

  /// Helper function to add nodes that generate witnesses rather than
  /// propagating them.
  void requireSource(WitnessGraphNode *Node, llvm::Value *Req,
                     llvm::Instruction *Loc) const;

  /// Helper function to create an internal node with an appropriate ITarget
  /// in the WitnessGraph.
  static WitnessGraphNode *getInternalNode(WitnessGraph &WG,
                                           llvm::Value *Instrumentee,
                                           llvm::Instruction *Location);

  /// Helper function to create a source node with an appropriate ITarget
  /// in the WitnessGraph.
  static WitnessGraphNode *getSourceNode(WitnessGraph &WG,
                                         llvm::Value *Instrumentee,
                                         llvm::Instruction *Location);
};
} // namespace meminstrument

#endif
