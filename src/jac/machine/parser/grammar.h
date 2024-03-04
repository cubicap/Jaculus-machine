#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <span>
#include <cassert>
#include <cmath>

#include "scanner.h"
#include "language.h"


namespace jac {


class ParserState {
    std::span<Token> _tokens;
    std::span<Token>::iterator _pos;

    Token _errorToken = Token(0, 0, "", Token::NoToken);
    std::string_view _errorMessage;

public:
    ParserState(std::span<Token> tokens):
        _tokens(tokens),
        _pos(_tokens.begin())
    {}

    void error(std::string_view message) {
        _errorToken = current();
        _errorMessage = message;
    }

    Token& current() {
        return *_pos;
    }

    void advance() {
        if (_pos == _tokens.end()) {
            error("Unexpected end of input");
            return;
        }
        ++_pos;
    }

    void backtrack() {
        assert(_pos != _tokens.begin());
        --_pos;
    }

    bool isEnd() {
        return _pos == _tokens.end();
    }

    auto getPosition() const {
        return _pos;
    }

    void restorePosition(auto position) {
        _pos = position;
    }

    std::string_view getErrorMessage() const {
        return _errorMessage;
    }

    Token getErrorToken() const {
        return _errorToken;
    }
};


struct IdentifierName {
    std::string name;

    static std::optional<IdentifierName> parse(ParserState& state) {
        if (state.isEnd()) {
            state.error("Unexpected end of input");
            return std::nullopt;
        }

        if (state.current().kind == Token::IdentifierName) {
            IdentifierName id;
            id.name = state.current().text;
            state.advance();
            return id;
        }

        return std::nullopt;
    }
};

struct Identifier {
    IdentifierName name;

    static std::optional<Identifier> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = IdentifierName::parse(state); id) {
            if (keywords.contains(id->name)) {
                state.restorePosition(start);
                state.error("Reserved word used as identifier");
                return std::nullopt;
            }
            return Identifier{*id};
        }

        return std::nullopt;
    }
};

template<bool Yield, bool Await>
struct IdentifierReference {
    Identifier identifier;

    static std::optional<IdentifierReference<Yield, Await>> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = Identifier::parse(state); id) {
            if (Yield && id->name.name == "yield") {
                state.restorePosition(start);
                state.error("Unexpected yield");
                return std::nullopt;
            }
            if (Await && id->name.name == "await") {
                state.restorePosition(start);
                state.error("Unexpected await");
                return std::nullopt;
            }
            return IdentifierReference<Yield, Await>{*id};
        }

        return std::nullopt;
    }
};

template<bool Yield, bool Await>
struct BindingIdentifier {
    Identifier identifier;

    static std::optional<BindingIdentifier<Yield, Await>> parse(ParserState& state) {
        if (auto id = Identifier::parse(state); id) {
            return BindingIdentifier<Yield, Await>{*id};
        }

        return std::nullopt;
    }
};

template<bool Yield, bool Await>
struct LabelIdentifier {
    Identifier identifier;

    static std::optional<LabelIdentifier<Yield, Await>> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = Identifier::parse(state); id) {
            if (Yield && id->name.name == "yield") {
                state.restorePosition(start);
                state.error("Unexpected yield");
                return std::nullopt;
            }
            if (Await && id->name.name == "await") {
                state.restorePosition(start);
                state.error("Unexpected await");
                return std::nullopt;
            }
            return LabelIdentifier<Yield, Await>{*id};
        }

        return std::nullopt;
    }
};

struct PrivateIdentifier {
    IdentifierName name;

    static std::optional<PrivateIdentifier> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = IdentifierName::parse(state); id) {
            if (id->name.size() < 2 || id->name[0] != '#') {
                state.restorePosition(start);
                state.error("Private identifier must start with #");
                return std::nullopt;
            }
            return PrivateIdentifier{*id};
        }

        return std::nullopt;
    }
};


struct ThisExpr {
    static std::optional<ThisExpr> parse(ParserState& state) {
        if (state.current().kind == Token::IdentifierName && state.current().text == "this") {
            state.advance();
            return ThisExpr{};
        }

        return std::nullopt;
    }
};

struct NullLiteral {
    static std::optional<NullLiteral> parse(ParserState& state) {
        if (state.current().kind == Token::IdentifierName && state.current().text == "null") {
            state.advance();
            return NullLiteral{};
        }

        return std::nullopt;
    }
};

struct BooleanLiteral {
    bool value;

