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

#include "meminstrument/instrumentation_mechanisms/softbound/RunTimeHandles.h"

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
  /// Handles for SoftBound run-time function declarations for which calls are
  /// inserted during instrumentation
  softbound::RunTimeHandles handles;

  /// The llvm context
  llvm::LLVMContext *context;

  /// Insert the declarations for SoftBound metadata propagation functions
  void insertFunDecls(llvm::Module &);

  /// Rename the main function, such that the run-time main will be executed
  /// instead, which then calls the renamed main
  void renameMain(llvm::Module &);

  /// Take care of running the SoftBound setup in the very beginning (including
  /// globally initialized variables)
  void setUpGlobals(llvm::Module &);

  /// Given a global initializer, insert pointer bounds in book
  /// keeping data structures
  auto handleInitializer(llvm::Constant *, llvm::IRBuilder<> &,
                         llvm::Value &topLevel,
                         std::vector<llvm::Value *> indices) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Prepare the shadow stack for a call, i.e. allocate space before the call
  /// and deallocate it afterwards.
  void handleCallInvariant(CallInvariantIT &) const;

  /// Add bit casts if the types are not yet those that base and bound should
  /// have.
  auto addBitCasts(llvm::IRBuilder<>, llvm::Value *base,
                   llvm::Value *bound) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Create bounds information for the given value, by creating a GetElementPtr
  /// to and one past the element.
  auto getBoundsConst(llvm::Constant *) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Get bound information for functions (this is used when loading a function
  /// pointer from memory and the result is called)
  auto getBoundsForFun(llvm::Value *) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Create base and bound values that are null pointer.
  /// Objects with nullptr bounds are not valid to access, but up to a load or
  /// store operation (in which case they lead to an error), they do not harm
  /// the program execution.
  auto getNullPtrBounds() const -> std::pair<llvm::Value *, llvm::Value *>;

  /// Compute bounds for a constant
  auto getBoundsForWitness(llvm::Constant *) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Insert a load of the base and bound metadata for ptr.
  /// The IR builder should provide the correct load location already.
  auto insertMetadataLoad(llvm::IRBuilder<> &, llvm::Value *ptr) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Insert a store of the given base and bound for the given ptr.
  /// The IR builder should provide the correct store location already.
  void insertMetadataStore(llvm::IRBuilder<> &, llvm::Value *ptr,
                           llvm::Value *base, llvm::Value *bound) const;

  /// Load base and bound information from the shadow stack for the given
  /// index.
  auto insertShadowStackLoad(llvm::IRBuilder<> &, int index) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Store base and bound information on the shadow stack for the given
  /// target.
  void insertShadowStackStore(llvm::IRBuilder<> &, ITarget &) const;

  /// The shadow stack stores bound information for pointers handed over to
  /// functions and pointers returned from functions.
  /// The location on the stack is build up follows:
  ///   retptr, arg0 bounds, arg1 boundes, ..., argn bounds
  /// As there is nothing to put on the stack for non-pointer values, the
  /// location on the stack is not the argument number (+1), but rather the
  /// number of pointers before the requested argument bounds.
  /// To avoid flaws in the shadow stack location computation, this functions
  /// provides a consistent look up of the correct index.
  auto computeShadowStackLocation(const llvm::Value *) const -> int;

  /// Currently vectors of pointers are not handled properly. This function
  /// identifies unproblematic vectors.
  bool isSimpleVectorTy(const llvm::VectorType *) const;

  /// Check if the instruction is supported.
  bool isUnsupportedInstruction(unsigned opCode) const;

  /// Returns true iff the constant contains an integer to pointer cast.
  bool containsUnsupportedOp(const llvm::Constant *) const;

  /// Check if this module contains any unsupported constructs (e.g. exception
  /// handling)
  void checkModule(llvm::Module &);
};

} // namespace meminstrument

#endif
