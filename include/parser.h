#ifndef SYSCALL_PARSER_H
#define SYSCALL_PARSER_H

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "ast.h"
#include "lexer.h"

namespace syscall {

class Parser {
  Lexer *lexer;
  Token nextToken;
  bool incompleteAST = false;

  void eatNextToken() { nextToken = lexer->getNextToken(); }
  void synchronize();
  void synchronizeOn(TokenKind kind) {
    incompleteAST = true;

    while (nextToken.kind != kind && nextToken.kind != TokenKind::Eof)
      eatNextToken();
  }

  // AST node parser methods
  std::unique_ptr<FunctionDecl> parseFunctionDecl();
  std::unique_ptr<ParamDecl> parseParamDecl();
  std::unique_ptr<VarDecl> parseVarDecl(bool isLet);

  std::unique_ptr<Stmt> parseStmt();
  std::unique_ptr<IfStmt> parseIfStmt();
  std::unique_ptr<WhileStmt> parseWhileStmt();
  std::unique_ptr<Assignment> parseAssignmentRHS(std::unique_ptr<DeclRefExpr> lhs);
  std::unique_ptr<DeclStmt> parseDeclStmt();
  std::unique_ptr<ReturnStmt> parseReturnStmt();

  std::unique_ptr<Stmt> parseAssignmentOrExpr();

  std::unique_ptr<Block> parseBlock();

  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseExprRHS(std::unique_ptr<Expr> lhs, int precedence);
  std::unique_ptr<Expr> parsePrefixExpr();
  std::unique_ptr<Expr> parsePostfixExpr();
  std::unique_ptr<Expr> parsePrimary();

  // helper methods
  using ParameterList = std::vector<std::unique_ptr<ParamDecl>>;
  std::unique_ptr<ParameterList> parseParameterList();

  using ArgumentList = std::vector<std::unique_ptr<Expr>>;
  std::unique_ptr<ArgumentList> parseArgumentList();

  std::optional<Type> parseType();

public:
  explicit Parser(Lexer &lexer)
      : lexer(&lexer),
        nextToken(lexer.getNextToken()) {}

  std::pair<std::vector<std::unique_ptr<FunctionDecl>>, bool> parseSourceFile();
};

// Implementation of the Parser methods

void Parser::synchronize() {
  // Implement synchronization logic here if needed
}

std::pair<std::vector<std::unique_ptr<FunctionDecl>>, bool> Parser::parseSourceFile() {
  std::vector<std::unique_ptr<FunctionDecl>> functions;

  while (nextToken.kind != TokenKind::Eof) {
    if (nextToken.kind == TokenKind::KwMain) {
      auto function = parseFunctionDecl();
      if (function) {
        functions.push_back(std::move(function));
      }
    } else {
      synchronizeOn(TokenKind::KwMain);
    }
  }

  return {std::move(functions), incompleteAST};
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
  if (nextToken.kind != TokenKind::KwMain) {
    return nullptr;
  }

  eatNextToken(); // consume 'main'

  auto block = parseBlock();
  return std::make_unique<FunctionDecl>("main", std::move(block));
}

std::unique_ptr<Block> Parser::parseBlock() {
  auto block = std::make_unique<Block>();
  
  while (nextToken.kind != TokenKind::Eof && nextToken.kind != TokenKind::Rbrace) {
    auto stmt = parseStmt();
    if (stmt) {
      block->stmts.push_back(std::move(stmt));
    }
  }

  if (nextToken.kind == TokenKind::Rbrace) {
    eatNextToken(); // consume '}'
  }

  return block;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
  switch (nextToken.kind) {
    case TokenKind::KwAdd:
    case TokenKind::KwPrint:
    case TokenKind::KwLog:
      return parseAssignmentOrExpr();
    case TokenKind::KwReturn:
      return parseReturnStmt();
    default:
      return nullptr;
  }
}

std::unique_ptr<Stmt> Parser::parseAssignmentOrExpr() {
  auto lhs = parsePrimary();
  if (!lhs) return nullptr;

  if (nextToken.kind == TokenKind::EqualSign) {
    eatNextToken();
    auto rhs = parseExpr();
    return std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
  }

  return nullptr;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
  eatNextToken(); // consume 'return'
  auto expr = parseExpr();
  return std::make_unique<ReturnStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::parseExpr() {
  return parsePrimary(); // Simplified for demonstration
}

std::unique_ptr<Expr> Parser::parsePrimary() {
  if (nextToken.kind == TokenKind::Number) {
    auto value = nextToken.value;
    eatNextToken();
    return std::make_unique<LiteralExpr>(std::stoi(*value));
  }
  return nullptr;
}

} // namespace syscall

#endif // SYSCALL_PARSER_H
