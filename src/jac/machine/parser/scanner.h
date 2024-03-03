#pragma once

#include <functional>
#include <string>

#include "language.h"


namespace jac {


struct Token {
    enum Kind {
        NoToken = 0,  // used as a sentinel
        IdentifierName,
        Keyword,
        Punctuator,
        NumericLiteral,
        StringLiteral,
        Template,
        Comment,
        Invalid
    };

    int line;
    int column;
    std::string_view text;
    Kind kind;

    Token(int line_, int column_, std::string_view text_, Kind kind_):
        line(line_), column(column_), text(text_), kind(kind_)
    {}

    operator bool() const {
        return kind != NoToken;
    }
};


class Scanner {
    std::string_view _input;
    std::function<void(int, int, std::string)> _report;

    std::string_view::iterator _pos;
    int _line;
    int _column;

    void report(std::string message) {
        _report(_line, _column, message);
    }

    bool isAtEnd() const {
        return _pos == _input.end();
    }

    char advance() {
        if (isAtEnd()) {
            return '\0';
        }

        char c = *_pos;
        if (c == '\n') {
            // FIXME: handle different LineTerminatorSequence
            _line++;
            _column = 1;
        }
        else {
            _column++;
        }
        ++_pos;

        return c;
    }

    char peek(int offset = 0) const {
        if (_pos + offset < _input.end()) {
            return *(_pos + offset);
        }
        return '\0';
    }

    bool match(char c) {
        if (peek() == c) {
            advance();
            return true;
        }
        return false;
    }

    int matchPrefix(std::string_view chars) {
        for (std::size_t i = 0; i < chars.size(); i++) {
            if (!match(chars[i])) {
                return i;
            }
        }
        return chars.size();
    }

    char matchAny(std::string_view chars) {
        // TODO: optimize
        char c = peek();
        if (chars.find(c) != std::string_view::npos) {
            advance();
            return c;
        }
        return '\0';
    }

