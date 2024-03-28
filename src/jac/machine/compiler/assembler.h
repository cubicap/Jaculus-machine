#pragma once

#include <cctype>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
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

    unsigned long long toInt() const {
        // FIXME: edge values
        return std::stoull(std::string(text));
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

        report("Unknown token");
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


class Assembler;


class TokenStream {
    Scanner& _scanner;
    std::optional<Token> _peeked;
public:
    TokenStream(Scanner& scanner):
        _scanner(scanner)
    {}

    Token next() {
        peek();
        Token token = _peeked.value();
        _peeked.reset();
        return token;
    }

    Token peek() {
        if (!_peeked) {
            _peeked = _scanner.next();
        }
        return _peeked.value();
    }

    Token match(Token::Kind kind) {
        if (peek().kind != kind) {
            return Token(0, 0, "", Token::NoToken);
        }

        return next();
    }
};


class InstructionStream {
    std::vector<uint8_t>& _output;
public:
    InstructionStream(std::vector<uint8_t>& output):
        _output(output)
    {}

    void emit(uint8_t byte) {
        _output.push_back(byte);
    }
};


enum class RegSize {
    R8 = 8,
    R16 = 16,
    R32 = 32,
    R64 = 64
};


using Emitter = std::function<void(std::span<Token>, InstructionStream&)>;

const std::unordered_map<std::string_view, std::pair<RegSize, uint8_t>> registers{
    { "rax", { RegSize::R64, 0 } },
    { "rcx", { RegSize::R64, 1 } },
    { "rdx", { RegSize::R64, 2 } },
    { "rbx", { RegSize::R64, 3 } },
    { "rsp", { RegSize::R64, 4 } },
    { "rbp", { RegSize::R64, 5 } },
    { "rsi", { RegSize::R64, 6 } },
    { "rdi", { RegSize::R64, 7 } },
    { "r8", { RegSize::R64, 8 } },
    { "r9", { RegSize::R64, 9 } },
    { "r10", { RegSize::R64, 10 } },
    { "r11", { RegSize::R64, 11 } },
    { "r12", { RegSize::R64, 12 } },
    { "r13", { RegSize::R64, 13 } },
    { "r14", { RegSize::R64, 14 } },
    { "r15", { RegSize::R64, 15 } },

    { "eax", { RegSize::R32, 0 } },
    { "ecx", { RegSize::R32, 1 } },
    { "edx", { RegSize::R32, 2 } },
    { "ebx", { RegSize::R32, 3 } },
    { "esp", { RegSize::R32, 4 } },
    { "ebp", { RegSize::R32, 5 } },
    { "esi", { RegSize::R32, 6 } },
    { "edi", { RegSize::R32, 7 } },
    { "r8d", { RegSize::R32, 8 } },
    { "r9d", { RegSize::R32, 9 } },
    { "r10d", { RegSize::R32, 10 } },
    { "r11d", { RegSize::R32, 11 } },
    { "r12d", { RegSize::R32, 12 } },
    { "r13d", { RegSize::R32, 13 } },
    { "r14d", { RegSize::R32, 14 } },
    { "r15d", { RegSize::R32, 15 } },

    { "ax", { RegSize::R16, 0 } },
    { "cx", { RegSize::R16, 1 } },
    { "dx", { RegSize::R16, 2 } },
    { "bx", { RegSize::R16, 3 } },
    { "sp", { RegSize::R16, 4 } },
    { "bp", { RegSize::R16, 5 } },
    { "si", { RegSize::R16, 6 } },
    { "di", { RegSize::R16, 7 } },

    { "al", { RegSize::R8, 0 } },
    { "cl", { RegSize::R8, 1 } },
    { "dl", { RegSize::R8, 2 } },
    { "bl", { RegSize::R8, 3 } },
    { "ah", { RegSize::R8, 4 } },
    { "ch", { RegSize::R8, 5 } },
    { "dh", { RegSize::R8, 6 } },
    { "bh", { RegSize::R8, 7 } }
};


template<RegSize size, RegSize maxSize>
void emitImmediate(InstructionStream& is, unsigned long long value) {
    int iterations = size <= maxSize ? static_cast<int>(size) / 8 : static_cast<int>(maxSize) / 8;
    for (int i = 0; i < iterations; i++) {
        is.emit(value & 0xFF);
        value >>= 8;
    }
}


template<RegSize size>
void emitMov(std::span<Token> toks, InstructionStream& is) {
    auto a = toks[0];
    auto b = toks[1];

    auto [sizeA, codeA] = registers.at(a.text);
    auto [sizeB, codeB] = registers.at(b.text);
    if (sizeA != size || sizeB != size) {
        throw std::runtime_error("Register sizes do not match in 'mov'");
    }
    if constexpr (size == RegSize::R8) {
        is.emit(0x88);
    }
    else if constexpr (size == RegSize::R16) {
        is.emit(0x66);
        is.emit(0x89);
    }
    else if constexpr (size == RegSize::R32) {
        is.emit(0x89);
    }
    else if constexpr (size == RegSize::R64) {
        is.emit(0x48);
        is.emit(0x89);
    }

    is.emit(0xC0 + codeA * 8 + codeB);
};


template<RegSize size>
void emitPush(std::span<Token> toks, InstructionStream& is) {
    auto [sizeA, code] = registers.at(toks[0].text);
    if (sizeA != size) {
        throw std::runtime_error("Register size does not match 'push' size");
    }
    if (code < 8) {
        is.emit(0x50 + code);
    }
    else {
        throw std::runtime_error("only registers <8 are supported in 'push'");
    }
}

template<RegSize size>
void emitAddRR(std::span<Token> toks, InstructionStream& is) {
    auto [sizeA, codeA] = registers.at(toks[0].text);
    auto [sizeB, codeB] = registers.at(toks[1].text);
    if (sizeA != size || sizeB != size) {
        throw std::runtime_error("Register sizes do not match");
    }
    if constexpr (size == RegSize::R8) {
        is.emit(0x00);
    }
    else if constexpr (size == RegSize::R16) {
        is.emit(0x66);
        is.emit(0x01);
    }
    else if constexpr (size == RegSize::R32) {
        is.emit(0x01);
    }
    else if constexpr (size == RegSize::R64) {
        is.emit(0x48);
        is.emit(0x01);
    }

    is.emit(0xC0 + codeA * 8 + codeB);
}

template<RegSize size>
void emitAddIR(std::span<Token> toks, InstructionStream& is) {
    auto imm = toks[0];

    auto [sizeB, codeB] = registers.at(toks[1].text);
    if (sizeB != size) {
        throw std::runtime_error("Register sizes do not match");
    }
    if (size != RegSize::R8 && codeB == 0) {
        if constexpr (size == RegSize::R16) {
            is.emit(0x66);
            is.emit(0x05);
        }
        else if constexpr (size == RegSize::R32) {
            is.emit(0x05);
        }
        else if constexpr (size == RegSize::R64) {
            throw std::runtime_error("'add imm, rax' is not supported");
        }

        emitImmediate<size, RegSize::R32>(is, imm.toInt());

        return;
    }
    if constexpr (size == RegSize::R8) {
        is.emit(0x80);
    }
    else if constexpr (size == RegSize::R16) {
        is.emit(0x66);
        is.emit(0x81);
    }
    else if constexpr (size == RegSize::R32) {
        is.emit(0x81);
    }
    else if constexpr (size == RegSize::R64) {
        is.emit(0x48);
        is.emit(0x81);
    }

    is.emit(0xC0 + codeB);
    emitImmediate<size, RegSize::R32>(is, imm.toInt());
}

template<RegSize size>
void emitImulRR(std::span<Token> toks, InstructionStream& is) {
    auto regA = toks[0];
    auto regB = toks[1];

    auto [sizeA, codeA] = registers.at(regA.text);
    auto [sizeB, codeB] = registers.at(regB.text);
    if (sizeA != size || sizeB != size) {
        throw std::runtime_error("Register sizes do not match");
    }

    if constexpr (size == RegSize::R8) {
        throw std::runtime_error("'imul' does not support 8-bit registers");
    }
    else if constexpr (size == RegSize::R16) {
        is.emit(0x66);
        is.emit(0x0F);
        is.emit(0xAF);
    }
    else if constexpr (size == RegSize::R32) {
        is.emit(0x0F);
        is.emit(0xAF);
    }
    else if constexpr (size == RegSize::R64) {
        is.emit(0x48);
        is.emit(0x0F);
        is.emit(0xAF);
    }

    is.emit(0xC0 + codeB * 8 + codeA);
};

template<RegSize size>
void emitImulIRR(std::span<Token> toks, InstructionStream& is) {
    auto imm = toks[0];
    auto regA = toks[1];
    auto regB = toks[2];

    auto [sizeA, codeA] = registers.at(regA.text);
    auto [sizeB, codeB] = registers.at(regB.text);
    if (sizeB != size || sizeA != size) {
        throw std::runtime_error("Register sizes do not match");
    }

    if constexpr (size == RegSize::R8) {
        throw std::runtime_error("'imul' does not support 8-bit registers");
    }
    else if constexpr (size == RegSize::R16) {
        is.emit(0x66);
        is.emit(0x69);
    }
    else if constexpr (size == RegSize::R32) {
        is.emit(0x69);
    }
    else if constexpr (size == RegSize::R64) {
        is.emit(0x48);
        is.emit(0x69);
    }

    is.emit(0xC0 + codeB * 8 + codeA);

    emitImmediate<size, RegSize::R32>(is, imm.toInt());
};

template<RegSize size>
void emitLeave(std::span<Token>, InstructionStream& is) {
    is.emit(0xC9);
}

template<RegSize size>
void emitRet(std::span<Token>, InstructionStream& is) {
    is.emit(0xC3);
}


using ArgsMatcher = std::function<bool(std::span<Token>)>;

template<Token::Kind... Kinds>
struct Args {
    template<std::size_t... Is>
    static constexpr bool match_imp(
            std::span<Token> tokens,
            std::integer_sequence<std::size_t, Is...>)
        {
        return tokens.size() == sizeof...(Kinds) && ((tokens[Is].kind == Kinds) && ...);
    }

    static bool match(std::span<Token> tokens) {
        return match_imp(tokens, std::make_integer_sequence<std::size_t, sizeof...(Kinds)>());
    }

    static ArgsMatcher matcher() {
        return [](std::span<Token> tokens) {
            return match(tokens);
        };
    }
};


constexpr inline Token::Kind Reg = Token::Register;
constexpr inline Token::Kind Imm = Token::Immediate;


struct Instruction {
    ArgsMatcher match;
    Emitter emit;
};


const std::unordered_multimap<std::string_view, Instruction> instructions{
    { "pushq", { Args<Reg>::matcher(), emitPush<RegSize::R64> } },  // XXX: maybe wrong?
    { "pushl", { Args<Reg>::matcher(), emitPush<RegSize::R32> } },
    { "pushw", { Args<Reg>::matcher(), emitPush<RegSize::R16> } },
    { "pushb", { Args<Reg>::matcher(), emitPush<RegSize::R8> } },
    { "movq",  { Args<Reg, Reg>::matcher(), emitMov<RegSize::R64> } },
    { "movl",  { Args<Reg, Reg>::matcher(), emitMov<RegSize::R32> } },
    { "movw",  { Args<Reg, Reg>::matcher(), emitMov<RegSize::R16> } },
    { "movb",  { Args<Reg, Reg>::matcher(), emitMov<RegSize::R8> } },
    { "addq",  { Args<Reg, Reg>::matcher(), emitAddRR<RegSize::R64> } },
    { "addl",  { Args<Reg, Reg>::matcher(), emitAddRR<RegSize::R32> } },
    { "addw",  { Args<Reg, Reg>::matcher(), emitAddRR<RegSize::R16> } },
    { "addb",  { Args<Reg, Reg>::matcher(), emitAddRR<RegSize::R8> } },
    { "addq",  { Args<Imm, Reg>::matcher(), emitAddIR<RegSize::R64> } },
    { "addl",  { Args<Imm, Reg>::matcher(), emitAddIR<RegSize::R32> } },
    { "addw",  { Args<Imm, Reg>::matcher(), emitAddIR<RegSize::R16> } },
    { "addb",  { Args<Imm, Reg>::matcher(), emitAddIR<RegSize::R8> } },
    { "imulq", { Args<Imm, Reg, Reg>::matcher(), emitImulIRR<RegSize::R64> } },
    { "imull", { Args<Imm, Reg, Reg>::matcher(), emitImulIRR<RegSize::R32> } },
    { "imulw", { Args<Imm, Reg, Reg>::matcher(), emitImulIRR<RegSize::R16> } },
    { "imulq", { Args<Reg, Reg>::matcher(), emitImulRR<RegSize::R64> } },
    { "imull", { Args<Reg, Reg>::matcher(), emitImulRR<RegSize::R32> } },
    { "imulw", { Args<Reg, Reg>::matcher(), emitImulRR<RegSize::R16> } },
    { "leave", { Args<>::matcher(), emitLeave<RegSize::R64> } },
    { "ret",   { Args<>::matcher(), emitRet<RegSize::R64> } }
};


class Assembler {
    std::function<void(int, int, std::string)> _report;

    std::string_view _code;
    std::unordered_map<std::string_view, size_t> _labels;

public:
    Assembler(std::string_view code, std::function<void(int, int, std::string)> report):
        _report(report), _code(code)
    {}

    std::vector<uint8_t> assemble() {
        std::vector<uint8_t> output;

        Scanner scanner(_code, [this](int line, int col, const std::string& msg) {
            _report(line, col, msg + " (scanner)");
        });
        TokenStream ts(scanner);
        InstructionStream is(output);

        while (true) {
            Token token = ts.next();
            if (!token) {
                break;
            }

            switch (token.kind) {
            case Token::Label:
                _labels[token.text] = output.size();
            break;
            case Token::Instruction: {
                auto [first, last] = instructions.equal_range(token.text);
                if (first == last) {
                    _report(token.line, token.column, "Unknown instruction " + std::string(token.text));
                    return {};
                }

                try {
                    std::vector<Token> args;
                    while (ts.peek().kind != Token::Newline) {
                        args.push_back(ts.next());
                        if (ts.peek().kind == Token::Comma) {
                            ts.next();
                        }
                        else if (ts.peek().kind != Token::Newline && ts.peek().kind != Token::NoToken) {
                            _report(token.line, token.column, "Expected comma, newline or eof");
                            return {};
                        }
                    }

                    bool found = false;
                    for (auto it = first; it != last; ++it) {
                        if (it->second.match(args)) {
                            it->second.emit(args, is);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        _report(token.line, token.column, "Invalid arguments for instruction " + std::string(token.text));
                        return {};
                    }
                }
                catch (const std::runtime_error& e) {
                    _report(token.line, token.column, e.what());
                    return {};
                }
            }
            break;
            case Token::Newline:
            case Token::Comment:
            case Token::Section:
            break;
            default:
                _report(token.line, token.column, "Unexpected token " + std::string(token.text));
                return {};
            }
        }

        return output;
    }
};


} // namespace jac::as::x86_64
