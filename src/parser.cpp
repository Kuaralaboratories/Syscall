#include <cassert>
#include <memory>
#include <vector>
#include <optional>

#include "parser.h"
#include "utils.h"

#define matchOrReturn(tok, msg)                                                \
  if (nextToken.kind != tok)                                                   \
    return report(nextToken.location, msg);

namespace syscall {
namespace {

int getTokPrecedence(TokenKind tok) {
  switch (tok) {
  case TokenKind::Asterisk:
  case TokenKind::Slash:
    return 6;
  case TokenKind::Plus:
  case TokenKind::Minus:
    return 5;
  case TokenKind::Lt:
  case TokenKind::Gt:
    return 4;
  case TokenKind::EqualEqual:
    return 3;
  case TokenKind::AmpAmp:
    return 2;
  case TokenKind::PipePipe:
    return 1;
  default:
    return -1;
  }
}

}; // namespace

void Parser::synchronize() {
  incompleteAST = true;

  int braces = 0;
  while (true) {
    TokenKind kind = nextToken.kind;

    if (kind == TokenKind::Lbrace) {
      ++braces;
    } else if (kind == TokenKind::Rbrace) {
      if (braces == 0)
        break;

      if (braces == 1) {
        eatNextToken(); // eat '}'
        break;
      }

      --braces;
    } else if (kind == TokenKind::Semi && braces == 0) {
      eatNextToken(); // eat ';'
      break;
    } else if (kind == TokenKind::KwFunction || kind == TokenKind::Eof)
      break;

    eatNextToken();
  }
}

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat function

  matchOrReturn(TokenKind::Identifier, "expected identifier");

  assert(nextToken.value && "identifier token without value");
  std::string functionIdentifier = *nextToken.value;
  eatNextToken(); // eat identifier

  varOrReturn(parameterList, parseParameterList());

  matchOrReturn(TokenKind::Colon, "expected ':'");
  eatNextToken(); // eat ':'

  varOrReturn(type, parseType());

  matchOrReturn(TokenKind::Lbrace, "expected function body");
  varOrReturn(block, parseBlock());

  return std::make_unique<FunctionDecl>(location, functionIdentifier, *type,
                                        std::move(*parameterList),
                                        std::move(block));
}

std::unique_ptr<ParamDecl> Parser::parseParamDecl() {
  SourceLocation location = nextToken.location;
  assert(nextToken.value && "identifier token without value");

  std::string identifier = *nextToken.value;
  eatNextToken(); // eat identifier

  matchOrReturn(TokenKind::Colon, "expected ':'");
  eatNextToken(); // eat :

  varOrReturn(type, parseType());

  return std::make_unique<ParamDecl>(location, std::move(identifier),
                                     std::move(*type));
}

std::unique_ptr<VarDecl> Parser::parseVarDecl(bool isLet) {
  SourceLocation location = nextToken.location;

  assert(nextToken.value && "identifier token without value");

  std::string identifier = *nextToken.value;
  eatNextToken(); // eat identifier

  std::optional<Type> type;
  if (nextToken.kind == TokenKind::Colon) {
    eatNextToken(); // eat ':'

    type = parseType();
    if (!type)
      return nullptr;
  }

  if (nextToken.kind != TokenKind::Equal)
    return std::make_unique<VarDecl>(location, identifier, type, !isLet);
  eatNextToken(); // eat '='

  varOrReturn(initializer, parseExpr());

  return std::make_unique<VarDecl>(location, identifier, type, !isLet,
                                   std::move(initializer));
}

std::unique_ptr<Block> Parser::parseBlock() {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat '{'

  std::vector<std::unique_ptr<Stmt>> statements;
  while (true) {
    if (nextToken.kind == TokenKind::Rbrace)
      break;

    if (nextToken.kind == TokenKind::Eof || nextToken.kind == TokenKind::KwFunction)
      return report(nextToken.location, "expected '}' at the end of a block");

    std::unique_ptr<Stmt> stmt = parseStmt();
    if (!stmt) {
      synchronize();
      continue;
    }

    statements.emplace_back(std::move(stmt));
  }

  eatNextToken(); // eat '}'

  return std::make_unique<Block>(location, std::move(statements));
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat 'if'

  varOrReturn(condition, parseExpr());

  matchOrReturn(TokenKind::Lbrace, "expected 'if' body");

  varOrReturn(trueBlock, parseBlock());

  if (nextToken.kind != TokenKind::KwElse)
    return std::make_unique<IfStmt>(location, std::move(condition),
                                    std::move(trueBlock));
  eatNextToken(); // eat 'else'

  std::unique_ptr<Block> falseBlock;
  if (nextToken.kind == TokenKind::KwIf) {
    varOrReturn(elseIf, parseIfStmt());

    SourceLocation loc = elseIf->location;
    std::vector<std::unique_ptr<Stmt>> stmts;
    stmts.emplace_back(std::move(elseIf));

    falseBlock = std::make_unique<Block>(loc, std::move(stmts));
  } else {
    matchOrReturn(TokenKind::Lbrace, "expected 'else' body");
    falseBlock = parseBlock();
  }

  if (!falseBlock)
    return nullptr;

  return std::make_unique<IfStmt>(location, std::move(condition),
                                  std::move(trueBlock), std::move(falseBlock));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat 'while'

  varOrReturn(cond, parseExpr());

  matchOrReturn(TokenKind::Lbrace, "expected 'while' body");
  varOrReturn(body, parseBlock());

  return std::make_unique<WhileStmt>(location, std::move(cond),
                                     std::move(body));
}