    Token scanPunctuator() {
        int startColumn = _column;
        std::string_view::iterator start = _pos;


        if (matchAny("{}()[];,:~")) {
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (int num = matchPrefix("...")) {
            if (num == 2) {
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (match('=')) {
            if (match('>')) {
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
            }
            matchPrefix("==");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (matchPrefix("!==")) {
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (match('+')) {
            matchAny("+=");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (match('-')) {
            matchAny("-=");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (matchPrefix("<<") || matchPrefix(">>>") || match('%') || matchPrefix("**")
         || matchPrefix("&&") || matchPrefix("||")  || match('^')) {
            match('=');
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }
        if (match('?')) {
            if (match('.')) {
                if (std::isdigit(peek())) {
                    report("Unexpected number after ?.");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
            }
            matchPrefix("?=");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
        }

        if (start != _pos) {
            report("Unknown punctuator token");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
        }
        return Token(_line, startColumn, std::string_view(start, _pos), Token::NoToken);
    }

    Token scanStringLiteral() {
        int startColumn = _column;
        std::string_view::iterator start = _pos;

        char quote = matchAny("\"'`");
        if (quote == '\0') {
            return Token(_line, startColumn, std::string_view(start, _pos), Token::NoToken);
        }

        while (!isAtEnd()) {
            char c = advance();
            if (c == quote) {
                return Token(_line, startColumn, std::string_view(start, _pos), Token::StringLiteral);
            }
            if (c == '\\') {
                if (isAtEnd()) {
                    report("Unterminated string literal");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                // should match any EscapeSequence or LineTerminatorSequence
                advance();
            }
            if (c == '\n') {
                report("Unterminated string literal");
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
        }

        report("Unterminated string literal");
        return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
    }

    Token scanSlash() {
        /**
         * match different sequences starting with /
         * - RegExpLiteral is not implemented
         */

        int startColumn = _column;
        std::string_view::iterator start = _pos;

        if (!match('/')) {
            return Token(_line, startColumn, std::string_view(start, _pos), Token::NoToken);
        }

        if (match('/')) {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Comment);
        }

        if (match('*')) {
            while (!isAtEnd()) {
                if (matchPrefix("*/") == 2) {
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Comment);
                }
                advance();
            }
            report("Unterminated block comment");
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
        }

        match('=');
        return Token(_line, startColumn, std::string_view(start, _pos), Token::Punctuator);
    }

    bool matchNumericSuffix() {
        char c;
        bool match = false;
        while ((c = peek()) && (std::isdigit(c) || std::isalpha(c) || c == '_')) {
            match = true;
            advance();
        }
        return match;
    }

    Token scanNumericLiteral() {
        int startColumn = _column;
        std::string_view::iterator start = _pos;

        bool leadingZero = false;
        bool lastSeparator = false;

        if ((leadingZero = match('0'))) {
            // TODO: remove duplication
            if (match('x') || match('X')) {
                if (!std::isxdigit(peek())) {
                    report("Hexadecimal number must have at least one digit");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                while (std::isxdigit(peek()) || (lastSeparator = match('_'))) {
                    if (lastSeparator && !std::isxdigit(peek())) {
                        report("Invalid separator in hexadecimal number");
                        return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                    }
                    lastSeparator = false;
                    advance();
                }
                matchNumericSuffix();
                return Token(_line, startColumn, std::string_view(start, _pos), Token::NumericLiteral);
            }
            if (match('o') || match('O')) {
                if (!std::isdigit(peek()) || peek() > '7') {
                    report("Octal number must have at least one digit and be less than 8");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                while ((std::isdigit(peek()) && peek() <= '7') || (lastSeparator = match('_'))) {
                    if (lastSeparator && (peek() > '7' || !std::isdigit(peek()))) {
                        report("Invalid separator in octal number");
                        return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                    }
                    lastSeparator = false;
                    advance();
                }
                matchNumericSuffix();
                return Token(_line, startColumn, std::string_view(start, _pos), Token::NumericLiteral);
            }
            if (match('b') || match('B')) {
                if (peek() != '0' && peek() != '1') {
                    report("Binary number must have at least one digit and be 0 or 1");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                while (peek() == '0' || peek() == '1' || (lastSeparator = match('_'))) {
                    if (lastSeparator && (peek() != '0' && peek() != '1')) {
                        report("Invalid separator in binary number");
                        return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                    }
                    lastSeparator = false;
                    advance();
                }
                matchNumericSuffix();
                return Token(_line, startColumn, std::string_view(start, _pos), Token::NumericLiteral);
            }
        }

        bool isLegacyOctal = leadingZero && std::isdigit(peek());

        if (!leadingZero && !std::isdigit(peek()) && peek() != '.') {
            // First character of the literal
            return Token(_line, startColumn, std::string_view(start, _pos), Token::NoToken);
        }

        lastSeparator = true;
        while (std::isdigit(peek()) || (!leadingZero && (lastSeparator = match('_')))) {
            if (lastSeparator && !std::isdigit(peek())) {
                report("Invalid separator in decimal number");
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
            lastSeparator = false;
            isLegacyOctal = isLegacyOctal && peek() < '8';
            advance();
        }

        if (isLegacyOctal) {
            // TODO: check whether the next character is valid (only Punctuator, whitespace?)
            return Token(_line, startColumn, std::string_view(start, _pos), Token::NumericLiteral);
        }

        if (match('.')) {
            if ((start + 1) == _pos && !std::isdigit(peek())) {
                // Dot without preceding and following digits
                // TODO: return Punctuator at this point?
                return Token(_line, startColumn, std::string_view(start, _pos), Token::NoToken);
            }
        }

        while (std::isdigit(peek()) || (lastSeparator = match('_'))) {
            if (lastSeparator && !std::isdigit(peek())) {
                report("Invalid separator in decimal number");
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
            lastSeparator = false;
            advance();
        }

        if (match('e') || match('E')) {
            matchAny("+-");
            if (!std::isdigit(peek())) {
                report("Exponent part must have at least one digit");
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
            while (std::isdigit(peek()) || (lastSeparator = match('_'))) {
                if (lastSeparator && !std::isdigit(peek())) {
                    report("Invalid separator in exponent part");
                    return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
                }
                lastSeparator = false;
                advance();
            }
        }
        else {
            matchNumericSuffix();
        }
        return Token(_line, startColumn, std::string_view(start, _pos), Token::NumericLiteral);
    }

    Token scanIdentifier() {
        int startColumn = _column;
        std::string_view::iterator start = _pos;

        bool isPrivate = match('#');

        if (!std::isalpha(peek()) && peek() != '$' && peek() != '_') {
            if (isPrivate) {
                report("Private identifier must have at least one identifier character");
                return Token(_line, startColumn, std::string_view(start, _pos), Token::Invalid);
            }
            return Token(_line, _column, std::string_view(_pos, _pos), Token::NoToken);
        }

        while (std::isalnum(peek()) || peek() == '$' || peek() == '_') {
            advance();
        }

        if (keywords.contains(std::string_view(start, _pos))) {
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Keyword);
        }

        return Token(_line, startColumn, std::string_view(start, _pos), Token::IdentifierName);
    }

    void skipWhitespace() {
        while (!isAtEnd() && std::isspace(peek())) {
            advance();
        }
    }

public:
    Scanner(std::string_view input, std::function<void(int, int, std::string)> report):
        _input(input), _report(report), _pos(input.begin()), _line(1), _column(1) {}

    std::vector<Token> scan() {
        std::vector<Token> _tokens;

        skipWhitespace();
        while (!isAtEnd()) {
            Token tok = scanToken();
            skipWhitespace();
            if (tok.kind == Token::Comment) {
                continue;
            }
            _tokens.push_back(tok);
            if (!_tokens.back()) {
                break;
            }
        }

        return _tokens;
    }

    Token scanToken() {
        if (Token token = scanNumericLiteral()) {
            return token;
        }
        if (Token token = scanStringLiteral()) {
            return token;
        }
        if (Token token = scanPunctuator()) {
            return token;
        }
        if (Token token = scanSlash()) {
            return token;
        }
        if (Token token = scanIdentifier()) {
            return token;
        }

        report("Unknown token");
        return Token(_line, _column, std::string_view(_pos, _pos), Token::Invalid);
    }
};


} // namespace jac
