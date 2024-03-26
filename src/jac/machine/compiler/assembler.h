#pragma once

#include <cctype>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


namespace jac::as {


struct Token {
    enum Kind {
        NoToken = 0,
        Section,
        Label,
        Instruction,
        Register,
        Immediate,
        Comma,
        Newline,
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

    bool operator==(const Token& other) const {
        return line == other.line
            && column == other.column
            && text == other.text
            && kind == other.kind;
    }
};



class Scanner {
    std::string_view _input;
    std::function<void(int, int, std::string)> _report;

    std::string_view::iterator _pos;
    size_t _line = 1;
    size_t _column = 1;

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

    bool isNameChar(char c) {
        return std::isalnum(c) || c == '_';
    }

    Token scanSection() {
        int startColumn = _column;

        if (!match('.')) {
            return Token(_line, _column, "", Token::NoToken);
        }

        auto start = _pos;
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }

        return Token(_line, startColumn, std::string_view(start, _pos), Token::Section);
    }

    Token scanComment() {
        int startColumn = _column;

        if (!match('/')) {
            return Token(_line, _column, "", Token::NoToken);
        }

        if (match('/')) {
            auto start = _pos;
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
            return Token(_line, startColumn, std::string_view(start, _pos), Token::Comment);
        }

        int startRow = _line;
        if (match('*')) {
            auto start = _pos;
            while (!isAtEnd()) {
                if (match('*') && match('/')) {
                    return Token(startRow, startColumn, std::string_view(start, _pos - 2), Token::Comment);
                }
                advance();
            }
            report("Unterminated comment");
            return Token(_line, _column, "", Token::Invalid);
        }

        return Token(_line, _column, "", Token::Invalid);
    }

    Token scanComma() {
        int startColumn = _column;

        if (match(',')) {
            return Token(_line, startColumn, ",", Token::Comma);
        }
        return Token(_line, _column, "", Token::NoToken);
    }

    Token scanNewline() {
        int startColumn = _column;
        int startRow = _line;
        if (match('\n')) {
            return Token(startRow, startColumn, "\n", Token::Newline);
        }
        return Token(_line, _column, "", Token::NoToken);
    }

    Token scanRegister() {
        int startColumn = _column;

        if (!match('%')) {
            return Token(_line, _column, "", Token::NoToken);
        }
        auto start = _pos;
        while (std::isalnum(peek())) {
            advance();
        }
        return Token(_line, startColumn, std::string_view(start, _pos), Token::Register);
    }

    Token scanInstructionOrLabel() {
        int startColumn = _column;

        auto start = _pos;
        while (isNameChar(peek())) {
            advance();
        }

        if (start == _pos) {
            return Token(_line, _column, "", Token::NoToken);
        }

        if (match(':')) {
            return Token(_line, startColumn, std::string_view(start, _pos - 1), Token::Label);
        }

        return Token(_line, startColumn, std::string_view(start, _pos), Token::Instruction);
    }

    Token scanImmediate() {
        int startColumn = _column;

        if (!match('$')) {
            return Token(_line, _column, "", Token::NoToken);
        }

        auto start = _pos;
        while (std::isdigit(peek()) || peek() == '-') {
            advance();
        }

        return Token(_line, startColumn, std::string_view(start, _pos), Token::Immediate);
    }

    void skipWhitespace() {
        while (!isAtEnd() && std::isspace(peek()) && peek() != '\n') {
            advance();
        }
    }

    Token scanToken() {
        skipWhitespace();
        if (isAtEnd()) {
            return Token(_line, _column, "", Token::NoToken);
        }

        if (Token token = scanSection(); token) {
            return token;
        }
        if (Token token = scanInstructionOrLabel(); token) {
            return token;
        }
        if (Token token = scanRegister(); token) {
            return token;
        }
        if (Token token = scanImmediate(); token) {
            return token;
        }
        if (Token token = scanComma(); token) {
            return token;
        }
        if (Token token = scanComment(); token) {
            return token;
        }
        if (Token token = scanNewline(); token) {
            return token;
        }

        report("Unknow token");
        return Token(_line, _column, "", Token::Invalid);
    }

public:
    Scanner(std::string_view input, std::function<void(int, int, std::string)> report):
        _input(input), _report(report), _pos(_input.begin())
    {}

    Token next(bool skipComments = true) {
        while (true) {
            Token token = scanToken();
            if (skipComments && token.kind == Token::Comment) {
                continue;
            }
            return token;
        }
    }
};


} // namespace jac::as


namespace jac::as::x86_64 {


class Assembler {
    std::string_view _code;
    std::unordered_map<std::string, size_t> _labels;

    std::vector<uint8_t> _output;
public:
    Assembler(std::string_view code) : _code(code) {}

    void assemble() {

    }
};


} // namespace jac::as::x86_64
