#include "lexer.h"

namespace {
bool isSpace(char c) {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

bool isAlpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool isNum(char c) { 
    return '0' <= c && c <= '9'; 
}

bool isAlnum(char c) { 
    return isAlpha(c) || isNum(c); 
}
} // namespace

namespace syscall {

Token Lexer::getNextToken() {
    char currentChar = eatNextChar();

    while (isSpace(currentChar))
        currentChar = eatNextChar();

    SourceLocation tokenStartLocation{source->path, line, column};

    // Single character tokens
    for (char c : singleCharTokens) {
        if (c == currentChar)
            return Token{tokenStartLocation, static_cast<TokenKind>(c)};
    }

    // Comments
    if (currentChar == '/') {
        if (peekNextChar() == '/') {
            // Skip the rest of the line
            while (peekNextChar() != '\n' && peekNextChar() != '\0')
                eatNextChar();
            return getNextToken(); // Continue with the next token
        } else {
            return Token{tokenStartLocation, TokenKind::Slash};
        }
    }

    // Operators
    if (currentChar == '=') {
        if (peekNextChar() == '=')
            return Token{tokenStartLocation, TokenKind::EqualEqual};
        else
            return Token{tokenStartLocation, TokenKind::Equal};
    }

    if (currentChar == '&') {
        if (peekNextChar() == '&')
            return Token{tokenStartLocation, TokenKind::AmpAmp};
    }

    if (currentChar == '|') {
        if (peekNextChar() == '|')
            return Token{tokenStartLocation, TokenKind::PipePipe};
    }

    // Identifiers and keywords
    if (isAlpha(currentChar) || currentChar == '@') { // @ indicates a function in Syscall
        std::string value{currentChar};

        while (isAlnum(peekNextChar()) || peekNextChar() == '_')
            value += eatNextChar();

        if (keywords.count(value))
            return Token{tokenStartLocation, keywords.at(value), std::move(value)};

        return Token{tokenStartLocation, TokenKind::Identifier, std::move(value)};
    }

    // Numeric literals
    if (isNum(currentChar)) {
        std::string value{currentChar};

        while (isNum(peekNextChar()))
            value += eatNextChar();

        if (peekNextChar() == '.') {
            value += eatNextChar(); // Consume '.'
            if (isNum(peekNextChar())) {
                while (isNum(peekNextChar()))
                    value += eatNextChar();
            } else {
                return Token{tokenStartLocation, TokenKind::Unk}; // Invalid number
            }
        }

        return Token{tokenStartLocation, TokenKind::Number, value};
    }

    // Handle unexpected characters
    return Token{tokenStartLocation, TokenKind::Unk};
}
} // namespace syscall
