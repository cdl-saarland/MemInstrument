//=== meminstrument/InstrumentationMechanism.h - MemSafety Instr -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file TODO doku
///
//===----------------------------------------------------------------------===//

#ifndef MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
#define MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H

#include "meminstrument/ITarget.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"

namespace meminstrument {

class InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) const = 0;

  virtual void insertCheck(ITarget &Target) const = 0;

  virtual void materializeBounds(ITarget &Target) const = 0;

  virtual std::shared_ptr<Witness> insertWitnessPhi(ITarget &Target) const = 0;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const = 0;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const = 0;

  virtual bool initialize(llvm::Module &M) = 0;

  virtual ~InstrumentationMechanism(void) {}

  static InstrumentationMechanism &get(void);

private:
  /// Base case for the implementation of the insertFunDecl helper function.
  static llvm::Constant *insertFunDecl_impl(std::vector<llvm::Type *> &Vec,
                                            llvm::Module &M,
                                            llvm::StringRef Name,
                                            llvm::Type *RetTy);

  /// Recursive case for the implementation of the insertFunDecl helper
  /// function.
  template <typename... Args>
  llvm::Constant *insertFunDecl_impl(std::vector<llvm::Type *> &Vec,
                                     llvm::Module &M, llvm::StringRef Name,
                                     llvm::Type *RetTy, llvm::Type *Ty,
                                     Args... args) {
    Vec.push_back(Ty);
    return insertFunDecl_impl(Vec, M, Name, RetTy, args...);
  }

protected:
  static std::unique_ptr<std::vector<llvm::Function *>>
  registerCtors(llvm::Module &M,
                llvm::ArrayRef<std::pair<llvm::StringRef, int>> List);
  static llvm::GlobalVariable *insertStringLiteral(llvm::Module &M,
                                                   llvm::StringRef Str);

  /// Inserts a function declaration into a Module and marks it to not be
  /// instrumented.
  template <typename... Args>
  llvm::Constant *insertFunDecl(llvm::Module &M, llvm::StringRef Name,
                                llvm::Type *RetTy, Args... args) {
    std::vector<llvm::Type *> Vec;
    return insertFunDecl_impl(Vec, M, Name, RetTy, args...);
  }
};

} // end namespace meminstrument

#endif // MEMINSTRUMENT_INSTRUMENTATIONMECHANISM_H