    static std::optional<BooleanLiteral> parse(ParserState& state) {
        if (state.current().kind == Token::IdentifierName) {
            if (state.current().text == "true") {
                state.advance();
                return BooleanLiteral{true};
            }
            if (state.current().text == "false") {
                state.advance();
                return BooleanLiteral{false};
            }
        }

        return std::nullopt;
    }
};

struct NumericLiteral {
    std::variant<int32_t, double> value;  // TODO: bigint

    static std::optional<NumericLiteral> parse(ParserState& state) {
        if (state.current().kind != Token::NumericLiteral) {
            return std::nullopt;
        }
        std::string_view text = state.current().text;
        int32_t num = 0;
        double dnum = 0;

        bool legacyOctal = text[0] == '0';
        int base = 10;
        if (text.size() >= 2 && true && text[0] == '0') {
            char baseChar = std::tolower(text[1]);
            if (baseChar == 'b') {
                base = 2;
            } else if (baseChar == 'o') {
                base = 8;
            } else if (baseChar == 'x') {
                base = 16;
            }
            if (base != 10) {
                text = text.substr(2);
                legacyOctal = false;
                if (text.empty()) {
                    state.error("Invalid numeric literal");
                    return std::nullopt;
                }
            }
        }

        int exponent = 0;
        bool decimalPoint = false;
        bool isFloatingPoint = false;
        auto suffixStart = text.end();

        for (auto it = text.begin(); it != text.end(); ++it) {
            char c = *it;
            if (c == '_') {
                continue;
            }

            if (std::isdigit(c)) {
                if (decimalPoint) {
                    exponent -= 1;
                }
                legacyOctal &= c < '8';

                if (!isFloatingPoint) {
                    int64_t tmp = static_cast<int64_t>(num) * base + (c - '0');
                    if (static_cast<int32_t>(tmp) != tmp) {
                        isFloatingPoint = true;
                        dnum = tmp;
                    }
                    else {
                        num = tmp;
                    }
                }
                else {
                    dnum = dnum * base + (c - '0');
                }
            }
            else if (std::isxdigit(c) && base == 16) {
                if (decimalPoint) {
                    exponent -= 1;
                }
                if (!isFloatingPoint) {
                    int64_t tmp = static_cast<int64_t>(num) * base + (std::tolower(c) - 'a' + 10);
                    if (static_cast<int32_t>(tmp) != tmp) {
                        isFloatingPoint = true;
                        dnum = tmp;
                    }
                    else {
                        num = tmp;
                    }
                }
                else {
                    dnum = dnum * base + (std::tolower(c) - 'a' + 10);
                }
            }
            else if (c == '.') {
                decimalPoint = true;
                isFloatingPoint = true;
                dnum = num;
            }
            else if (suffixStart == text.end()) {
                suffixStart = it;
                break;
            }
        }

        if (legacyOctal && num != 0) {
            // TODO: fix base
            state.error("Legacy octal literals are not supported");
            return std::nullopt;
        }

        if (suffixStart != text.end() && std::tolower(*suffixStart) == 'e' && base == 10) {
            std::string_view exponentText = text.substr(suffixStart - text.begin() + 1);
            int exp = 0;
            bool negative = false;
            if (exponentText.empty()) {
                state.error("Invalid numeric literal");
                return std::nullopt;
            }
            if (exponentText[0] == '+' || exponentText[0] == '-') {
                negative = exponentText[0] == '-';
                exponentText = exponentText.substr(1);
            }
            if (exponentText.empty()) {
                state.error("Invalid numeric literal");
                return std::nullopt;
            }
            for (char c : exponentText) {
                if (c == '_') {
                    continue;
                }
                exp = exp * 10 + (c - '0');
            }

            if (negative) {
                exp = -exp;
            }


            exponent += exp;
        }
        else if (suffixStart != text.end()) {
            // TODO: different suffix - bigint
        }

        if (exponent != 0) {
            if (isFloatingPoint) {
                dnum *= std::pow(10, exponent);
            }
            else {
                // TODO: check overflow
                num *= std::pow(10, exponent);
            }
        }

        state.advance();
        if (isFloatingPoint) {
            return NumericLiteral{dnum};
        }
        return NumericLiteral{num};
    }
};

struct StringLiteral {
    std::string value;