std::unique_ptr<Assignment>
Parser::parseAssignmentRHS(std::unique_ptr<DeclRefExpr> lhs) {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat '='

  varOrReturn(rhs, parseExpr());

  return std::make_unique<Assignment>(location, std::move(lhs), std::move(rhs));
}

std::unique_ptr<DeclStmt> Parser::parseDeclStmt() {
  Token tok = nextToken;
  eatNextToken(); // eat 'let' | 'var'

  matchOrReturn(TokenKind::Identifier, "expected identifier");
  varOrReturn(varDecl, parseVarDecl(tok.kind == TokenKind::KwLet));

  matchOrReturn(TokenKind::Semi, "expected ';' after declaration");
  eatNextToken(); // eat ';'

  return std::make_unique<DeclStmt>(tok.location, std::move(varDecl));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
  SourceLocation location = nextToken.location;
  eatNextToken(); // eat 'return'

  std::unique_ptr<Expr> expr;
  if (nextToken.kind != TokenKind::Semi) {
    expr = parseExpr();
    if (!expr)
      return nullptr;
  }

  matchOrReturn(TokenKind::Semi, "expected ';' at the end of a return statement");
  eatNextToken(); // eat ';'

  return std::make_unique<ReturnStmt>(location, std::move(expr));
}

std::unique_ptr<Stmt> Parser::parseStmt() {
  if (nextToken.kind == TokenKind::KwIf)
    return parseIfStmt();

  if (nextToken.kind == TokenKind::KwWhile)
    return parseWhileStmt();

  if (nextToken.kind == TokenKind::KwReturn)
    return parseReturnStmt();

  if (nextToken.kind == TokenKind::KwLet || nextToken.kind == TokenKind::KwVar)
    return parseDeclStmt();

  return parseAssignmentOrExpr();
}

std::unique_ptr<Stmt> Parser::parseAssignmentOrExpr() {
  varOrReturn(lhs, parsePrefixExpr());

  if (nextToken.kind != TokenKind::Equal) {
    varOrReturn(expr, parseExprRHS(std::move(lhs), 0));

    matchOrReturn(TokenKind::Semi, "expected ';' at the end of expression");
    eatNextToken(); // eat ';'

    return expr;
  }

  auto *dre = dynamic_cast<DeclRefExpr *>(lhs.get());
  if (!dre)
    return report(lhs->location,
                  "expected variable on the LHS of an assignment");

  std::ignore = lhs.release();

  varOrReturn(assignment,
              parseAssignmentRHS(std::unique_ptr<DeclRefExpr>(dre)));

  matchOrReturn(TokenKind::Semi, "expected ';' at the end of assignment");
  eatNextToken(); // eat ';'

  return assignment;
}

std::unique_ptr<Expr> Parser::parseExpr() {
  return parseExprRHS(parsePrefixExpr(), 0);
}

std::unique_ptr<Expr> Parser::parseExprRHS(std::unique_ptr<Expr> lhs, int exprPrec) {
  while (true) {
    int tokPrec = getTokPrecedence(nextToken.kind);

    if (tokPrec < exprPrec)
      return lhs;

    TokenKind binOp = nextToken.kind;
    SourceLocation binOpLoc = nextToken.location;
    eatNextToken(); // eat binary operator

    std::unique_ptr<Expr> rhs = parsePrefixExpr();
    if (!rhs)
      return nullptr;

    int nextPrec = getTokPrecedence(nextToken.kind);
    if (tokPrec < nextPrec) {
      rhs = parseExprRHS(std::move(rhs), tokPrec + 1);
      if (!rhs)
        return nullptr;
    }

    lhs = std::make_unique<BinaryExpr>(binOpLoc, binOp, std::move(lhs),
                                        std::move(rhs));
  }
}

std::unique_ptr<Expr> Parser::parsePrefixExpr() {
  TokenKind kind = nextToken.kind;
  SourceLocation location = nextToken.location;

  if (kind == TokenKind::Identifier) {
    eatNextToken(); // eat identifier
    return std::make_unique<DeclRefExpr>(location, *nextToken.value);
  }

  if (kind == TokenKind::IntegerLiteral) {
    eatNextToken(); // eat integer literal
    return std::make_unique<IntegerLiteral>(location, *nextToken.value);
  }

  if (kind == TokenKind::Lparen) {
    eatNextToken(); // eat '('

    varOrReturn(expr, parseExpr());

    matchOrReturn(TokenKind::Rparen, "expected ')' after expression");
    eatNextToken(); // eat ')'

    return expr;
  }

  return nullptr;
}

std::unique_ptr<Type> Parser::parseType() {
  if (nextToken.kind == TokenKind::Identifier) {
    assert(nextToken.value && "identifier token without value");

    auto type = std::make_unique<Type>();
    type->name = *nextToken.value;
    eatNextToken(); // eat identifier

    return type;
  }

  return nullptr;
}

std::unique_ptr<std::vector<std::unique_ptr<ParamDecl>>>
Parser::parseParameterList() {
  SourceLocation location = nextToken.location;

  matchOrReturn(TokenKind::Lparen, "expected '(' at the start of parameter list");
  eatNextToken(); // eat '('

  std::vector<std::unique_ptr<ParamDecl>> params;
  if (nextToken.kind != TokenKind::Rparen) {
    do {
      varOrReturn(param, parseParamDecl());
      params.emplace_back(std::move(param));

      if (nextToken.kind == TokenKind::Comma)
        eatNextToken(); // eat ','
      else
        break;
    } while (true);
  }

  matchOrReturn(TokenKind::Rparen, "expected ')' at the end of parameter list");
  eatNextToken(); // eat ')'

  return std::make_unique<std::vector<std::unique_ptr<ParamDecl>>>(std::move(params));
}

} // namespace syscall
