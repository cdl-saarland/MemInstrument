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

  virtual bool skipInstrumentation(llvm::Module &) const;

  virtual auto getFailFunction() const -> llvm::Value * override;

  virtual auto getExtCheckCounterFunction() const -> llvm::Value * override;

  virtual auto getName() const -> const char * override;

private:
  /// Handles for SoftBound run-time function declarations for which calls are
  /// inserted during instrumentation
  softbound::RunTimeHandles handles;

  /// Allocas to store loaded metadata in
  std::map<llvm::Function *, std::pair<llvm::AllocaInst *, llvm::AllocaInst *>>
      metadataAllocs;

  /// The llvm context
  llvm::LLVMContext *context;

  /// The data layout
  const llvm::DataLayout *DL;

  /// Replace all functions for which SoftBound provides a wrapper with the
  /// wrapped version (affects calls and declarations only)
  void replaceWrappedFunction(llvm::Module &) const;

  /// The wrappers introduced by SoftBound do not have the same properties as
  /// the original standard library functions, we need to update the
  /// call attributes accordingly.
  void updateCallAttributesForWrappedFunctions(llvm::Module &) const;

  /// Update the attribute of the given function or call according to the
  /// behavior of the wrapper.
  auto updateNotPreservedAttributes(const llvm::AttributeList &,
                                    int numArgs) const -> llvm::AttributeList;

  /// Insert the declarations for SoftBound metadata propagation functions and
  /// library function wrappers
  void insertFunDecls(llvm::Module &);

  /// Insert allocas to store loaded metadata in
  void insertMetadataAllocs(llvm::Module &);

  /// Rename the main function, such that the run-time main will be executed
  /// instead, which then calls the renamed main
  void renameMain(llvm::Module &) const;

  /// Transform all functions that have byval arguments to functions without
  /// byval arguments (allocate memory for them at every call site).
  void transformByValFunctions(llvm::Module &) const;

  /// Transform the given call byval arguments (if any) to hand over a pointer
  /// to the call site allocated stuff.
  void transformCallByValArgs(llvm::CallBase &, llvm::IRBuilder<> &) const;

  /// Take care of running the SoftBound setup in the very beginning (including
  /// globally initialized variables)
  void setUpGlobals(llvm::Module &) const;

  /// Given a global initializer, insert pointer bounds in book
  /// keeping data structures
  auto handleInitializer(llvm::Constant *, llvm::IRBuilder<> &,
                         llvm::Value &topLevel,
                         std::vector<llvm::Value *> indices) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Handles invariant targets that we use for metadata propagation. It
  /// produces stores to propagate bounds and inserts allocations for metadata
  /// data structures if necessary.
  void handleInvariant(const InvariantIT &) const;

  /// Take care of a call invariant target. This will rename wrapped functions,
  /// allocate and deallocate shadow stack space and insert further code for
  /// intrinsic calls if necessary.
  void handleCallInvariant(const CallInvariantIT &) const;

  /// Take care of shadow stack allocation and deallocation for the given call.
  void handleShadowStackAllocation(llvm::CallBase *) const;

  /// Insert metadata calls for invariants of intrinsic function if necessary.
  void handleIntrinsicInvariant(llvm::IntrinsicInst *) const;

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

  /// Return the bounds for a pointer constructed from an integer.
  /// The resulting bounds depend on the policy on how to handle them, which is
  /// set by a command line flag.
  auto getBoundsForIntToPtrCast() const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Return the bounds for a zero sized array.
  /// The resulting bounds depend on the policy on how to handle them, which is
  /// set by a command line flag.
  auto getBoundsForZeroSizedArray(llvm::GlobalVariable *) const
      -> std::pair<llvm::Value *, llvm::Value *>;

  /// Create base and bound values that are null pointer.
  /// Objects with nullptr bounds are not valid to access, but up to a load or
  /// store operation (in which case they lead to an error), they do not harm
  /// the program execution.
  auto getNullPtrBounds() const -> std::pair<llvm::Value *, llvm::Value *>;

  /// Create base and bound values that span the range of valid pointers.
  /// These bounds will prevent run-time errors for pointers where the actual
  /// bounds are unknown. Access errors through pointers with wide bounds will
  /// not be detected. These bounds will only be used when explicitly enabled by
  /// the user.
  auto getWideBounds() const -> std::pair<llvm::Value *, llvm::Value *>;

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

  /// Store base and bound information on the shadow stack.
  void insertShadowStackStore(llvm::IRBuilder<> &, llvm::Value *lowerBound,
                              llvm::Value *upperBound, int locIndex) const;

  /// The shadow stack stores bound information for pointers handed over to
  /// functions and pointers returned from functions.
  /// The location on the stack is build up follows:
  ///   retptr, arg0 bounds, arg1 boundes, ..., argn bounds
  /// As there is nothing to put on the stack for non-pointer values, the
  /// location on the stack is not the argument number (+1), but rather the
  /// number of pointers before the requested argument bounds.
  /// To avoid flaws in the shadow stack location computation, this functions
  /// provides a consistent look up of the correct index.
  ///
  /// The first value describes which argument/returned pointer should be looked
  /// up, the second one specifies in which call/function to search for the
  /// argument. The second argument is not needed for returned pointer values,
  /// as they can be identified as call/return.
  auto computeShadowStackLocation(const llvm::Instruction *) const -> unsigned;
  auto computeShadowStackLocation(const llvm::Argument *,
                                  const llvm::Function *usedIn) const
      -> unsigned;
  auto computeShadowStackLocation(const llvm::Value *,
                                  const llvm::CallBase *usedIn,
                                  unsigned argNum) const -> unsigned;

  /// Computes how many elements the shadow stack needs to be capable to store
  /// for a given call.
  auto computeSizeShadowStack(const llvm::CallBase *) const -> int;

  /// Insert a call to a spatial check function for the given target.
  void insertSpatialDereferenceCheck(const ITarget &) const;

  /// Insert a call to a spatial call check function for the given target.
  void insertSpatialCallCheck(const CallCheckIT &) const;

  /// Currently vectors of pointers are not handled properly. This function
  /// identifies unproblematic vectors.
  bool isSimpleVectorTy(const llvm::VectorType *) const;

  /// Check if the instruction is supported.
  bool isUnsupportedInstruction(unsigned opCode) const;

  /// Returns true iff the constant contains an integer to pointer cast.
  bool containsUnsupportedOp(const llvm::Constant *) const;

  /// Compute the highest valid address. This is needed in case wide bounds are
  /// used.
  auto determineHighestValidAddress() const -> uintptr_t;

  /// Check if this module contains any unsupported constructs (e.g. exception
  /// handling)
  void checkModule(llvm::Module &);

  /// Get the first instruction before/after start, that does not have metadata
  /// containing nodeString.
  auto getLastMDLocation(llvm::Instruction *start, llvm::StringRef nodeString,
                         bool forward) const -> llvm::Instruction *;
};

} // namespace meminstrument

#endif
