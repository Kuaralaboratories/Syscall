#include <optional>

#include "constexpr.h"

namespace {
std::optional<bool> toBool(std::optional<double> d) {
  if (!d)
    return std::nullopt;

  return *d != 0.0;
}
} // namespace

namespace syscall {

std::optional<double> ConstantExpressionEvaluator::evaluateBinaryOperator(
    const ResolvedBinaryOperator &binop, bool allowSideEffects) {
  std::optional<double> lhs = evaluate(*binop.lhs, allowSideEffects);

  if (!lhs && !allowSideEffects)
    return std::nullopt;

  switch (binop.op) {
    case TokenKind::PipePipe: { // Logical OR (||)
      // Short-circuit evaluation for OR
      if (toBool(lhs) == true)
        return 1.0;

      std::optional<double> rhs = evaluate(*binop.rhs, allowSideEffects);
      if (toBool(rhs) == true)
        return 1.0;

      if (lhs && rhs)
        return 0.0;

      return std::nullopt;
    }
    
    case TokenKind::AmpAmp: { // Logical AND (&&)
      // Short-circuit evaluation for AND
      if (toBool(lhs) == false)
        return 0.0;

      std::optional<double> rhs = evaluate(*binop.rhs, allowSideEffects);
      if (toBool(rhs) == false)
        return 0.0;

      if (lhs && rhs)
        return 1.0;

      return std::nullopt;
    }

    case TokenKind::Asterisk: // Multiplication (*)
      if (!lhs)
        return std::nullopt;
      return *lhs * evaluate(*binop.rhs, allowSideEffects).value_or(0.0);

    case TokenKind::Slash: // Division (/)
      if (!lhs)
        return std::nullopt;
      return *lhs / evaluate(*binop.rhs, allowSideEffects).value_or(1.0); // Avoid division by zero

    case TokenKind::Plus: // Addition (+)
      if (!lhs)
        return std::nullopt;
      return *lhs + evaluate(*binop.rhs, allowSideEffects).value_or(0.0);

    case TokenKind::Minus: // Subtraction (-)
      if (!lhs)
        return std::nullopt;
      return *lhs - evaluate(*binop.rhs, allowSideEffects).value_or(0.0);

    case TokenKind::Lt: // Less than (<)
      if (!lhs)
        return std::nullopt;
      return *lhs < evaluate(*binop.rhs, allowSideEffects).value_or(0.0) ? 1.0 : 0.0;

    case TokenKind::Gt: // Greater than (>)
      if (!lhs)
        return std::nullopt;
      return *lhs > evaluate(*binop.rhs, allowSideEffects).value_or(0.0) ? 1.0 : 0.0;

    case TokenKind::EqualEqual: // Equality (==)
      if (!lhs)
        return std::nullopt;
      return *lhs == evaluate(*binop.rhs, allowSideEffects).value_or(0.0) ? 1.0 : 0.0;

    default:
      llvm_unreachable("unexpected binary operator");
  }
}

std::optional<double> ConstantExpressionEvaluator::evaluateUnaryOperator(
    const ResolvedUnaryOperator &unop, bool allowSideEffects) {
  std::optional<double> operand = evaluate(*unop.operand, allowSideEffects);
  if (!operand)
    return std::nullopt;

  switch (unop.op) {
    case TokenKind::Excl: // Logical NOT (!)
      return !toBool(operand) ? 1.0 : 0.0;

    case TokenKind::Minus: // Unary minus (-)
      return -*operand;

    default:
      llvm_unreachable("unexpected unary operator");
  }
}

std::optional<double> ConstantExpressionEvaluator::evaluateDeclRefExpr(
    const ResolvedDeclRefExpr &dre, bool allowSideEffects) {
  // We only care about references to immutable variables with an initializer.
  const auto *rvd = dynamic_cast<const ResolvedVarDecl *>(dre.decl);
  if (!rvd || rvd->isMutable || !rvd->initializer)
    return std::nullopt;

  return evaluate(*rvd->initializer, allowSideEffects);
}

std::optional<double> ConstantExpressionEvaluator::evaluate(
    const ResolvedExpr &expr, bool allowSideEffects) {
  // Don't evaluate the same expression multiple times.
  if (std::optional<double> val = expr.getConstantValue())
    return val;

  if (const auto *numberLiteral =
          dynamic_cast<const ResolvedNumberLiteral *>(&expr))
    return numberLiteral->value;

  if (const auto *groupingExpr =
          dynamic_cast<const ResolvedGroupingExpr *>(&expr))
    return evaluate(*groupingExpr->expr, allowSideEffects);

  if (const auto *binaryOperator =
          dynamic_cast<const ResolvedBinaryOperator *>(&expr))
    return evaluateBinaryOperator(*binaryOperator, allowSideEffects);

  if (const auto *unaryOperator =
          dynamic_cast<const ResolvedUnaryOperator *>(&expr))
    return evaluateUnaryOperator(*unaryOperator, allowSideEffects);

  if (const auto *declRefExpr =
          dynamic_cast<const ResolvedDeclRefExpr *>(&expr))
    return evaluateDeclRefExpr(*declRefExpr, allowSideEffects);

  return std::nullopt;
}
} // namespace syscall