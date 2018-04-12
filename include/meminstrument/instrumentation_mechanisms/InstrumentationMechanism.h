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

class InstrumentationMechanism {
public:
  virtual void insertWitness(ITarget &Target) const = 0;

  virtual void insertCheck(ITarget &Target) const = 0;

  virtual void materializeBounds(ITarget &Target) const = 0;

  virtual llvm::Constant *getFailFunction(void) const = 0;

  virtual std::shared_ptr<Witness> insertWitnessPhi(ITarget &Target) const = 0;

  virtual void addIncomingWitnessToPhi(std::shared_ptr<Witness> &Phi,
                                       std::shared_ptr<Witness> &Incoming,
                                       llvm::BasicBlock *InBB) const = 0;

  virtual std::shared_ptr<Witness>
  insertWitnessSelect(ITarget &Target, std::shared_ptr<Witness> &TrueWitness,
                      std::shared_ptr<Witness> &FalseWitness) const = 0;

  virtual bool initialize(llvm::Module &M) = 0;

  virtual const char *getName(void) const = 0;

  virtual ~InstrumentationMechanism(void) {}

  InstrumentationMechanism(GlobalConfig &cfg) : _CFG(cfg) { }

protected:
  GlobalConfig &_CFG;

private:

  /// Base case for the implementation of the insertFunDecl helper function.
  static llvm::Constant *insertFunDecl_impl(std::vector<llvm::Type *> &Vec,
                                            llvm::Module &M,
                                            llvm::StringRef Name,
                                            llvm::AttributeList AList,
                                            llvm::Type *RetTy);

  /// Recursive case for the implementation of the insertFunDecl helper
  /// function.
  template <typename... Args>
  static llvm::Constant *
  insertFunDecl_impl(std::vector<llvm::Type *> &Vec, llvm::Module &M,
                     llvm::StringRef Name, llvm::AttributeList AList, llvm::Type *RetTy, llvm::Type *Ty,
                     Args... args) {
    Vec.push_back(Ty);
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

  /// Base case for the implementation of the insertCall helper function.
  static llvm::Value *insertCall_impl(std::vector<llvm::Value *> &Vec,
                                      llvm::IRBuilder<> &B, llvm::Constant *Fun,
                                      llvm::Twine &Name);

  /// Recursive case for the implementation of the insertCall helper
  /// function.
  template <typename... Args>
  static llvm::Value *insertCall_impl(std::vector<llvm::Value *> &Vec,
                                      llvm::IRBuilder<> &B, llvm::Constant *Fun,
                                      llvm::Twine &Name, llvm::Value *Val,
                                      Args... args) {
    Vec.push_back(Val);
    return insertCall_impl(Vec, B, Fun, Name, args...);
  }

protected:
  static std::unique_ptr<std::vector<llvm::Function *>>
  registerCtors(llvm::Module &M,
                llvm::ArrayRef<std::pair<llvm::StringRef, int>> List);
  static llvm::GlobalVariable *insertStringLiteral(llvm::Module &M,
                                                   llvm::StringRef Str);

  /// Inserts a function declaration into a Module and marks it for no
  /// instrumentation.
  template <typename... Args>
  static llvm::Constant *insertFunDecl(llvm::Module &M, llvm::StringRef Name,
                                       llvm::Type *RetTy, Args... args) {
    std::vector<llvm::Type *> Vec;
    // create an empty AttributeList
    llvm::ArrayRef<std::pair<unsigned, llvm::AttributeSet>> ar;
    llvm::AttributeList AList = llvm::AttributeList::get(M.getContext(), ar);
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

  template <typename... Args>
  static llvm::Constant *insertFunDecl(llvm::Module &M, llvm::StringRef Name, llvm::AttributeList AList,
                                       llvm::Type *RetTy, Args... args) {
    std::vector<llvm::Type *> Vec;
    return insertFunDecl_impl(Vec, M, Name, AList, RetTy, args...);
  }

  /// Inserts a call to a function and marks it for no instrumentation.
  template <typename... Args>
  static llvm::Value *insertCall(llvm::IRBuilder<> &B, llvm::Constant *Fun,
                                 llvm::Twine &Name, Args... args) {
    std::vector<llvm::Value *> Vec;
    return insertCall_impl(Vec, B, Fun, Name, args...);
  }

  /// Inserts a call to a function and marks it for no instrumentation.
  template <typename... Args>
  static llvm::Value *insertCall(llvm::IRBuilder<> &B, llvm::Constant *Fun,
                                 Args... args) {
    llvm::Twine Name = "";
    return insertCall(B, Fun, Name, args...);
  }

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
