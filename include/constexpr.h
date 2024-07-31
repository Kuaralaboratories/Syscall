#ifndef SYSCALL_CONSTEXPR_H
#define SYSCALL_CONSTEXPR_H

#include <optional>

#include "ast.h"

namespace syscall {

class ConstantExpressionEvaluator {
  std::optional<double>
  evaluateBinaryOperator(const SyscallBinaryOperator &binop,
                         bool allowSideEffects);
  std::optional<double> evaluateUnaryOperator(const SyscallUnaryOperator &unop,
                                              bool allowSideEffects);
  std::optional<double> evaluateDeclRefExpr(const SyscallDeclRefExpr &dre,
                                            bool allowSideEffects);

public:
  std::optional<double> evaluate(const SyscallExpr &expr,
                                 bool allowSideEffects);
};

} // namespace syscall

#endif // SYSCALL_CONSTEXPR_H
