#ifndef SYSCALL_SEMA_H
#define SYSCALL_SEMA_H

#include <memory>
#include <optional>
#include <vector>

#include "ast.h"
#include "cfg.h"
#include "constexpr.h"

namespace syscall {

class Sema {
  ConstantExpressionEvaluator cee;
  std::vector<std::unique_ptr<SyscallFunctionDecl>> ast;
  std::vector<std::vector<SyscallDecl *>> scopes;

  SyscallResolvedFunctionDecl *currentFunction;

  class ScopeRAII {
    Sema *sema;

  public:
    explicit ScopeRAII(Sema *sema)
        : sema(sema) {
      sema->scopes.emplace_back();
    }
    ~ScopeRAII() { sema->scopes.pop_back(); }
  };

  std::optional<SyscallType> resolveType(SyscallType parsedType);

  std::unique_ptr<SyscallResolvedUnaryOperator>
  resolveUnaryOperator(const SyscallUnaryOperator &unary);
  std::unique_ptr<SyscallResolvedBinaryOperator>
  resolveBinaryOperator(const SyscallBinaryOperator &binop);
  std::unique_ptr<SyscallResolvedGroupingExpr>
  resolveGroupingExpr(const SyscallGroupingExpr &grouping);
  std::unique_ptr<SyscallResolvedDeclRefExpr>
  resolveDeclRefExpr(const SyscallDeclRefExpr &declRefExpr, bool isCallee = false);
  std::unique_ptr<SyscallResolvedCallExpr> resolveCallExpr(const SyscallCallExpr &call);
  std::unique_ptr<SyscallResolvedExpr> resolveExpr(const SyscallExpr &expr);

  std::unique_ptr<SyscallResolvedStmt> resolveStmt(const SyscallStmt &stmt);
  std::unique_ptr<SyscallResolvedIfStmt> resolveIfStmt(const SyscallIfStmt &ifStmt);
  std::unique_ptr<SyscallResolvedWhileStmt>
  resolveWhileStmt(const SyscallWhileStmt &whileStmt);
  std::unique_ptr<SyscallResolvedDeclStmt> resolveDeclStmt(const SyscallDeclStmt &declStmt);
  std::unique_ptr<SyscallResolvedAssignment>
  resolveAssignment(const SyscallAssignment &assignment);
  std::unique_ptr<SyscallResolvedReturnStmt>
  resolveReturnStmt(const SyscallReturnStmt &returnStmt);

  std::unique_ptr<SyscallResolvedBlock> resolveBlock(const SyscallBlock &block);

  std::unique_ptr<SyscallResolvedParamDecl> resolveParamDecl(const SyscallParamDecl &param);
  std::unique_ptr<SyscallResolvedVarDecl> resolveVarDecl(const SyscallVarDecl &varDecl);
  std::unique_ptr<SyscallResolvedFunctionDecl>
  resolveFunctionDeclaration(const SyscallFunctionDecl &function);

  bool insertDeclToCurrentScope(SyscallResolvedDecl &decl);
  std::pair<SyscallResolvedDecl *, int> lookupDecl(const std::string id);
  std::unique_ptr<SyscallResolvedFunctionDecl> createBuiltinPrintln();

  bool runFlowSensitiveChecks(const SyscallResolvedFunctionDecl &fn);
  bool checkReturnOnAllPaths(const SyscallResolvedFunctionDecl &fn, const CFG &cfg);
  bool checkVariableInitialization(const CFG &cfg);

public:
  explicit Sema(std::vector<std::unique_ptr<SyscallFunctionDecl>> ast)
      : ast(std::move(ast)) {}

  std::vector<std::unique_ptr<SyscallResolvedFunctionDecl>> resolveAST();
};

} // namespace syscall

#endif // SYSCALL_SEMA_H
