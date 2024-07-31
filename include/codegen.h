#ifndef SYSCALL_CODEGEN_H
#define SYSCALL_CODEGEN_H

#include <llvm/IR/IRBuilder.h>

#include <map>
#include <memory>
#include <vector>

#include "ast.h"

namespace syscall {

class Codegen {
  std::vector<std::unique_ptr<SyscallFunctionDecl>> resolvedTree; // Updated type for Syscall
  std::map<const SyscallDecl *, llvm::Value *> declarations;     // Updated type for Syscall

  llvm::Value *retVal = nullptr;
  llvm::BasicBlock *retBB = nullptr;
  llvm::Instruction *allocaInsertPoint;

  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  llvm::Module module;

  llvm::Type *generateType(SyscallType type); // Updated to SyscallType

  llvm::Value *generateStmt(const SyscallStmt &stmt); // Updated type for Syscall
  llvm::Value *generateIfStmt(const SyscallIfStmt &stmt); // Updated type for Syscall
  llvm::Value *generateWhileStmt(const SyscallWhileStmt &stmt); // Updated type for Syscall
  llvm::Value *generateDeclStmt(const SyscallDeclStmt &stmt); // Updated type for Syscall
  llvm::Value *generateAssignment(const SyscallAssignment &stmt); // Updated type for Syscall
  llvm::Value *generateReturnStmt(const SyscallReturnStmt &stmt); // Updated type for Syscall

  llvm::Value *generateExpr(const SyscallExpr &expr); // Updated type for Syscall
  llvm::Value *generateCallExpr(const SyscallCallExpr &call); // Updated type for Syscall
  llvm::Value *generateBinaryOperator(const SyscallBinaryOperator &binop); // Updated type for Syscall
  llvm::Value *generateUnaryOperator(const SyscallUnaryOperator &unop); // Updated type for Syscall

  void generateConditionalOperator(const SyscallExpr &op,
                                   llvm::BasicBlock *trueBlock,
                                   llvm::BasicBlock *falseBlock); // Updated type for Syscall

  llvm::Value *doubleToBool(llvm::Value *v);
  llvm::Value *boolToDouble(llvm::Value *v);

  llvm::Function *getCurrentFunction();
  llvm::AllocaInst *allocateStackVariable(const std::string_view identifier);

  void generateBlock(const SyscallBlock &block); // Updated type for Syscall
  void generateFunctionBody(const SyscallFunctionDecl &functionDecl); // Updated type for Syscall
  void generateFunctionDecl(const SyscallFunctionDecl &functionDecl); // Updated type for Syscall

  void generateBuiltinPrintlnBody(const SyscallFunctionDecl &println); // Updated type for Syscall
  void generateMainWrapper();

public:
  Codegen(std::vector<std::unique_ptr<SyscallFunctionDecl>> resolvedTree,
          std::string_view sourcePath);

  llvm::Module *generateIR();
};

} // namespace syscall

#endif // SYSCALL_CODEGEN_H
