#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Host.h>

#include "codegen.h"

namespace syscall {
Codegen::Codegen(
    std::vector<std::unique_ptr<ResolvedFunctionDecl>> resolvedTree,
    std::string_view sourcePath)
    : resolvedTree(std::move(resolvedTree)),
      builder(context),
      module("<translation_unit>", context) {
  module.setSourceFileName(sourcePath);
  module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
}

llvm::Type *Codegen::generateType(Type type) {
  switch (type.kind) {
    case Type::Kind::Number:
      return builder.getDoubleTy();
    case Type::Kind::Void:
      return builder.getVoidTy();
    case Type::Kind::Int:
      return builder.getInt32Ty(); // Assuming Syscall has 32-bit integers
    // Add more cases for other types as needed
    default:
      llvm_unreachable("unknown type");
  }
}

llvm::Value *Codegen::generateStmt(const ResolvedStmt &stmt) {
  if (auto *expr = dynamic_cast<const ResolvedExpr *>(&stmt))
    return generateExpr(*expr);

  if (auto *ifStmt = dynamic_cast<const ResolvedIfStmt *>(&stmt))
    return generateIfStmt(*ifStmt);

  if (auto *declStmt = dynamic_cast<const ResolvedDeclStmt *>(&stmt))
    return generateDeclStmt(*declStmt);

  if (auto *assignment = dynamic_cast<const ResolvedAssignment *>(&stmt))
    return generateAssignment(*assignment);

  if (auto *whileStmt = dynamic_cast<const ResolvedWhileStmt *>(&stmt))
    return generateWhileStmt(*whileStmt);

  if (auto *returnStmt = dynamic_cast<const ResolvedReturnStmt *>(&stmt))
    return generateReturnStmt(*returnStmt);

  llvm_unreachable("unknown statement");
}

llvm::Value *Codegen::generateIfStmt(const ResolvedIfStmt &stmt) {
  llvm::Function *function = getCurrentFunction();

  auto *trueBB = llvm::BasicBlock::Create(context, "if.true");
  auto *exitBB = llvm::BasicBlock::Create(context, "if.exit");

  llvm::BasicBlock *elseBB = exitBB;
  if (stmt.falseBlock)
    elseBB = llvm::BasicBlock::Create(context, "if.false");

  llvm::Value *cond = generateExpr(*stmt.condition);
  builder.CreateCondBr(doubleToBool(cond), trueBB, elseBB);

  trueBB->insertInto(function);
  builder.SetInsertPoint(trueBB);
  generateBlock(*stmt.trueBlock);
  builder.CreateBr(exitBB);

  if (stmt.falseBlock) {
    elseBB->insertInto(function);

    builder.SetInsertPoint(elseBB);
    generateBlock(*stmt.falseBlock);
    builder.CreateBr(exitBB);
  }

  exitBB->insertInto(function);
  builder.SetInsertPoint(exitBB);
  return nullptr;
}

llvm::Value *Codegen::generateWhileStmt(const ResolvedWhileStmt &stmt) {
  llvm::Function *function = getCurrentFunction();

  auto *header = llvm::BasicBlock::Create(context, "while.cond", function);
  auto *body = llvm::BasicBlock::Create(context, "while.body", function);
  auto *exit = llvm::BasicBlock::Create(context, "while.exit", function);

  builder.CreateBr(header);

  builder.SetInsertPoint(header);
  llvm::Value *cond = generateExpr(*stmt.condition);
  builder.CreateCondBr(doubleToBool(cond), body, exit);

  builder.SetInsertPoint(body);
  generateBlock(*stmt.body);
  builder.CreateBr(header);

  builder.SetInsertPoint(exit);
  return nullptr;
}

llvm::Value *Codegen::generateDeclStmt(const ResolvedDeclStmt &stmt) {
  const auto *decl = stmt.varDecl.get();
  llvm::AllocaInst *var = allocateStackVariable(decl->identifier);

  if (const auto &init = decl->initializer)
    builder.CreateStore(generateExpr(*init), var);

  declarations[decl] = var;
  return nullptr;
}

llvm::Value *Codegen::generateAssignment(const ResolvedAssignment &stmt) {
  return builder.CreateStore(generateExpr(*stmt.expr),
                             declarations[stmt.variable->decl]);
}

llvm::Value *Codegen::generateReturnStmt(const ResolvedReturnStmt &stmt) {
  if (stmt.expr)
    builder.CreateStore(generateExpr(*stmt.expr), retVal);

  assert(retBB && "function with return stmt doesn't have a return block");
  return builder.CreateBr(retBB);
}

llvm::Value *Codegen::generateExpr(const ResolvedExpr &expr) {
  if (auto *number = dynamic_cast<const ResolvedNumberLiteral *>(&expr))
    return llvm::ConstantFP::get(builder.getDoubleTy(), number->value);

  if (auto val = expr.getConstantValue())
    return llvm::ConstantFP::get(builder.getDoubleTy(), *val);

  if (auto *dre = dynamic_cast<const ResolvedDeclRefExpr *>(&expr))
    return builder.CreateLoad(builder.getDoubleTy(), declarations[dre->decl]);

  if (auto *call = dynamic_cast<const ResolvedCallExpr *>(&expr))
    return generateCallExpr(*call);

  if (auto *grouping = dynamic_cast<const ResolvedGroupingExpr *>(&expr))
    return generateExpr(*grouping->expr);

  if (auto *binop = dynamic_cast<const ResolvedBinaryOperator *>(&expr))
    return generateBinaryOperator(*binop);

  if (auto *unop = dynamic_cast<const ResolvedUnaryOperator *>(&expr))
    return generateUnaryOperator(*unop);

  llvm_unreachable("unexpected expression");
}

llvm::Value *Codegen::generateCallExpr(const ResolvedCallExpr &call) {
  llvm::Function *callee = module.getFunction(call.callee->identifier);

  std::vector<llvm::Value *> args;
  for (auto &&arg : call.arguments)
    args.emplace_back(generateExpr(*arg));

  return builder.CreateCall(callee, args);
}

llvm::Value *Codegen::generateUnaryOperator(const ResolvedUnaryOperator &unop) {
  llvm::Value *rhs = generateExpr(*unop.operand);

  if (unop.op == TokenKind::Excl)
    return boolToDouble(builder.CreateNot(doubleToBool(rhs)));

  if (unop.op == TokenKind::Minus)
    return builder.CreateFNeg(rhs);

  llvm_unreachable("unknown unary op");
}

void Codegen::generateConditionalOperator(const ResolvedExpr &op,
                                          llvm::BasicBlock *trueBB,
                                          llvm::BasicBlock *falseBB) {
  llvm::Function *function = getCurrentFunction();
  const auto *binop = dynamic_cast<const ResolvedBinaryOperator *>(&op);

  if (binop && binop->op == TokenKind::PipePipe) {
    llvm::BasicBlock *nextBB =
        llvm::BasicBlock::Create(context, "or.lhs.false", function);
    generateConditionalOperator(*binop->lhs, trueBB, nextBB);

    builder.SetInsertPoint(nextBB);
    generateConditionalOperator(*binop->rhs, trueBB, falseBB);
    return;
  }

  if (binop && binop->op == TokenKind::AmpAmp) {
    llvm::BasicBlock *nextBB =
        llvm::BasicBlock::Create(context, "and.lhs.true", function);
    generateConditionalOperator(*binop->lhs, nextBB, falseBB);

    builder.SetInsertPoint(nextBB);
    generateConditionalOperator(*binop->rhs, trueBB, falseBB);
    return;
  }

  llvm::Value *val = doubleToBool(generateExpr(op));
  builder.CreateCondBr(val, trueBB, falseBB);
}

llvm::Value *
Codegen::generateBinaryOperator(const ResolvedBinaryOperator &binop) {
  TokenKind op = binop.op;

  if (op == TokenKind::AmpAmp || op == TokenKind::PipePipe) {
    llvm::Function *function = getCurrentFunction();
    bool isOr = op == TokenKind::PipePipe;

    auto *rhsTag = isOr ? "or.rhs" : "and.rhs";
    auto *mergeTag = isOr ? "or.merge" : "and.merge";

    auto *rhsBB = llvm::BasicBlock::Create(context, rhsTag, function);
    auto *mergeBB = llvm::BasicBlock::Create(context, mergeTag, function);

    llvm::BasicBlock *trueBB = isOr ? mergeBB : rhsBB;
    llvm::BasicBlock *falseBB = isOr ? rhsBB : mergeBB;
    generateConditionalOperator(*binop.lhs, trueBB, falseBB);

    builder.SetInsertPoint(rhsBB);
    llvm::Value *rhs = doubleToBool(generateExpr(*binop.rhs));
    builder.CreateBr(mergeBB);

    rhsBB = builder.GetInsertBlock();
    builder.SetInsertPoint(mergeBB);
    llvm::PHINode *phi = builder.CreatePHI(builder.getInt1Ty(), 2);

    for (auto it = pred_begin(rhsBB); it != pred_end(rhsBB); ++it) {
      if (*it == rhsBB) {
        phi->addIncoming(rhs, rhsBB);
      }
    }

    return phi;
  }

  llvm::Value *lhs = generateExpr(*binop.lhs);
  llvm::Value *rhs = generateExpr(*binop.rhs);

  switch (op) {
    case TokenKind::Plus:
      return builder.CreateFAdd(lhs, rhs);
    case TokenKind::Minus:
      return builder.CreateFSub(lhs, rhs);
    case TokenKind::Star:
      return builder.CreateFMul(lhs, rhs);
    case TokenKind::Slash:
      return builder.CreateFDiv(lhs, rhs);
    case TokenKind::EqualEqual:
      return builder.CreateFCmpOEQ(lhs, rhs);
    case TokenKind::ExclEqual:
      return builder.CreateFCmpUNE(lhs, rhs);
    case TokenKind::Less:
      return builder.CreateFCmpOLT(lhs, rhs);
    case TokenKind::Greater:
      return builder.CreateFCmpOGT(lhs, rhs);
    case TokenKind::LessEqual:
      return builder.CreateFCmpOLE(lhs, rhs);
    case TokenKind::GreaterEqual:
      return builder.CreateFCmpOGE(lhs, rhs);
    default:
      llvm_unreachable("unknown binary operator");
  }
}

llvm::Value *Codegen::doubleToBool(llvm::Value *value) {
  return builder.CreateFCmpONE(value, llvm::ConstantFP::get(builder.getDoubleTy(), 0));
}

llvm::Value *Codegen::boolToDouble(llvm::Value *value) {
  return builder.CreateSelect(value, llvm::ConstantFP::get(builder.getDoubleTy(), 1),
                              llvm::ConstantFP::get(builder.getDoubleTy(), 0));
}

llvm::AllocaInst *Codegen::allocateStackVariable(const std::string &name) {
  llvm::Function *function = getCurrentFunction();
  llvm::IRBuilder<> tempBuilder(function->getEntryBlock().getFirstInsertionPt());
  return tempBuilder.CreateAlloca(builder.getDoubleTy(), nullptr, name);
}

llvm::Function *Codegen::getCurrentFunction() {
  return builder.GetInsertBlock()->getParent();
}

void Codegen::generateBlock(const ResolvedBlock &block) {
  for (auto &stmt : block.statements)
    generateStmt(stmt);
}
} // namespace syscall