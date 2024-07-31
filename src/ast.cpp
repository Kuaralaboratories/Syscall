#include <iostream>
#include "ast.h"

namespace syscall {
namespace {
std::string_view getOpStr(TokenKind op) {
  if (op == TokenKind::Plus)
    return "+";
  if (op == TokenKind::Minus)
    return "-";
  if (op == TokenKind::Asterisk)
    return "*";
  if (op == TokenKind::Slash)
    return "/";
  if (op == TokenKind::EqualEqual)
    return "==";
  if (op == TokenKind::AmpAmp)
    return "and";
  if (op == TokenKind::PipePipe)
    return "or";
  if (op == TokenKind::Lt)
    return "<";
  if (op == TokenKind::Gt)
    return ">";
  if (op == TokenKind::Excl)
    return "!";

  llvm_unreachable("unexpected operator");
}

std::string indent(size_t level) { return std::string(level * 2, ' '); }
} // namespace

void SyscallBlock::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallBlock\n";
  for (auto &&stmt : statements)
    stmt->dump(level + 1);
}

void SyscallIfStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallIfStmt\n";
  condition->dump(level + 1);
  trueBlock->dump(level + 1);
  if (falseBlock)
    falseBlock->dump(level + 1);
}

void SyscallWhileStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallWhileStmt\n";
  condition->dump(level + 1);
  body->dump(level + 1);
}

void SyscallReturnStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallReturnStmt\n";
  if (expr)
    expr->dump(level + 1);
}

void SyscallNumberLiteral::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallNumberLiteral: '" << value << "'\n";
}

void SyscallDeclRefExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallDeclRefExpr: " << identifier << '\n';
}

void SyscallCallExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallCallExpr:\n";
  callee->dump(level + 1);
  for (auto &&arg : arguments)
    arg->dump(level + 1);
}

void SyscallGroupingExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallGroupingExpr:\n";
  expr->dump(level + 1);
}

void SyscallBinaryOperator::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallBinaryOperator: '" << getOpStr(op) << "'\n";
  lhs->dump(level + 1);
  rhs->dump(level + 1);
}

void SyscallUnaryOperator::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallUnaryOperator: '" << getOpStr(op) << "'\n";
  operand->dump(level + 1);
}

void SyscallParamDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallParamDecl: " << identifier << ':' << type.name << '\n';
}

void SyscallVarDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallVarDecl: " << identifier;
  if (type)
    std::cerr << ':' << type->name;
  std::cerr << '\n';
  if (initializer)
    initializer->dump(level + 1);
}

void SyscallFunctionDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallFunctionDecl: " << identifier << ':' << type.name << '\n';
  for (auto &&param : params)
    param->dump(level + 1);
  body->dump(level + 1);
}

void SyscallDeclStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallDeclStmt:\n";
  varDecl->dump(level + 1);
}

void SyscallAssignment::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallAssignment:\n";
  variable->dump(level + 1);
  expr->dump(level + 1);
}

void SyscallResolvedBlock::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedBlock\n";
  for (auto &&stmt : statements)
    stmt->dump(level + 1);
}

void SyscallResolvedIfStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedIfStmt\n";
  condition->dump(level + 1);
  trueBlock->dump(level + 1);
  if (falseBlock)
    falseBlock->dump(level + 1);
}

void SyscallResolvedWhileStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedWhileStmt\n";
  condition->dump(level + 1);
  body->dump(level + 1);
}

void SyscallResolvedParamDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedParamDecl: @(" << this << ") " << identifier << ':' << '\n';
}

void SyscallResolvedVarDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedVarDecl: @(" << this << ") " << identifier << ':' << '\n';
  if (initializer)
    initializer->dump(level + 1);
}

void SyscallResolvedFunctionDecl::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedFunctionDecl: @(" << this << ") " << identifier << ':' << '\n';
  for (auto &&param : params)
    param->dump(level + 1);
  body->dump(level + 1);
}

void SyscallResolvedNumberLiteral::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedNumberLiteral: '" << value << "'\n";
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
}

void SyscallResolvedDeclRefExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedDeclRefExpr: @(" << decl << ") " << decl->identifier << '\n';
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
}

void SyscallResolvedCallExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedCallExpr: @(" << callee << ") " << callee->identifier << '\n';
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
  for (auto &&arg : arguments)
    arg->dump(level + 1);
}

void SyscallResolvedGroupingExpr::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedGroupingExpr:\n";
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
  expr->dump(level + 1);
}

void SyscallResolvedBinaryOperator::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedBinaryOperator: '" << getOpStr(op) << "'\n";
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
  lhs->dump(level + 1);
  rhs->dump(level + 1);
}

void SyscallResolvedUnaryOperator::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedUnaryOperator: '" << getOpStr(op) << "'\n";
  if (auto val = getConstantValue())
    std::cerr << indent(level) << "| value: " << *val << '\n';
  operand->dump(level + 1);
}

void SyscallResolvedDeclStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedDeclStmt:\n";
  varDecl->dump(level + 1);
}

void SyscallResolvedAssignment::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedAssignment:\n";
  variable->dump(level + 1);
  expr->dump(level + 1);
}

void SyscallResolvedReturnStmt::dump(size_t level) const {
  std::cerr << indent(level) << "SyscallResolvedReturnStmt\n";
  if (expr)
    expr->dump(level + 1);
}
} // namespace syscall
