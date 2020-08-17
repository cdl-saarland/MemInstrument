//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_INSTRUMENTATIONMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATION_MECHANISMS_INSTRUMENTATIONMECHANISM_H

#include "meminstrument/pass/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

class GlobalConfig;

/// Subclasses of this abstract class describe how a specific instrumentation
/// is implemented. This abstract class therefore declares methods to obtain
/// Witnesses (e.g. load bounds), to propagate witnesses, and to insert check
/// code.
/// It further provides handles to usefule instrumentation functions for other
/// components, such as a fail function that aborts the instrumented cleanly
/// with a backtrace when executed.
///
/// This class is not concerned with where to place checks and where to realize
/// witnesses, refer to InstrumentationPolicy and WitnessStrategy for these
/// issues.
class InstrumentationMechanism {
public:
  /// Set-Up code that is executed once in the beginning before using the
  /// instrumentation. Classical uses are inserting necessary Functions and/or
  /// declarations and creating relevant LLVM types for later use.
  virtual void initialize(llvm::Module &M) = 0;

  /// Generates a Witness for the instrumentee of the Target at the location of
  /// the Target and store it in the target.
  /// Typically used to get witnesses for sources of pointer computations in a
  /// function.
  virtual void insertWitness(ITarget &Target) const = 0;

  /// Inserts a new witness for Target at Target's location, with the same
  /// information as W.
  /// This is an unelegant hack to force the instrumentation to materialize
  /// bounds at specific locations.
  virtual void relocCloneWitness(Witness &W, ITarget &Target) const = 0;

  /// Insert phis for all Values necessary for the Witness for Target at the
  /// location of Target without filling in phi operands.
  /// This is a component of the propagation of Witnesses through the program
  /// for phis with pointer type.
  virtual std::shared_ptr<Witness> insertWitnessPhi(ITarget &Target) const = 0;

  /// Add the Witness Values from Incoming as phi arguments to the Witness phi
  /// Phi as coming from InBB.
  /// This is a component of the propagation of Witnesses through the program
  /// for phis with pointer type.
  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const = 0;

  /// Insert selects for all Values necessary for the Witness for Target at the
  /// location of Target.
  /// This is a component of the propagation of Witnesses through the program
  /// for selects with pointer type.
  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const = 0;

  /// Make sure that explicit bounds are available in the witness for the
  /// instrumentee of the Target at the location of Target.
  virtual void materializeBounds(ITarget &Target) = 0;

  /// Insert a check to check whether the instrumentee of Target is valid
  /// according to the witness stored in Target at the location of Target.
  virtual void insertCheck(ITarget &Target) const = 0;

  /// Provides an llvm Function in the module that can be called to abort the
  /// execution of the instrumented program.
  virtual llvm::Value *getFailFunction(void) const = 0;

  /// Provides an llvm Function in the module that can be called to abort the
  /// execution of the instrumented program with a custom error message.
  /// Optional.
  virtual llvm::Value *getVerboseFailFunction(void) const {
    llvm_unreachable("Not supported!");
  }

  /// Provides a function to call in the instrumented program to increment a
  /// run-time counter (for statistics).
  /// Optional.
  virtual llvm::Value *getExtCheckCounterFunction(void) const {
    llvm_unreachable("Not supported!");
  }

  /// Returns the name of the instrumentation mechanism for printing and easy
  /// recognition.
  virtual const char *getName(void) const = 0;

  virtual ~InstrumentationMechanism(void) {}

  InstrumentationMechanism(GlobalConfig &cfg) : _CFG(cfg) {}

  /// Convenient helper function to insert a string literal into an LLVM
  /// Module, useful for use with the verbose fail function provided by
  /// getVerboseFailFunction().
  static llvm::GlobalVariable *insertStringLiteral(llvm::Module &M,
                                                   llvm::StringRef Str);

protected:
  GlobalConfig &_CFG;

private:
  /// Base case for the implementation of the insertFunDecl helper function.
  static llvm::Value *insertFunDecl_impl(std::vector<llvm::Type *> &Vec,
                                         llvm::Module &M, llvm::StringRef Name,
                                         llvm::AttributeList AList,
                                         llvm::Type *RetTy);

  /// Recursive case for the implementation of the insertFunDecl helper
  /// function.
  template <typename... Args>
  static llvm::Value *
  insertFunDecl_impl(std::vector<llvm::Type *> &Vec, llvm::Module &M,
                     llvm::StringRef Name, llvm::AttributeList AList,
                     llvm::Type *RetTy, llvm::Type *Ty, Args... args) {
    Vec.push_back(Ty);
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

protected:
  /// Helper function to register an llvm function to be called at
  /// initialization time in the execution of the instrumented program.
  static std::unique_ptr<std::vector<llvm::Function *>>
  registerCtors(llvm::Module &M,
                llvm::ArrayRef<std::pair<llvm::StringRef, int>> List);

  /// Inserts a function declaration into a Module and marks it for no
  /// instrumentation.
  template <typename... Args>
  static llvm::Value *insertFunDecl(llvm::Module &M, llvm::StringRef Name,
                                    llvm::Type *RetTy, Args... args) {
    std::vector<llvm::Type *> Vec;
    // create an empty AttributeList
    llvm::ArrayRef<std::pair<unsigned, llvm::AttributeSet>> ar;
    llvm::AttributeList AList = llvm::AttributeList::get(M.getContext(), ar);
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

  template <typename... Args>
  static llvm::Value *insertFunDecl(llvm::Module &M, llvm::StringRef Name,
                                    llvm::AttributeList AList,
                                    llvm::Type *RetTy, Args... args) {
    std::vector<llvm::Type *> Vec;
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

  /// Several helper functions for inserting new instructions.
  static llvm::Instruction *insertCall(llvm::IRBuilder<> &B, llvm::Value *Fun,
                                       const std::vector<llvm::Value *> &&args,
                                       const llvm::Twine &Name);

  static llvm::Instruction *insertCall(llvm::IRBuilder<> &B, llvm::Value *Fun,
                                       llvm::Value *arg,
                                       const llvm::Twine &Name);

  static llvm::Instruction *insertCall(llvm::IRBuilder<> &B, llvm::Value *Fun,
                                       const std::vector<llvm::Value *> &&args);

  static llvm::Instruction *insertCall(llvm::IRBuilder<> &B, llvm::Value *Fun,
                                       llvm::Value *arg);

  static llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                                 llvm::IRBuilder<> &Builder,
                                 llvm::StringRef Suffix);

  static llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                                 llvm::IRBuilder<> &Builder);

  static llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                                 llvm::Instruction *Location);

  static llvm::Value *insertCast(llvm::Type *DestType, llvm::Value *FromVal,
                                 llvm::Instruction *Location,
                                 llvm::StringRef Suffix);
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
