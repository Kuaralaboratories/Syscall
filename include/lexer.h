#ifndef SYSCALL_LEXER_H
#define SYSCALL_LEXER_H

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <cctype>

#include "utils.h"

namespace syscall {
constexpr char singleCharTokens[] = {'\0', '(', ')', '{', '}', ':', ';',
                                     ',',  '+', '-', '*', '<', '>', '!', '='};

enum class TokenKind : char {
  Unk = -128,
  Slash,

  Equal,
  EqualEqual,
  AmpAmp,
  PipePipe,

  Identifier,
  Number,

  KwMain,
  KwAdd,
  KwPrint,
  KwLog,
  KwReturn,

  Eof = singleCharTokens[0],
  Lpar = singleCharTokens[1],
  Rpar = singleCharTokens[2],
  Lbrace = singleCharTokens[3],
  Rbrace = singleCharTokens[4],
  Colon = singleCharTokens[5],
  Semi = singleCharTokens[6],
  Comma = singleCharTokens[7],
  Plus = singleCharTokens[8],
  Minus = singleCharTokens[9],
  Asterisk = singleCharTokens[10],
  Lt = singleCharTokens[11],
  Gt = singleCharTokens[12],
  Excl = singleCharTokens[13],
  EqualSign = singleCharTokens[14]
};

const std::unordered_map<std::string_view, TokenKind> keywords = {
    {"main", TokenKind::KwMain}, {"add", TokenKind::KwAdd},
    {"print", TokenKind::KwPrint}, {"log", TokenKind::KwLog},
    {"return", TokenKind::KwReturn}};

struct Token {
  SourceLocation location;
  TokenKind kind;
  std::optional<std::string> value = std::nullopt;
};

class Lexer {
  const SourceFile *source;
  size_t idx = 0;

  int line = 1;
  int column = 0;

  char peekNextChar() const { return source->buffer[idx]; }
  char eatNextChar() {
    assert(idx <= source->buffer.size() &&
           "indexing past the end of the source buffer");

    ++column;

    if (source->buffer[idx] == '\n') {
      ++line;
      column = 0;
    }

    return source->buffer[idx++];
  }

  bool isAtEnd() const { return idx >= source->buffer.size(); }

  Token makeToken(TokenKind kind, std::optional<std::string> value = std::nullopt) {
    return { {source->path, line, column}, kind, std::move(value) };
  }

public:
  explicit Lexer(const SourceFile &source)
      : source(&source) {}
  Token getNextToken();
};

Token Lexer::getNextToken() {
  while (!isAtEnd() && std::isspace(peekNextChar())) {
    eatNextChar();
  }

  if (isAtEnd()) {
    return makeToken(TokenKind::Eof);
  }

  char currentChar = eatNextChar();
  
  // Handle single character tokens
  for (char tokenChar : singleCharTokens) {
    if (currentChar == tokenChar) {
      return makeToken(static_cast<TokenKind>(currentChar));
    }
  }

  // Handle identifiers and keywords
  if (std::isalpha(currentChar)) {
    std::string identifier(1, currentChar);
    while (!isAtEnd() && std::isalnum(peekNextChar())) {
      identifier += eatNextChar();
    }
    auto keyword = keywords.find(identifier);
    if (keyword != keywords.end()) {
      return makeToken(keyword->second, identifier);
    }
    return makeToken(TokenKind::Identifier, identifier);
  }

  // Handle numbers
  if (std::isdigit(currentChar)) {
    std::string number(1, currentChar);
    while (!isAtEnd() && std::isdigit(peekNextChar())) {
      number += eatNextChar();
    }
    return makeToken(TokenKind::Number, number);
  }

  return makeToken(TokenKind::Unk);
}

} // namespace syscall

#endif // SYSCALL_LEXER_H