    static std::optional<StringLiteral> parse(ParserState& state) {
        if (state.current().kind != Token::StringLiteral) {
            return std::nullopt;
        }
        std::string_view text = state.current().text;
        text = text.substr(1, text.size() - 2);  // remove quotes

        int length = 0;
        for (char c : text) {
            if (c != '\\') {
                length += 1;
            }
            // TODO: handle more complex escape sequences
        }

        std::string str;
        str.reserve(length);

        for (auto it = text.begin(); it != text.end(); ++it) {
            if (*it != '\\') {
                str.push_back(*it);
                continue;
            }
            else {
                ++it;
                if (it == text.end()) {
                    state.error("Invalid escape sequence");
                    return std::nullopt;
                }
                char c = *it;
                switch (c) {
                    case 'b':
                        str.push_back('\b');
                        break;
                    case 'f':
                        str.push_back('\f');
                        break;
                    case 'n':
                        str.push_back('\n');
                        break;
                    case 'r':
                        str.push_back('\r');
                        break;
                    case 't':
                        str.push_back('\t');
                        break;
                    case 'v':
                        str.push_back('\v');
                        break;
                    case '0':
                        str.push_back('\0');
                        break;
                    case '\'':
                    case '"':
                    case '\\':
                    default:
                        str.push_back(c);
                        break;
                }
            }
        }

        state.advance();
        return StringLiteral{str};
    }
};

struct Literal {
    std::variant<NullLiteral, BooleanLiteral, NumericLiteral, StringLiteral> value;

    static std::optional<Literal> parse(ParserState& state) {
        if (auto null = NullLiteral::parse(state); null) {
            return Literal{*null};
        }
        if (auto boolean = BooleanLiteral::parse(state); boolean) {
            return Literal{*boolean};
        }
        if (auto numeric = NumericLiteral::parse(state); numeric) {
            return Literal{*numeric};
        }
        if (auto string = StringLiteral::parse(state); string) {
            return Literal{*string};
        }

        return std::nullopt;
    }
};

template<bool In, bool Yield, bool Await>
struct AssignmentExpression;

template<bool In, bool Yield, bool Await>
using AssignmentExpressionPtr = std::unique_ptr<AssignmentExpression<In, Yield, Await>>;

template<bool In, bool Yield, bool Await>
using InitializerPtr = AssignmentExpressionPtr<In, Yield, Await>;

template<bool Yield, bool Await>
struct LeftHandSideExpression;
template<bool Yield, bool Await>
using LeftHandSideExpressionPtr = std::unique_ptr<LeftHandSideExpression<Yield, Await>>;

template<bool In, bool Yield, bool Await>
struct Assignment {
    LeftHandSideExpression<Yield, Await> lhs;
    AssignmentExpressionPtr<In, Yield, Await> rhs;
    std::string_view op;
};

template<bool Yield, bool Await>
struct UnaryExpression {
    using UnaryExpressionPtr = std::unique_ptr<UnaryExpression<Yield, Await>>;

    std::variant<
        UnaryExpressionPtr,
        LeftHandSideExpressionPtr<Yield, Await>
    > value;

    std::string_view op;
};

template<bool In, bool Yield, bool Await>
struct BinaryExpression {
    using BinaryExpressionPtr = std::unique_ptr<BinaryExpression<In, Yield, Await>>;

    std::variant<
        BinaryExpressionPtr,  // with higher precedence
        UnaryExpression<Yield, Await>
    > lhs;
    std::variant<
        BinaryExpressionPtr,  // with higher precedence
        UnaryExpression<Yield, Await>
    > rhs;
    std::string_view op;
};

template<bool In, bool Yield, bool Await>
struct ShortCircuitExpression {
    BinaryExpression<In, Yield, Await> expression;
};

template<bool In, bool Yield, bool Await>
struct ConditionalExpression {
    std::variant<
        ShortCircuitExpression<In, Yield, Await>,
        std::tuple<  // ternary conditional operator
            ShortCircuitExpression<In, Yield, Await>,
            AssignmentExpressionPtr<In, Yield, Await>,
            AssignmentExpressionPtr<In, Yield, Await>
        >
    > value;
};

struct Elision {};

template<bool Yield, bool Await>
struct ArrayLiteral {
    std::vector<std::variant<Elision, AssignmentExpressionPtr<true, Yield, Await>>> elementList;
    std::optional<AssignmentExpressionPtr<true, Yield, Await>> spreadElement;
};

