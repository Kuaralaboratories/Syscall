#ifndef SYSCALL_AST_H
#define SYSCALL_AST_H

#include <llvm/Support/ErrorHandling.h>

#include <memory>
#include <vector>

#include "lexer.h"
#include "utils.h"

namespace yl {
struct Type {
  enum class Kind { Void, Number, Custom };

  Kind kind;
  std::string name;

  static Type builtinVoid() { return {Kind::Void, "void"}; }
  static Type builtinNumber() { return {Kind::Number, "number"}; }
  static Type custom(const std::string &name) { return {Kind::Custom, name}; }

private:
  Type(Kind kind, std::string name)
      : kind(kind),
        name(std::move(name)){};
};

struct Decl {
  SourceLocation location;
  std::string identifier;

  Decl(SourceLocation location, std::string identifier)
      : location(location),
        identifier(std::move(identifier)) {}
  virtual ~Decl() = default;

  virtual void dump(size_t level = 0) const = 0;
};

struct Stmt {
  SourceLocation location;
  Stmt(SourceLocation location)
      : location(location) {}

  virtual ~Stmt() = default;

  virtual void dump(size_t level = 0) const = 0;
};

struct Expr : public Stmt {
  Expr(SourceLocation location)
      : Stmt(location) {}
};

struct Block {
  SourceLocation location;
  std::vector<std::unique_ptr<Stmt>> statements;

  Block(SourceLocation location, std::vector<std::unique_ptr<Stmt>> statements)
      : location(location),
        statements(std::move(statements)) {}

  void dump(size_t level = 0) const;
};

struct IfStmt : public Stmt {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Block> trueBlock;
  std::unique_ptr<Block> falseBlock;

  IfStmt(SourceLocation location,
         std::unique_ptr<Expr> condition,
         std::unique_ptr<Block> trueBlock,
         std::unique_ptr<Block> falseBlock = nullptr)
      : Stmt(location),
        condition(std::move(condition)),
        trueBlock(std::move(trueBlock)),
        falseBlock(std::move(falseBlock)) {}

  void dump(size_t level = 0) const override;
};

struct ReturnStmt : public Stmt {
  std::unique_ptr<Expr> expr;

  ReturnStmt(SourceLocation location, std::unique_ptr<Expr> expr = nullptr)
      : Stmt(location),
        expr(std::move(expr)) {}

  void dump(size_t level = 0) const override;
};

struct NumberLiteral : public Expr {
  std::string value;

  NumberLiteral(SourceLocation location, std::string value)
      : Expr(location),
        value(value) {}

  void dump(size_t level = 0) const override;
};

struct DeclRefExpr : public Expr {
  std::string identifier;

  DeclRefExpr(SourceLocation location, std::string identifier)
      : Expr(location),
        identifier(identifier) {}

  void dump(size_t level = 0) const override;
};

struct BinaryOperator : public Expr {
  std::unique_ptr<Expr> lhs;
  std::unique_ptr<Expr> rhs;
  TokenKind op;

  BinaryOperator(SourceLocation location,
                 std::unique_ptr<Expr> lhs,
                 std::unique_ptr<Expr> rhs,
                 TokenKind op)
      : Expr(location),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)),
        op(op) {}

  void dump(size_t level = 0) const override;
};

struct CallExpr : public Expr {
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> arguments;

  CallExpr(SourceLocation location,
           std::unique_ptr<Expr> callee,
           std::vector<std::unique_ptr<Expr>> arguments)
      : Expr(location),
        callee(std::move(callee)),
        arguments(std::move(arguments)) {}

  void dump(size_t level = 0) const override;
};

struct Assignment : public Stmt {
  std::unique_ptr<DeclRefExpr> variable;
  std::unique_ptr<Expr> expr;

  Assignment(SourceLocation location,
             std::unique_ptr<DeclRefExpr> variable,
             std::unique_ptr<Expr> expr)
      : Stmt(location),
        variable(std::move(variable)),
        expr(std::move(expr)) {}

  void dump(size_t level = 0) const override;
};

struct ReadRegisterExpr : public Expr {
  std::string address;

  ReadRegisterExpr(SourceLocation location, std::string address)
      : Expr(location),
        address(address) {}

  void dump(size_t level = 0) const override;
};

struct LogExpr : public Expr {
  std::unique_ptr<Expr> expr;

  LogExpr(SourceLocation location, std::unique_ptr<Expr> expr)
      : Expr(location),
        expr(std::move(expr)) {}

  void dump(size_t level = 0) const override;
};

struct MainFunctionDecl : public Decl {
  std::unique_ptr<Block> body;

  MainFunctionDecl(SourceLocation location, std::unique_ptr<Block> body)
      : Decl(location, "main"),
        body(std::move(body)) {}

  void dump(size_t level = 0) const override;
};

struct Program {
  std::vector<std::unique_ptr<Decl>> declarations;

  void dump(size_t level = 0) const;
};

} // namespace yl

#endif // SYSCALL_AST_H