template<bool Yield, bool Await>
struct ComputedPropertyName {
    AssignmentExpressionPtr<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct PropertyName {
    std::variant<
        IdentifierName,
        StringLiteral,
        NumericLiteral,
        ComputedPropertyName<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct ClassElementName {
    std::variant<
        PropertyName<Yield, Await>,
        PrivateIdentifier
    > value;
};

template<bool Yield, bool Await>
struct BindingProperty;

template<bool Yield, bool Await>
struct BindingElement;

template<bool Yield, bool Await>
struct ObjectBindingPattern {
    std::vector<BindingProperty<Yield, Await>> properties;
    std::optional<BindingIdentifier<Yield, Await>> rest;
};

template<bool Yield, bool Await>
struct ArrayBindingPattern {
    std::vector<std::variant<
        Elision,
        BindingElement<Yield, Await>
    >> elements;
    std::optional<BindingIdentifier<Yield, Await>> rest;
};

template<bool Yield, bool Await>
struct BindingPattern {
    std::variant<
        ObjectBindingPattern<Yield, Await>,
        ArrayBindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct BindingElement {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<true, Yield, Await>>,
        BindingPattern<Yield, Await>,
        std::pair<BindingPattern<Yield, Await>, InitializerPtr<true, Yield, Await>>
    > value;
};

template<bool Yield, bool Await>
struct BindingProperty {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<true, Yield, Await>>,
        std::pair<PropertyName<Yield, Await>, BindingElement<Yield, Await>>
    > value;
};

template<bool Yield, bool Await>
using FormalParameter = BindingElement<Yield, Await>;

template<bool Yield, bool Await>
struct BindingRestElement {
    std::variant<
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct FormalParameters {
    std::vector<FormalParameter<Yield, Await>> parameterList;
    std::optional<BindingRestElement<Yield, Await>> restParameter;
};

template<bool Yield, bool Await>
struct UniqueFormalParameters {
    FormalParameters<Yield, Await> parameters;
};

template<bool Yield, bool Await, bool Return>
struct Statement;

template<bool Yield, bool Await, bool Return>
using StatementPtr = std::unique_ptr<Statement<Yield, Await, Return>>;

template<bool Yield, bool Await, bool Return>
struct StatementListItem;

template<bool Yield, bool Await, bool Return>
struct StatementList {
    std::vector<StatementListItem<Yield, Await, Return>> items;

    static std::optional<StatementList<Yield, Await, Return>> parse(ParserState& state);

    ~StatementList();
    StatementList();
    StatementList(StatementList&&);
    StatementList(const StatementList&) = delete;
};

template<bool Yield, bool Await>
using FunctionBody = StatementList<Yield, Await, true>;
using GeneratorBody = FunctionBody<false, false>;
using AsyncFunctionBody = FunctionBody<false, false>;
using AsyncGeneratorBody = FunctionBody<false, false>;

template<bool In, bool Yield, bool Await>
struct Expression {
    std::vector<AssignmentExpressionPtr<In, Yield, Await>> items;
};

template<bool Yield, bool Await, bool Return>
struct Block {
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool In, bool Yield, bool Await>
struct LexicalBinding {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<In, Yield, Await>>,
        std::pair<BindingPattern<Yield, Await>, InitializerPtr<In, Yield, Await>>
    > value;

    static std::optional<LexicalBinding<In, Yield, Await>> parse(ParserState& state) {
        if (auto id = BindingIdentifier<Yield, Await>::parse(state); id) {
            if (state.current().kind == Token::Punctuator && state.current().text == "=") {
                state.advance();
                if (auto initializer = AssignmentExpression<In, Yield, Await>::parse(state); initializer) {
                    auto ptr = std::make_unique<AssignmentExpression<In, Yield, Await>>(std::move(*initializer));
                    return LexicalBinding<In, Yield, Await>{std::pair{std::move(*id), std::move(ptr)}};
                }
                state.backtrack();
                return std::nullopt;
            }
            return LexicalBinding<In, Yield, Await>{std::move(*id)};
        }

        // TODO: rest
        return std::nullopt;
    }
};

template<bool In, bool Yield, bool Await>
struct LexicalDeclaration {
    bool isConst;
    std::vector<LexicalBinding<In, Yield, Await>> bindings;

    static std::optional<LexicalDeclaration<In, Yield, Await>> parse(ParserState& state) {
        auto start = state.getPosition();
        if (state.current().kind != Token::Keyword) {
            state.error("Expected let or const");
            return std::nullopt;
        }
        bool isConst;
        if (state.current().text == "let") {
            isConst = false;
        }
        else if (state.current().text == "const") {
            isConst = true;
        }
        else {
            state.error("Expected let or const");
            return std::nullopt;
        }
        state.advance();

        LexicalDeclaration<In, Yield, Await> declaration{ isConst, {} };

        while (true) {
            if (auto binding = LexicalBinding<In, Yield, Await>::parse(state); binding) {
                declaration.bindings.push_back(std::move(*binding));
            }
            else {
                state.restorePosition(start);
                return std::nullopt;
            }


            if (state.current().kind == Token::Punctuator && state.current().text == ",") {
                state.advance();
                continue;
            }
            if (state.current().kind == Token::Punctuator && state.current().text == ";") {
                state.advance();
                break;
            }
            state.error("Unexpected token");
            state.restorePosition(start);
            return std::nullopt;
        }

        return std::move(declaration);
    }
};

template<bool Yield, bool Await, bool Return>
using BlockStatement = Block<Yield, Await, Return>;

template<bool In, bool Yield, bool Await>
struct VariableDeclaration {
    BindingIdentifier<Yield, Await> identifier;
    std::optional<InitializerPtr<In, Yield, Await>> initializer;
};

template<bool In, bool Yield, bool Await>
struct VariableDeclarationList {
    std::vector<std::variant<VariableDeclaration<In, Yield, Await>>> declarations;

};

template<bool Yield, bool Await>
struct VariableStatement {
    VariableDeclarationList<true, Yield, Await> declarationList;
};

struct EmptyStatement {};

template<bool Yield, bool Await>
struct ExpressionStatement {
    Expression<false, Yield, Await> expression;
};

template<bool Yield, bool Await, bool Return>
struct IfStatement {
    Expression<true, Yield, Await> expression;
    StatementPtr<Yield, Await, Return> consequent;
    std::optional<StatementPtr<Yield, Await, Return>> alternate;
};

template<bool Yield, bool Await, bool Return>
struct DoWhileStatement {
    StatementPtr<Yield, Await, Return> statement;
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await, bool Return>
struct WhileStatement {
    Expression<true, Yield, Await> expression;
    StatementPtr<Yield, Await, Return> statement;
};

template<bool Yield, bool Await, bool Return>
struct ForStatement {
    std::variant<
        std::monostate,
        std::pair<LexicalDeclaration<false, Yield, Await>, Expression<true, Yield, Await>>,
        Expression<false, Yield, Await>
    > init;
    std::optional<Expression<true, Yield, Await>> condition;
    std::optional<Expression<true, Yield, Await>> update;
    StatementPtr<Yield, Await, Return> statement;
};

template<bool Yield, bool Await, bool Return>
struct ForInOfStatement {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Return>
struct IterationStatement {
    std::variant<
        DoWhileStatement<Yield, Await, Return>,
        WhileStatement<Yield, Await, Return>,
        ForStatement<Yield, Await, Return>,
        ForInOfStatement<Yield, Await, Return>
    > value;
};

template<bool Yield, bool Await, bool Return>
struct CaseClause {
    Expression<true, Yield, Await> expression;
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool Yield, bool Await, bool Return>
struct DefaultClause {
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool Yield, bool Await, bool Return>
struct SwitchStatement {
    Expression<true, Yield, Await> expression;
    std::vector<CaseClause<Yield, Await, Return>> caseBlock;
    std::optional<DefaultClause<Yield, Await, Return>> defaultClause;
};

template<bool Yield, bool Await, bool Return>
struct BreakableStatement {
    std::variant<
        IterationStatement<Yield, Await, Return>,
        SwitchStatement<Yield, Await, Return>
    > value;
};

template<bool Yield, bool Await>
struct ContinueStatement {
    std::optional<LabelIdentifier<Yield, Await>> label;
};

template<bool Yield, bool Await>
struct BreakStatement {
    std::optional<LabelIdentifier<Yield, Await>> label;
};

template<bool Yield, bool Await>
struct ReturnStatement {
    std::optional<Expression<true, Yield, Await>> expression;
};

template<bool Yield, bool Await, bool Return>
struct WithStatement {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Default>
struct FunctionDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    FunctionBody<false, false> body;
};

template<bool Yield, bool Await, bool Return>
struct LabeledStatement {
    LabelIdentifier<Yield, Await> label;
    std::variant<
        StatementPtr<Yield, Await, Return>,
        FunctionDeclaration<Yield, Await, false>
    > statement;
};

template<bool Yield, bool Await>
struct ThrowStatement {
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct CatchParameter {
    std::variant<
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await, bool Return>
struct Catch {
    CatchParameter<Yield, Await> parameter;
    Block<Yield, Await, Return> block;
};

template<bool Yield, bool Await, bool Return>
struct TryStatement {
    Block<Yield, Await, Return> block;
    std::optional<Catch<Yield, Await, Return>> catchClause;
    std::optional<Block<Yield, Await, Return>> finallyClause;
};

struct DebuggerStatement {};

template<bool Yield, bool Await, bool Return>
struct Statement {
    std::variant<
        BlockStatement<Yield, Await, Return>,
        VariableStatement<Yield, Await>,
        EmptyStatement,
        ExpressionStatement<Yield, Await>,
        IfStatement<Yield, Await, Return>,
        BreakableStatement<Yield, Await, Return>,
        ContinueStatement<Yield, Await>,
        BreakStatement<Yield, Await>,
        ReturnStatement<Yield, Await>, // [+Return]
        WithStatement<Yield, Await, Return>,
        LabeledStatement<Yield, Await, Return>,
        ThrowStatement<Yield, Await>,
        TryStatement<Yield, Await, Return>,
        DebuggerStatement
    > value;

    static std::optional<Statement<Yield, Await, Return>> parse(ParserState& state) {
        // TODO
        return std::nullopt;
    }
};

template<bool Yield, bool Await, bool Default>
struct GeneratorDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    GeneratorBody body;
};

template<bool Yield, bool Await, bool Default>
struct AsyncFunctionDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    AsyncFunctionBody body;
};

template<bool Yield, bool Await, bool Default>
struct AsyncGeneratorDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    AsyncGeneratorBody body;
};

template<bool Yield, bool Await, bool Default>
struct HoistableDeclaration {
    std::variant<
        FunctionDeclaration<Yield, Await, Default>,
        GeneratorDeclaration<Yield, Await, Default>,
        AsyncFunctionDeclaration<Yield, Await, Default>,
        AsyncGeneratorDeclaration<Yield, Await, Default>
    > value;
};

template<bool Yield, bool Await>
struct ClassHeritage {
    LeftHandSideExpressionPtr<Yield, Await> value;
};

struct ClassStaticBlock {
    std::optional<StatementList<false, false, false>> statementList;
};

template<bool Yield, bool Await>
struct MethodDefinition {
    struct GetFunctionBody : public FunctionBody<false, false> {};
    struct SetFunctionBody : public FunctionBody<false, false> {};

    std::variant<
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, FunctionBody<false, false>>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, GeneratorBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, AsyncFunctionBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, AsyncGeneratorBody>,
        std::tuple<ClassElementName<Yield, Await>, GetFunctionBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, SetFunctionBody>
    > value;
};

template<bool Yield, bool Await>
struct FieldDefinition {
    ClassElementName<Yield, Await> name;
    std::optional<InitializerPtr<true, Yield, Await>> initializer;
};

template<bool Yield, bool Await>
struct ClassElement {
    std::variant<
        std::monostate,  // semicolon
        std::pair<bool, MethodDefinition<Yield, Await>>,  // bool: static
        std::pair<bool, FieldDefinition<Yield, Await>>,  // bool: static
        ClassStaticBlock
    > value;
};

template<bool Yield, bool Await>
struct ClassBody {
    std::vector<ClassElement<Yield, Await>> elements;
};

template<bool Yield, bool Await, bool Default>
struct ClassDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    std::optional<ClassHeritage<Yield, Await>> heritage;
    ClassBody<Yield, Await> body;
};

template<bool Yield, bool Await, bool Return>
struct Declaration {
    std::variant<
        HoistableDeclaration<Yield, Await, false>,
        ClassDeclaration<Yield, Await, false>,
        LexicalDeclaration<true, Yield, Await>
    > value;

    static std::optional<Declaration<Yield, Await, Return>> parse(ParserState& state) {
        if (auto lexical = LexicalDeclaration<true, Yield, Await>::parse(state); lexical) {
            return Declaration<Yield, Await, Return>{std::move(*lexical)};
        }
        return std::nullopt;
    }
};

template<bool Yield, bool Await, bool Return>
struct StatementListItem {
    std::variant<
        Statement<Yield, Await, Return>,
        Declaration<Yield, Await, Return>
    > value;

    StatementListItem(Statement<Yield, Await, Return> statement): value(std::move(statement)) {}
    StatementListItem(Declaration<Yield, Await, Return> declaration): value(std::move(declaration)) {}

    static std::optional<StatementListItem<Yield, Await, Return>> parse(ParserState& state) {
        if (auto statement = Statement<Yield, Await, Return>::parse(state); statement) {
            return StatementListItem<Yield, Await, Return>{std::move(*statement)};
        }
        if (auto declaration = Declaration<Yield, Await, Return>::parse(state); declaration) {
            return StatementListItem<Yield, Await, Return>{std::move(*declaration)};
        }

        return std::nullopt;
    }
};

template<bool Yield, bool Await>
struct CoverInitializedName {
    IdentifierReference<Yield, Await> identifier;
    InitializerPtr<true, Yield, Await> initializer;
};

template<bool Yield, bool Await>
struct PropertyDefinition {
    using SpreadAssignmentExpressionPtr = AssignmentExpressionPtr<true, Yield, Await>;

    std::variant<
        IdentifierReference<Yield, Await>,
        CoverInitializedName<Yield, Await>,
        std::pair<PropertyName<Yield, Await>, AssignmentExpressionPtr<true, Yield, Await>>,
        MethodDefinition<Yield, Await>,
        SpreadAssignmentExpressionPtr
    > value;
};

template<bool Yield, bool Await>
struct ObjectLiteral {
    std::vector<PropertyDefinition<Yield, Await>> properties;
};

// XXX: why are the following in the grammar twice?
struct FunctionExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    FunctionBody<false, false> body;
};

template<bool Yield, bool Await>
struct ClassExpression {
    std::optional<BindingIdentifier<Yield, Await>> name;
    std::optional<ClassHeritage<Yield, Await>> heritage;
    ClassBody<Yield, Await> body;
};

struct GeneratorExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    GeneratorBody body;
};

struct AsyncFunctionExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    AsyncFunctionBody body;
};

struct AsyncGeneratorExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    AsyncGeneratorBody body;
};

struct RegularExpressionLiteral {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Tagged>
struct TemplateLiteral {
    // XXX: ignore for now
};

template<bool Yield, bool Await>
struct CoverParenthesizedExpressionAndArrowParameterList {
    std::optional<Expression<true, Yield, Await>> expression;
    std::variant<
        std::monostate,  // no parameters
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > parameters;
};

template<bool Yield, bool Await>
struct PrimaryExpression {
    std::variant<
        ThisExpr,
        IdentifierReference<Yield, Await>,
        Literal,
        ArrayLiteral<Yield, Await>,
        ObjectLiteral<Yield, Await>,
        FunctionExpression,
        ClassExpression<Yield, Await>,
        GeneratorExpression,
        AsyncFunctionExpression,
        AsyncGeneratorExpression,
        RegularExpressionLiteral,
        TemplateLiteral<Yield, Await, false>,
        CoverParenthesizedExpressionAndArrowParameterList<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct SuperProperty {
    std::variant<
        IdentifierName,  // dot
        Expression<true, Yield, Await>  // bracket
    > value;
};

struct MetaProperty {
    enum Kind {
        NewTarget,
        ImportMeta
    };

    Kind kind;
};

template<bool Yield, bool Await>
struct Arguments {
    std::vector<std::pair<bool, AssignmentExpressionPtr<true, Yield, Await>>> arguments;  // bool: spread
};

template<bool Yield, bool Await>
struct MemberExpression {
    using MemberExpressionPtr = std::unique_ptr<MemberExpression<Yield, Await>>;

    std::variant<
        PrimaryExpression<Yield, Await>,
        std::pair<MemberExpressionPtr, std::variant<
            Expression<true, Yield, Await>,  // bracket
            IdentifierName, // dot
            PrivateIdentifier,  // dot
            TemplateLiteral<Yield, Await, true>,  // tag
            Arguments<Yield, Await>  // new
        >>,
        SuperProperty<Yield, Await>,
        MetaProperty
    > value;
};

template<bool Yield, bool Await>
struct NewExpression {
    using NewExpressionPtr = std::unique_ptr<NewExpression<Yield, Await>>;

    std::variant<
        MemberExpression<Yield, Await>,
        NewExpressionPtr
    > value;
};

template<bool Yield, bool Await>
struct SuperCall {
    Arguments<Yield, Await> arguments;
};

template<bool Yield, bool Await>
struct ImportCall {
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct CoverCallExpressionAndAsyncArrowHead {
    std::variant<
        MemberExpression<Yield, Await>,
        Arguments<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct CallExpression {
    using CallExpressionPtr = std::unique_ptr<CallExpression<Yield, Await>>;

    std::variant<
        CoverCallExpressionAndAsyncArrowHead<Yield, Await>,
        SuperCall<Yield, Await>,
        ImportCall<Yield, Await>,
        std::pair<CallExpressionPtr, Arguments<Yield, Await>>,
        std::pair<CallExpressionPtr, Expression<true, Yield, Await>>,  // bracket
        std::pair<CallExpressionPtr, IdentifierName>,  // dot
        std::pair<CallExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<CallExpressionPtr, TemplateLiteral<Yield, Await, true>>  // tag
    > value;
};

template<bool Yield, bool Await>
struct OptionalChain {
    using OptionalChainPtr = std::unique_ptr<OptionalChain<Yield, Await>>;

    std::optional<OptionalChainPtr> chain;
    std::variant<
        Arguments<Yield, Await>,  // ?.
        Expression<true, Yield, Await>,  // ?.[
        IdentifierName,  // ?.identifier
        PrivateIdentifier,  // ?.#identifier
        TemplateLiteral<Yield, Await, true>  // ?.tag
    > value;
};

template<bool Yield, bool Await>
struct OptionalExpression {
    using OptionalExpressionPtr = std::unique_ptr<OptionalExpression<Yield, Await>>;

    std::variant<
        MemberExpression<Yield, Await>,
        CallExpression<Yield, Await>,
        OptionalExpressionPtr
    > value;
    OptionalChain<Yield, Await> chain;
};

template<bool Yield, bool Await>
struct LeftHandSideExpression {
    std::variant<
        NewExpression<Yield, Await>,
        CallExpression<Yield, Await>,
        OptionalExpression<Yield, Await>
    > value;
};

template<bool In, bool Yield, bool Await>
struct YieldExpression {
    bool star;
    std::optional<AssignmentExpressionPtr<In, Yield, Await>> expression;
};

template<bool Yield, bool Await>
struct ArrowParameters {
    std::variant<
        BindingIdentifier<Yield, Await>,
        CoverParenthesizedExpressionAndArrowParameterList<Yield, Await>
    > value;
};

template<bool In>
struct ConciseBody {
    std::variant<
        AssignmentExpressionPtr<In, false, false>,  // lookahead not {
        FunctionBody<false, false>
    > value;
};

template<bool In, bool Yield, bool Await>
struct ArrowFunction {
    ArrowParameters<Yield, Await> parameters;
    ConciseBody<In> body;
};

template<bool In>
struct AsyncConciseBody {
    std::variant<
        AssignmentExpressionPtr<In, false, true>,  // lookahead not {
        AsyncFunctionBody
    > value;
};

template<bool In, bool Yield, bool Await>
struct AsyncArrowFunction {
    std::variant<
        BindingIdentifier<Yield, true>,
        CoverCallExpressionAndAsyncArrowHead<Yield, Await>
    > parameters;
    AsyncConciseBody<In> body;
};

template<bool In, bool Yield, bool Await>
struct AssignmentExpression {
    std::variant<
        ConditionalExpression<In, Yield, Await>,
        YieldExpression<In, Yield, Await>,  // only when [+Yield]
        ArrowFunction<In, Yield, Await>,
        AsyncArrowFunction<In, Yield, Await>,
        Assignment<In, Yield, Await>
    > value;

    static std::optional<AssignmentExpression<In, Yield, Await>> parse(ParserState& state) {
        state.error("Not implemented");
        return std::nullopt;
    }
};


struct Script {
    std::optional<StatementList<false, false, false>> statementList;

    static std::optional<Script> parse(ParserState& state) {
        if (state.isEnd()) {
            // return Script{};
        }

        if (auto statementList = StatementList<false, false, false>::parse(state); statementList) {
            return Script{std::move(*statementList)};
        }

        return std::nullopt;
    }
};

struct ImportDeclaration {
    // XXX: ignore for now
};

struct ExportDeclaration {
    // XXX: ignore for now
};

struct ModuleItem {
    std::variant<
        ImportDeclaration,
        ExportDeclaration,
        StatementListItem<false, true, false>
    > value;
};

struct ModuleItemList {
    std::vector<ModuleItem> items;
};

struct Module {
    std::optional<ModuleItemList> moduleItemList;
};

template<bool Yield, bool Await, bool Return>
std::optional<StatementList<Yield, Await, Return>> StatementList<Yield, Await, Return>::parse(ParserState& state) {
    StatementList<Yield, Await, Return> list;
    while (true) {
        if (auto item = StatementListItem<Yield, Await, Return>::parse(state); item) {
            list.items.push_back(std::move(*item));
        }
        else {
            break;
        }
    }
    if (list.items.empty()) {
        return std::nullopt;
    }
    return std::move(list);
}

template<bool Yield, bool Await, bool Return>
StatementList<Yield, Await, Return>::~StatementList() = default;

template<bool Yield, bool Await, bool Return>
StatementList<Yield, Await, Return>::StatementList() = default;

template<bool Yield, bool Await, bool Return>
StatementList<Yield, Await, Return>::StatementList(StatementList&& other) = default;


}  // namespace jac
