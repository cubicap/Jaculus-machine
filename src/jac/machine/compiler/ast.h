#pragma once

#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

#include "language.h"
#include "scanner.h"

#include "../../util.h"


namespace jac::ast {


struct Yield {
    const bool value = true;
};
struct Await {
    const bool value = true;
};
struct In {
    const bool value = true;
};
struct Return {
    const bool value = true;
};


class ParserState {
    std::span<lex::Token> _tokens;
    std::span<lex::Token>::iterator _pos;

    lex::Token _errorToken = lex::Token(0, 0, "", lex::Token::NoToken);
    std::string_view _errorMessage;

    std::vector<bool> yieldStack;
    std::vector<bool> awaitStack;
    std::vector<bool> inStack;
    std::vector<bool> returnStack;

    template<typename Arg>
    auto& getStack() {
        if constexpr (std::is_same_v<Arg, Yield>) {
            return yieldStack;
        }
        else if constexpr (std::is_same_v<Arg, Await>) {
            return awaitStack;
        }
        else if constexpr (std::is_same_v<Arg, In>) {
            return inStack;
        }
        else if constexpr (std::is_same_v<Arg, Return>) {
            return returnStack;
        }
    }
public:
    ParserState(std::span<lex::Token> tokens):
        _tokens(tokens),
        _pos(_tokens.begin())
    {}

    void error(std::string_view message) {
        if (_tokens.empty()) {
            _errorMessage = message;
            return;
        }
        if (_pos == _tokens.end()) { // TODO: fix token position
            _errorToken = _tokens.back();
            _errorMessage = message;
            return;
        }
        if (_errorToken.text.begin() > _pos->text.begin()) {
            return;
        }
        _errorToken = current();
        _errorMessage = message;
    }

    lex::Token current() {
        if (_tokens.empty()) {
            return lex::Token(0, 0, "", lex::Token::NoToken);
        }
        if (_pos == _tokens.end()) {
            auto pos = _tokens.back().text.end();
            return lex::Token(_tokens.back().line + 1, 0, std::string_view(pos, pos), lex::Token::NoToken);
        }
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

    lex::Token getErrorToken() const {
        return _errorToken;
    }

    bool getYield() const {
        if (yieldStack.empty()) {
            return false;
        }
        return yieldStack.back();
    }
    bool getAwait() const {
        if (awaitStack.empty()) {
            return false;
        }
        return awaitStack.back();
    }
    bool getIn() const {
        if (inStack.empty()) {
            return false;
        }
        return inStack.back();
    }
    bool getReturn() const {
        if (returnStack.empty()) {
            return false;
        }
        return returnStack.back();
    }


    decltype(auto) pushCallPop(auto&& func) {
        return func();
    }

    template<auto Arg, auto... Args>
    decltype(auto) pushCallPop(auto&& func) {
        using Arg_t = std::decay_t<decltype(Arg)>;

        getStack<Arg_t>().push_back(Arg.value);
        Defer d([this] {
            getStack<Arg_t>().pop_back();
        });

        return pushCallPop<Args...>(func);
    }

};


struct IdentifierName {
    std::string name;

    static std::optional<IdentifierName> parse(ParserState& state) {
        if (state.isEnd()) {
            state.error("Unexpected end of input");
            return std::nullopt;
        }

        if (state.current().kind == lex::Token::IdentifierName) {
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
        if (auto id = IdentifierName::parse(state)) {
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

struct IdentifierReference {
    Identifier identifier;

    static std::optional<IdentifierReference> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = Identifier::parse(state)) {
            if (state.getYield() && id->name.name == "yield") {
                state.restorePosition(start);
                state.error("Unexpected yield");
                return std::nullopt;
            }
            if (state.getAwait() && id->name.name == "await") {
                state.restorePosition(start);
                state.error("Unexpected await");
                return std::nullopt;
            }
            return IdentifierReference{*id};
        }

        return std::nullopt;
    }
};

struct BindingIdentifier {
    Identifier identifier;

    static std::optional<BindingIdentifier> parse(ParserState& state) {
        if (auto id = Identifier::parse(state)) {
            return BindingIdentifier{*id};
        }

        return std::nullopt;
    }
};

struct LabelIdentifier {
    Identifier identifier;

    static std::optional<LabelIdentifier> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = Identifier::parse(state)) {
            if (state.getYield() && id->name.name == "yield") {
                state.restorePosition(start);
                state.error("Unexpected yield");
                return std::nullopt;
            }
            if (state.getAwait() && id->name.name == "await") {
                state.restorePosition(start);
                state.error("Unexpected await");
                return std::nullopt;
            }
            return LabelIdentifier{*id};
        }

        return std::nullopt;
    }
};

struct PrivateIdentifier {
    IdentifierName name;

    static std::optional<PrivateIdentifier> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto id = IdentifierName::parse(state)) {
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
        if (state.current().kind == lex::Token::Keyword && state.current().text == "this") {
            state.advance();
            return ThisExpr{};
        }

        return std::nullopt;
    }
};

struct NullLiteral {
    static std::optional<NullLiteral> parse(ParserState& state) {
        if (state.current().kind == lex::Token::Keyword && state.current().text == "null") {
            state.advance();
            return NullLiteral{};
        }

        return std::nullopt;
    }
};

struct BooleanLiteral {
    bool value;

    static std::optional<BooleanLiteral> parse(ParserState& state) {
        if (state.current().kind == lex::Token::Keyword) {
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
    std::variant<std::int32_t, double> value;  // TODO: bigint

    static std::optional<NumericLiteral> parse(ParserState& state) {
        if (state.current().kind != lex::Token::NumericLiteral) {
            return std::nullopt;
        }
        std::string_view text = state.current().text;
        std::int32_t num = 0;
        double dnum = 0;

        bool legacyOctal = text[0] == '0';
        int base = 10;
        if (text.size() >= 2 && text[0] == '0') {
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
                    if (static_cast<std::int32_t>(tmp) != tmp) {
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
                    if (static_cast<std::int32_t>(tmp) != tmp) {
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
        if (state.current().kind != lex::Token::StringLiteral) {
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
        if (auto null = NullLiteral::parse(state)) {
            return Literal{*null};
        }
        if (auto boolean = BooleanLiteral::parse(state)) {
            return Literal{*boolean};
        }
        if (auto numeric = NumericLiteral::parse(state)) {
            return Literal{*numeric};
        }
        if (auto string = StringLiteral::parse(state)) {
            return Literal{*string};
        }

        return std::nullopt;
    }
};

struct AssignmentExpression;

using AssignmentExpressionPtr = std::unique_ptr<AssignmentExpression>;

using InitializerPtr = AssignmentExpressionPtr;

struct LeftHandSideExpression;
using LeftHandSideExpressionPtr = std::unique_ptr<LeftHandSideExpression>;

struct UnaryExpression;

using UnaryExpressionPtr = std::unique_ptr<UnaryExpression>;


enum class UpdateKind {
    None,
    PreInc,
    PreDec,
    PostInc,
    PostDec
};

struct UpdateExpression {
    std::variant<
        UnaryExpressionPtr,
        LeftHandSideExpressionPtr
    > value;

    UpdateKind kind;

    static std::optional<UpdateExpression> parse(ParserState& state);
};

struct UnaryExpression {
    std::variant<
        std::pair<UnaryExpressionPtr, std::string_view>,
        UpdateExpression
    > value;

    static std::optional<UnaryExpression> parse(ParserState& state) {
        if (state.current().kind == lex::Token::Punctuator || state.current().kind == lex::Token::Keyword) { // parse prefix unary
            std::string_view op = state.current().text;
            if (unaryOperator.contains(op) && (state.getAwait() || op != "await")) {
                state.advance();
                if (auto expr = UnaryExpression::parse(state)) {
                    auto ptr = std::make_unique<UnaryExpression>(std::move(*expr));
                    return UnaryExpression{std::pair{std::move(ptr), op}};
                }
                state.backtrack();
            }
        }

        if (auto update = UpdateExpression::parse(state)) {
            return UnaryExpression{std::move(*update)};
        }

        return std::nullopt;
    }
};

struct BinaryExpression {
    using BinaryExpressionPtr = std::unique_ptr<BinaryExpression>;

    std::variant<
        std::tuple<BinaryExpressionPtr, BinaryExpressionPtr, std::string_view>,
        UnaryExpressionPtr
    > value;

    static std::optional<BinaryExpression> parse(ParserState& state) {
        // Shunting Yard algorithm
        using Element = std::variant<
            UnaryExpressionPtr,
            std::string_view
        >;

        std::vector<Element> output;
        std::vector<std::string_view> operators;

        auto start = state.getPosition();
        while (true) {
            auto unary = UnaryExpression::parse(state);
            if (!unary) {
                return std::nullopt;
            }
            output.push_back(std::make_unique<UnaryExpression>(std::move(*unary)));
            auto next = state.current();
            if (next.kind != lex::Token::Punctuator) {
                break;
            }
            auto precedence = binaryPrecedence.find(next.text);
            if (precedence == binaryPrecedence.end()) {
                break;
            }
            while (!operators.empty()) {
                auto top = operators.back();
                auto topPrecedence = binaryPrecedence.at(top);
                if (topPrecedence < precedence->second
                  || (rightAssociative.contains(top) && precedence->second == topPrecedence)) {
                    break;
                }
                operators.pop_back();
                output.push_back(top);
            }
            operators.push_back(next.text);
            state.advance();
        }
        while (!operators.empty()) {
            output.push_back(operators.back());
            operators.pop_back();
        }

        struct Popper {
            std::vector<Element>& output;
            ParserState& state;
            std::span<lex::Token>::iterator start;

            std::optional<BinaryExpression> pop() {
                if (output.empty()) {
                    state.restorePosition(start);
                    return std::nullopt;
                }
                auto elem = std::move(output.back());
                output.pop_back();
                if (auto unary = std::get_if<UnaryExpressionPtr>(&elem)) {
                    return BinaryExpression{std::move(*unary)};
                }
                if (auto op = std::get_if<std::string_view>(&elem)) {
                    auto rhs = pop();
                    if (!rhs) {
                        state.restorePosition(start);
                        return std::nullopt;
                    }
                    auto lhs = pop();
                    if (!lhs) {
                        state.restorePosition(start);
                        return std::nullopt;
                    }
                    return BinaryExpression{std::make_tuple(
                        std::make_unique<BinaryExpression>(std::move(*lhs)),
                        std::make_unique<BinaryExpression>(std::move(*rhs)),
                        *op
                    )};
                }

                return std::nullopt;
            }
        };

        Popper popper{output, state, start};
        auto res = popper.pop();
        if (!res) {
            state.restorePosition(start);
            return std::nullopt;
        }
        return BinaryExpression{std::move(*res)};
    }
};

using BinaryExpressionPtr = std::unique_ptr<BinaryExpression>;

struct ConditionalExpression {
    std::variant<
        BinaryExpression,
        std::tuple<  // ternary conditional operator
            BinaryExpression,
            AssignmentExpressionPtr,
            AssignmentExpressionPtr
        >
    > value;

    static std::optional<ConditionalExpression> parse(ParserState& state);
};

struct Elision {};

struct ArrayLiteral {
    // AssignmentExpression[+In, Yield, Await]
    std::vector<std::variant<Elision, AssignmentExpressionPtr>> elementList;
    std::optional<AssignmentExpressionPtr> spreadElement;
};

struct ComputedPropertyName {
    // AssignmentExpression[+In, Yield, Await]
    AssignmentExpressionPtr expression;
};

struct PropertyName {
    std::variant<
        IdentifierName,
        StringLiteral,
        NumericLiteral,
        ComputedPropertyName
    > value;
};

struct ClassElementName {
    std::variant<
        PropertyName,
        PrivateIdentifier
    > value;
};

struct BindingProperty;

struct BindingElement;

struct ObjectBindingPattern {
    std::vector<BindingProperty> properties;
    std::optional<BindingIdentifier> rest;
};

struct ArrayBindingPattern {
    std::vector<std::variant<
        Elision,
        BindingElement
    >> elements;
    std::optional<BindingIdentifier> rest;
};

struct BindingPattern {
    std::variant<
        ObjectBindingPattern,
        ArrayBindingPattern
    > value;

    static std::optional<BindingPattern> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }

    BindingPattern(BindingPattern&&);
    ~BindingPattern();
};


// XXX: EXTENSION, incompatible with standard
struct TypeAnnotation {
    Identifier type;

    static std::optional<TypeAnnotation> parse(ParserState& state) {
        if (state.current().kind == lex::Token::Punctuator && state.current().text == ":") {
            state.advance();
            if (auto id = Identifier::parse(state)) {
                return TypeAnnotation{*id};
            }
        }

        return std::nullopt;
    }
};

struct BindingElement {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,  // Initializer[+In, Yield, Await]
        BindingPattern,
        std::pair<BindingPattern, InitializerPtr>  // Initializer[+In, Yield, Await]
    > value;
    std::optional<TypeAnnotation> type;

    static std::optional<BindingElement> parse(ParserState& state);
};

struct BindingProperty {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,  // Initializer[+In, Yield, Await]
        std::pair<PropertyName, BindingElement>
    > value;
};

using FormalParameter = BindingElement;

struct BindingRestElement {
    std::variant<
        BindingIdentifier,
        BindingPattern
    > value;

    static std::optional<BindingRestElement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct FormalParameters {
    std::vector<FormalParameter> parameterList;
    std::optional<BindingRestElement> restParameter;

    static std::optional<FormalParameters> parse(ParserState& state) {
        FormalParameters params;

        bool canContinue = true;
        while (true) {
            if (auto param = FormalParameter::parse(state)) {
                params.parameterList.push_back(std::move(*param));
                if (state.current().kind == lex::Token::Punctuator && state.current().text == ",") {
                    state.advance();
                    continue;
                }
            }

            canContinue = false;
            break;
        }

        if (canContinue) {
            if (auto rest = BindingRestElement::parse(state)) {
                params.restParameter.emplace(BindingRestElement{std::move(*rest)});
            }
        }

        return params;
    }
};

struct UniqueFormalParameters {
    FormalParameters parameters;
};

struct Statement;

using StatementPtr = std::unique_ptr<Statement>;

struct StatementListItem;

struct StatementList {
    std::vector<StatementListItem> items;

    static std::optional<StatementList> parse(ParserState& state);

    ~StatementList();
    StatementList();
    StatementList(StatementList&&);
};

struct FunctionBody {
    StatementList statementList;  // StatementList[Yield, Await, +Return]

    static std::optional<FunctionBody> parse(ParserState& state) {
        if (auto stmts = state.pushCallPop<Return(true)>([&] { return StatementList::parse(state); }) ) {
            return FunctionBody{std::move(*stmts)};
        }
        return std::nullopt;
    }
};

using GeneratorBody = FunctionBody;         // FunctionBody[+Yield, -Await]
using AsyncFunctionBody = FunctionBody;     // FunctionBody[-Yield, +Await]
using AsyncGeneratorBody = FunctionBody;     // FunctionBody[+Yield, +Await]

struct Expression {
    std::vector<AssignmentExpressionPtr> items;

    static std::optional<Expression> parse(ParserState& state);

    static std::optional<Expression> parseParenthesised(ParserState& state) {
        auto start = state.getPosition();
        if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
            state.error("Expected (");
            return std::nullopt;
        }
        state.advance();

        auto expr = parse(state);
        if (!expr) {
            state.error("Expected expression");
            state.restorePosition(start);
            return std::nullopt;
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != ")") {
            state.error("Expected )");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();

        return expr;
    }
};

struct Block {
    std::optional<StatementList> statementList;

    static std::optional<Block> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Punctuator || state.current().text != "{") {
            state.error("Expected {");
            return std::nullopt;
        }
        state.advance();

        Block block;
        auto start = state.getPosition();
        if (auto list = StatementList::parse(state)) {
            block.statementList.emplace(std::move(*list));
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != "}") {
            state.restorePosition(start);
            state.error("Expected }");
            return std::nullopt;
        }
        state.advance();

        return block;
    }
};

struct LexicalBinding {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,
        std::pair<BindingPattern, InitializerPtr>
    > value;
    std::optional<TypeAnnotation> type;  // TODO: maybe move to BindingIdentifier?

    static std::optional<LexicalBinding> parse(ParserState& state);
};

struct LexicalDeclaration {
    bool isConst;
    std::vector<LexicalBinding> bindings;

    static std::optional<LexicalDeclaration> parse(ParserState& state) {
        auto start = state.getPosition();
        if (state.current().kind != lex::Token::Keyword) {
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

        LexicalDeclaration declaration{ isConst, {} };

        while (true) {
            if (auto binding = LexicalBinding::parse(state)) {
                declaration.bindings.push_back(std::move(*binding));
            }
            else {
                state.restorePosition(start);
                state.error("Invalid lexical declaration");
                return std::nullopt;
            }


            if (state.current().kind == lex::Token::Punctuator && state.current().text == ",") {
                state.advance();
                continue;
            }
            if (state.current().kind == lex::Token::Punctuator && state.current().text == ";") {
                state.advance();
                break;
            }
            state.error("Unexpected token");
            state.restorePosition(start);
            return std::nullopt;
        }

        return declaration;
    }
};

using BlockStatement = Block;

struct VariableDeclaration {
    BindingIdentifier identifier;
    std::optional<InitializerPtr> initializer;
};

struct VariableDeclarationList {
    std::vector<std::variant<VariableDeclaration>> declarations;

};

struct VariableStatement {
    VariableDeclarationList declarationList;  // VariableDeclarationList[+In, Yield, Await]

    static std::optional<VariableStatement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct EmptyStatement {
    static std::optional<EmptyStatement> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            return std::nullopt;
        }
        state.advance();
        return EmptyStatement{};
    }
};

struct ExpressionStatement {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ExpressionStatement> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto expr = state.pushCallPop<In(false)>([&](){ return Expression::parse(state); })) {
            if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
                state.error("Expected ;");
                state.restorePosition(start);
                return std::nullopt;
            }
            state.advance();
            return ExpressionStatement{std::move(*expr)};
        }

        return std::nullopt;
    }
};

struct IfStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    StatementPtr consequent;
    std::optional<StatementPtr> alternate;

    static std::optional<IfStatement> parse(ParserState& state);
};

struct DoWhileStatement {
    StatementPtr statement;
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<DoWhileStatement> parse(ParserState& state);
};

struct WhileStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    StatementPtr statement;

    static std::optional<WhileStatement> parse(ParserState& state);
};

struct ForStatement {
    std::variant<
        std::monostate,
        Expression,               // Expression[-In, Yield, Await]
        VariableDeclarationList,  // VariableDeclarationList[-In, Yield, Await]
        LexicalDeclaration        // LexicalDeclaration[-In, Yield, Await]
    > init;
    std::optional<Expression> condition;  // Expression[+In, Yield, Await]
    std::optional<Expression> update;     // Expression[+In, Yield, Await]
    StatementPtr statement;

    static std::optional<ForStatement> parse(ParserState& state);
};

struct ForInOfStatement {
    // XXX: ignore for now

    static std::optional<ForInOfStatement> parse(ParserState&) {
        return std::nullopt;
    }
};

struct IterationStatement {
    std::variant<
        DoWhileStatement,
        WhileStatement,
        ForStatement,
        ForInOfStatement
    > value;

    static std::optional<IterationStatement> parse(ParserState& state) {
        if (auto doWhile = DoWhileStatement::parse(state)) {
            return IterationStatement{std::move(*doWhile)};
        }
        if (auto whileStmt = WhileStatement::parse(state)) {
            return IterationStatement{std::move(*whileStmt)};
        }
        if (auto forStmt = ForStatement::parse(state)) {
            return IterationStatement{std::move(*forStmt)};
        }
        if (auto forInOf = ForInOfStatement::parse(state)) {
            return IterationStatement{std::move(*forInOf)};
        }

        return std::nullopt;
    }
};

struct CaseClause {
    Expression expression;  // Expression[+In, Yield, Await]
    std::optional<StatementList> statementList;
};

struct DefaultClause {
    std::optional<StatementList> statementList;
};

struct SwitchStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    std::vector<CaseClause> caseBlock;
    std::optional<DefaultClause> defaultClause;

    static std::optional<SwitchStatement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct BreakableStatement {
    std::variant<
        IterationStatement,
        SwitchStatement
    > value;

    static std::optional<BreakableStatement> parse(ParserState& state) {
        if (auto iter = IterationStatement::parse(state)) {
            return BreakableStatement{std::move(*iter)};
        }
        if (auto sw = SwitchStatement::parse(state)) {
            return BreakableStatement{std::move(*sw)};
        }

        return std::nullopt;
    }
};

struct ContinueStatement {
    std::optional<LabelIdentifier> label;

    static std::optional<ContinueStatement> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Keyword || state.current().text != "continue") {
            return std::nullopt;
        }
        state.advance();

        ContinueStatement statement;

        if (auto ident = LabelIdentifier::parse(state)) {
            statement.label = std::move(*ident);
        }
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            return std::nullopt;
        }
        state.advance();
        return statement;
    }
};

struct BreakStatement {
    std::optional<LabelIdentifier> label;

    static std::optional<BreakStatement> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Keyword || state.current().text != "break") {
            return std::nullopt;
        }
        state.advance();

        BreakStatement statement;

        if (auto ident = LabelIdentifier::parse(state)) {
            statement.label = std::move(*ident);
        }
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            return std::nullopt;
        }
        state.advance();
        return statement;
    }
};

struct ReturnStatement {
    std::optional<Expression> expression;  // Expression[+In, Yield, Await]

    static std::optional<ReturnStatement> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Keyword || state.current().text != "return") {
            return std::nullopt;
        }
        state.advance();

        ReturnStatement statement;

        auto start = state.getPosition();
        if (auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
            statement.expression = std::move(*expr);
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            state.restorePosition(start);
            return std::nullopt;
        }

        state.advance();
        return statement;
    }
};

struct WithStatement {
    // XXX: ignore for now

    static std::optional<WithStatement> parse(ParserState&) {
        return std::nullopt;
    }
};

struct FunctionDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;       // FormalParameters[-Yield, -Await]
    std::optional<FunctionBody> body;  // FunctionBody[-Yield, -Await]
    std::optional<TypeAnnotation> returnType;

    std::string_view code;

    static std::optional<FunctionDeclaration> parse(ParserState& state, bool Default) {
        auto start = state.getPosition();
        if (state.current().kind != lex::Token::Keyword || state.current().text != "function") {
            return std::nullopt;
        }
        state.advance();

        std::optional<BindingIdentifier> name;
        if (!Default) {
            if (auto id = BindingIdentifier::parse(state)) {
                name = std::move(*id);
            }
            else {
                state.error("Expected identifier");
                state.restorePosition(start);
                return std::nullopt;
            }
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
            state.error("Expected (");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();

        auto params = state.pushCallPop<Yield(false), Await(false)>([&](){ return FormalParameters::parse(state); });
        if (!params) {
            state.restorePosition(start);
            return std::nullopt;
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != ")") {
            state.error("Expected )");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();

        auto returnType = TypeAnnotation::parse(state);

        if (state.current().kind != lex::Token::Punctuator || state.current().text != "{") {
            state.error("Expected {");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();

        auto body = state.pushCallPop<Yield(false), Await(false)>([&](){ return FunctionBody::parse(state); });

        if (state.current().kind != lex::Token::Punctuator || state.current().text != "}") {
            state.error("Expected }");
            state.restorePosition(start);
            return std::nullopt;
        }

        std::string_view code(start->text.begin(), state.current().text.end());

        state.advance();

        return FunctionDeclaration{
            std::move(name),
            std::move(*params),
            std::move(body),
            std::move(returnType),
            code
        };
    }
};

struct LabeledStatement {
    LabelIdentifier label;
    std::variant<
        StatementPtr,
        FunctionDeclaration  // FunctionDeclaration[Yield, Await, -Default]
    > statement;

    static std::optional<LabeledStatement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct ThrowStatement {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ThrowStatement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct CatchParameter {
    std::variant<
        BindingIdentifier,
        BindingPattern
    > value;
};

struct Catch {
    CatchParameter parameter;
    Block block;
};

struct TryStatement {
    Block block;
    std::optional<Catch> catchClause;
    std::optional<Block> finallyClause;

    static std::optional<TryStatement> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct DebuggerStatement {
    static std::optional<DebuggerStatement> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Keyword || state.current().text != "debugger") {
            return std::nullopt;
        }
        state.advance();
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            return std::nullopt;
        }
        state.advance();
        return DebuggerStatement{};
    }
};

struct Statement {
    std::variant<
        BlockStatement,
        VariableStatement,
        EmptyStatement,
        ExpressionStatement,
        IfStatement,
        BreakableStatement,
        ContinueStatement,
        BreakStatement,
        ReturnStatement, // [+Return]
        WithStatement,
        LabeledStatement,
        ThrowStatement,
        TryStatement,
        DebuggerStatement
    > value;

    static std::optional<Statement> parse(ParserState& state) {
        if (auto block = BlockStatement::parse(state)) {
            return Statement{std::move(*block)};
        }
        if (auto variable = VariableStatement::parse(state)) {
            return Statement{std::move(*variable)};
        }
        if (auto empty = EmptyStatement::parse(state)) {
            return Statement{std::move(*empty)};
        }
        if (auto expression = ExpressionStatement::parse(state)) {
            return Statement{std::move(*expression)};
        }
        if (auto if_ = IfStatement::parse(state)) {
            return Statement{std::move(*if_)};
        }
        if (auto breakable = BreakableStatement::parse(state)) {
            return Statement{std::move(*breakable)};
        }
        if (auto continue_ = ContinueStatement::parse(state)) {
            return Statement{std::move(*continue_)};
        }
        if (auto break_ = BreakStatement::parse(state)) {
            return Statement{std::move(*break_)};
        }
        if (state.getReturn()) {
            if (auto ret = ReturnStatement::parse(state)) {
                return Statement{std::move(*ret)};
            }
        }
        if (auto with = WithStatement::parse(state)) {
            return Statement{std::move(*with)};
        }
        if (auto labeled = LabeledStatement::parse(state)) {
            return Statement{std::move(*labeled)};
        }
        if (auto throw_ = ThrowStatement::parse(state)) {
            return Statement{std::move(*throw_)};
        }
        if (auto try_ = TryStatement::parse(state)) {
            return Statement{std::move(*try_)};
        }
        if (auto debugger = DebuggerStatement::parse(state)) {
            return Statement{std::move(*debugger)};
        }

        return std::nullopt;
    }
};

struct GeneratorDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    GeneratorBody body;

    static std::optional<GeneratorDeclaration> parse(ParserState&, bool) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct AsyncFunctionDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    AsyncFunctionBody body;

    static std::optional<AsyncFunctionDeclaration> parse(ParserState&, bool) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct AsyncGeneratorDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    AsyncGeneratorBody body;

    static std::optional<AsyncGeneratorDeclaration> parse(ParserState&, bool) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct HoistableDeclaration {
    std::variant<
        FunctionDeclaration,
        GeneratorDeclaration,
        AsyncFunctionDeclaration,
        AsyncGeneratorDeclaration
    > value;

    static std::optional<HoistableDeclaration> parse(ParserState& state, bool Default) {
        if (auto function = FunctionDeclaration::parse(state, Default)) {
            return HoistableDeclaration{std::move(*function)};
        }
        if (auto generator = GeneratorDeclaration::parse(state, Default)) {
            return HoistableDeclaration{std::move(*generator)};
        }
        if (auto asyncFunction = AsyncFunctionDeclaration::parse(state, Default)) {
            return HoistableDeclaration{std::move(*asyncFunction)};
        }
        if (auto asyncGenerator = AsyncGeneratorDeclaration::parse(state, Default)) {
            return HoistableDeclaration{std::move(*asyncGenerator)};
        }
        return std::nullopt;
    }
};

struct ClassHeritage {
    LeftHandSideExpressionPtr value;
};

struct ClassStaticBlock {
    std::optional<StatementList> statementList;  // StatementList[-Yield, -Await, -Return]
};

struct GetFunctionBody : public FunctionBody {};  // FunctionBody[-Yield, -Await]
struct SetFunctionBody : public FunctionBody {};  // FunctionBody[-Yield, -Await]

struct MethodDefinition {

    std::variant<  // UniqueFormalParameters[-Yield, -Await]
        std::tuple<ClassElementName, UniqueFormalParameters, FunctionBody>,  // FunctionBody[-Yield, -Await]
        std::tuple<ClassElementName, UniqueFormalParameters, GeneratorBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, AsyncFunctionBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, AsyncGeneratorBody>,
        std::tuple<ClassElementName, GetFunctionBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, SetFunctionBody>
    > value;
};

struct FieldDefinition {
    ClassElementName name;
    std::optional<InitializerPtr> initializer;  // Initializer[+In, Yield, Await]
};

struct ClassElement {
    std::variant<
        std::monostate,  // semicolon
        std::pair<bool, MethodDefinition>,  // bool: static
        std::pair<bool, FieldDefinition>,  // bool: static
        ClassStaticBlock
    > value;
};

struct ClassBody {
    std::vector<ClassElement> elements;
};

struct ClassDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    std::optional<ClassHeritage> heritage;
    ClassBody body;

    static std::optional<ClassDeclaration> parse(ParserState&, bool) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct Declaration {
    std::variant<
        HoistableDeclaration,  // HoistableDeclaration[Yield, Await, -Default]
        ClassDeclaration,      // ClassDeclaration[Yield, Await, -Default]
        LexicalDeclaration     // LexicalDeclaration[+In, Yield, Await]
    > value;

    static std::optional<Declaration> parse(ParserState& state) {
        if (auto hoistable = HoistableDeclaration::parse(state, false)) {
            return Declaration{std::move(*hoistable)};
        }
        if (auto klass = ClassDeclaration::parse(state, false)) {
            return Declaration{std::move(*klass)};
        }
        if (auto lexical = state.pushCallPop<In(true)>([&](){ return LexicalDeclaration::parse(state); })) {
            return Declaration{std::move(*lexical)};
        }
        return std::nullopt;
    }
};

struct StatementListItem {
    std::variant<
        Statement,
        Declaration
    > value;

    StatementListItem(Statement statement): value(std::move(statement)) {}
    StatementListItem(Declaration declaration): value(std::move(declaration)) {}

    static std::optional<StatementListItem> parse(ParserState& state) {
        if (auto statement = Statement::parse(state)) {
            return StatementListItem{std::move(*statement)};
        }
        if (auto declaration = Declaration::parse(state)) {
            return StatementListItem{std::move(*declaration)};
        }

        return std::nullopt;
    }
};

struct CoverInitializedName {
    IdentifierReference identifier;
    InitializerPtr initializer;  // Initializer[+In, Yield, Await]
};

struct PropertyDefinition {
    using SpreadAssignmentExpressionPtr = AssignmentExpressionPtr;  // AssignmentExpression[+In, Yield, Await]

    std::variant<
        IdentifierReference,
        CoverInitializedName,
        std::pair<PropertyName, AssignmentExpressionPtr>,  // AssignmentExpression[+In, Yield, Await]
        MethodDefinition,
        SpreadAssignmentExpressionPtr
    > value;
};

struct ObjectLiteral {
    std::vector<PropertyDefinition> properties;
};

// XXX: why are the following in the grammar twice?
struct FunctionExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    FunctionBody body;                      // FunctionBody[-Yield, -Await]
};

struct ClassExpression {
    std::optional<BindingIdentifier> name;
    std::optional<ClassHeritage> heritage;
    ClassBody body;
};

struct GeneratorExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    GeneratorBody body;
};

struct AsyncFunctionExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    AsyncFunctionBody body;
};

struct AsyncGeneratorExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    AsyncGeneratorBody body;
};

struct RegularExpressionLiteral {
    // XXX: ignore for now

    static std::optional<RegularExpressionLiteral> parse(ParserState&) {
        return std::nullopt;
    }
};

struct TemplateLiteral {
    // XXX: ignore for now

    static std::optional<TemplateLiteral> parse(ParserState&, bool) {
        return std::nullopt;
    }
};

struct ParenthesizedExpression {
    Expression expression;  // Expression[+In, Yield, Await]
};

struct CoverParenthesizedExpressionAndArrowParameterList {
    std::optional<Expression> expression;  // Expression[+In, Yield, Await]
    std::variant<
        std::monostate,  // no parameters
        BindingIdentifier,
        BindingPattern
    > parameters;

    static std::optional<CoverParenthesizedExpressionAndArrowParameterList> parse(ParserState& state) {
        auto start = state.getPosition();
        if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
            return std::nullopt;
        }
        state.advance();

        CoverParenthesizedExpressionAndArrowParameterList result;
        bool canContinue = true;

        if (auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
            result.expression = std::move(*expr);
            if (state.current().kind != lex::Token::Punctuator || state.current().text != ",") {
                canContinue = false;
            }
            else {
                state.advance();
            }
        }

        if (canContinue) {
            if (state.current().kind == lex::Token::Punctuator && state.current().text == "...") {
                state.advance();
                if (auto binding = BindingIdentifier::parse(state)) {
                    canContinue = false;
                    result.parameters.template emplace<BindingIdentifier>(std::move(*binding));
                }
                else if (auto pattern = BindingPattern::parse(state)) {
                    canContinue = false;
                    result.parameters.template emplace<BindingPattern>(std::move(*pattern));
                }
                else {
                    state.error("Invalid binding");
                    state.restorePosition(start);
                    return std::nullopt;
                }
            }
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != ")") {
            state.error("Expected )");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();

        return result;
    }

    std::optional<ParenthesizedExpression> refineParExp() {
        // XXX: investigate more
        if (!std::holds_alternative<std::monostate>(parameters)) {
            return std::nullopt;
        }
        if (!expression) {
            return std::nullopt;
        }
        return ParenthesizedExpression{std::move(*expression)};
    }
};

struct PrimaryExpression {
    std::variant<
        ThisExpr,
        IdentifierReference,
        Literal,
        ArrayLiteral,
        ObjectLiteral,
        FunctionExpression,
        ClassExpression,
        GeneratorExpression,
        AsyncFunctionExpression,
        AsyncGeneratorExpression,
        RegularExpressionLiteral,
        TemplateLiteral,  // TemplateLiteral[In, Yield, -Tagged]
        ParenthesizedExpression  // TODO: check if ok
    > value;

    static std::optional<PrimaryExpression> parse(ParserState& state) {
        if (auto thisExpr = ThisExpr::parse(state)) {
            return PrimaryExpression{std::move(*thisExpr)};
        }
        if (auto identifier = IdentifierReference::parse(state)) {
            return PrimaryExpression{std::move(*identifier)};
        }
        if (auto lit = Literal::parse(state)) {
            return PrimaryExpression{std::move(*lit)};
        }

        if (auto cover = CoverParenthesizedExpressionAndArrowParameterList::parse(state)) {
            if (auto refined = cover->refineParExp()) {
                return PrimaryExpression{std::move(*refined)};
            }
            state.error("Invalid parenthesized expression");
            return std::nullopt;
        }

        // TODO: rest of the cases
        return std::nullopt;
    }
};

struct SuperProperty {
    std::variant<
        IdentifierName,  // dot
        Expression  // bracket; Expression[+In, Yield, Await]
    > value;

    static std::optional<SuperProperty> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct MetaProperty {
    enum Kind {
        NewTarget,
        ImportMeta
    };

    Kind kind;

    static std::optional<MetaProperty> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct Arguments {
    std::vector<std::pair<bool, AssignmentExpressionPtr>> arguments;  // bool: spread; AssignmentExpression[+In, Yield, Await]

    static std::optional<Arguments> parse(ParserState& state);
};

struct MemberExpression {
    using MemberExpressionPtr = std::unique_ptr<MemberExpression>;

    std::variant<
        SuperProperty,
        MetaProperty,
        PrimaryExpression,
        std::pair<MemberExpressionPtr, Expression>,  // bracket; Expression[+In, Yield, Await]
        std::pair<MemberExpressionPtr, IdentifierName>, // dot
        std::pair<MemberExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<MemberExpressionPtr, TemplateLiteral>,  // tag; TemplateLiteral[In, Yield, +Tagged]
        std::pair<MemberExpressionPtr, Arguments>  // new
    > value;

    static std::optional<MemberExpression> parse(ParserState& state) {
        if (state.current().kind == lex::Token::Keyword && state.current().text == "new") {
            auto start = state.getPosition();
            state.advance();
            if (auto member = MemberExpression::parse(state)) {
                if (auto args = Arguments::parse(state)) {
                    auto ptr = std::make_unique<MemberExpression>(std::move(*member));
                    return MemberExpression{std::pair{std::move(ptr), std::move(*args)}};
                }
            }
            state.restorePosition(start);
            return std::nullopt;
        }

        std::optional<MemberExpression> member;
        if (auto super = SuperProperty::parse(state)) {
            member.emplace(MemberExpression{ std::move(*super) });
        }
        else if (auto meta = MetaProperty::parse(state)) {
            member.emplace(MemberExpression{ std::move(*meta) });
        }
        else if (auto primary = PrimaryExpression::parse(state)) {
            member.emplace(MemberExpression{ std::move(*primary) });
        }

        if (!member) {
            return std::nullopt;
        }

        do {
            auto start = state.getPosition();

            if (state.current().kind == lex::Token::Punctuator && state.current().text == "[") {
                state.advance();
                if (auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
                    if (state.current().kind == lex::Token::Punctuator && state.current().text == "]") {
                        state.advance();
                        auto ptr = std::make_unique<MemberExpression>(std::move(*member));
                        member.emplace(MemberExpression{std::pair{std::move(ptr), std::move(*expr)}});
                        continue;
                    }
                    state.error("Expected ]");
                }
                state.restorePosition(start);
            }
            if (state.current().kind == lex::Token::Punctuator && state.current().text == ".") {
                state.advance();
                if (auto identifier = IdentifierName::parse(state)) {
                    auto ptr = std::make_unique<MemberExpression>(std::move(*member));
                    member.emplace(MemberExpression{std::pair{std::move(ptr), std::move(*identifier)}});
                    continue;
                }
                if (auto priv = PrivateIdentifier::parse(state)) {
                    auto ptr = std::make_unique<MemberExpression>(std::move(*member));
                    member.emplace(MemberExpression{std::pair{std::move(ptr), std::move(*priv)}});
                    continue;
                }
                state.backtrack();
            }
            if (auto tplate = TemplateLiteral::parse(state, true)) {
                auto ptr = std::make_unique<MemberExpression>(std::move(*member));
                member.emplace(MemberExpression{std::pair{std::move(ptr), std::move(*tplate)}});
                continue;
            }

            state.restorePosition(start);
            break;
        } while (true);

        return member;
    }
};
using MemberExpressionPtr = std::unique_ptr<MemberExpression>;


struct NewExpression;

using NewExpressionPtr = std::unique_ptr<NewExpression>;

struct NewExpression {

    std::variant<
        MemberExpression,
        NewExpressionPtr
    > value;

    static std::optional<NewExpression> parse(ParserState& state) {
        if (state.current().kind != lex::Token::Keyword && state.current().text == "new") {
            state.advance();
            if (auto expr = NewExpression::parse(state)) {
                auto ptr = std::make_unique<NewExpression>(std::move(*expr));
                return NewExpression{std::move(ptr)};
            }
            state.backtrack();
        }
        if (auto member = MemberExpression::parse(state)) {
            return NewExpression{std::move(*member)};
        }

        return std::nullopt;
    }
};
using NewExpressionPtr = std::unique_ptr<NewExpression>;

struct SuperCall {
    Arguments arguments;

    static std::optional<SuperCall> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct ImportCall {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ImportCall> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct CoverCallExpressionAndAsyncArrowHead {
    MemberExpression memberExpression;
    Arguments arguments;

    static std::optional<CoverCallExpressionAndAsyncArrowHead> parse(ParserState& state) {
        auto start = state.getPosition();
        if (auto member = MemberExpression::parse(state)) {
            if (auto args = Arguments::parse(state)) {
                return CoverCallExpressionAndAsyncArrowHead{std::move(*member), std::move(*args)};
            }
        }

        state.restorePosition(start);
        return std::nullopt;
    }
};

struct CallExpression {
    using CallExpressionPtr = std::unique_ptr<CallExpression>;

    // grammar: CoverCallExpressionAndAsyncArrowHead
    std::variant<
        SuperCall,
        ImportCall,
        std::pair<MemberExpression, Arguments>,  // cover grammar
        std::pair<CallExpressionPtr, Arguments>,
        std::pair<CallExpressionPtr, Expression>,  // bracket; Expression[+In, Yield, Await]
        std::pair<CallExpressionPtr, IdentifierName>,  // dot
        std::pair<CallExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<CallExpressionPtr, TemplateLiteral>  // tag; TemplateLiteral[In, Yield, +Tagged]
    > value;

    static std::optional<CallExpression> parse(ParserState& state) {
        std::optional<CallExpression> call;

        if (auto super = SuperCall::parse(state)) {
            call.emplace(CallExpression{std::move(*super)});
        }
        else if (auto import = ImportCall::parse(state)) {
            call.emplace(CallExpression{std::move(*import)});
        }
        else if (auto cover = CoverCallExpressionAndAsyncArrowHead::parse(state)) {
            call.emplace(CallExpression{std::pair{std::move(cover->memberExpression), std::move(cover->arguments)}});
        }

        if (!call) {
            return std::nullopt;
        }

        do {
            auto start = state.getPosition();

            if (auto args = Arguments::parse(state)) {
                auto ptr = std::make_unique<CallExpression>(std::move(*call));
                call.emplace(CallExpression{std::pair{std::move(ptr), std::move(*args)}});
                continue;
            }
            if (state.current().kind == lex::Token::Punctuator && state.current().text == "[") {
                state.advance();
                if (auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
                    if (state.current().kind == lex::Token::Punctuator && state.current().text == "]") {
                        state.advance();
                        auto ptr = std::make_unique<CallExpression>(std::move(*call));
                        call.emplace(CallExpression{std::pair{std::move(ptr), std::move(*expr)}});
                        continue;
                    }
                    state.error("Expected ]");
                }
                state.restorePosition(start);
            }
            if (state.current().kind == lex::Token::Punctuator && state.current().text == ".") {
                state.advance();
                if (auto identifier = IdentifierName::parse(state)) {
                    auto ptr = std::make_unique<CallExpression>(std::move(*call));
                    call.emplace(CallExpression{std::pair{std::move(ptr), std::move(*identifier)}});
                    continue;
                }
                if (auto identifier = PrivateIdentifier::parse(state)) {
                    auto ptr = std::make_unique<CallExpression>(std::move(*call));
                    call.emplace(CallExpression{std::pair{std::move(ptr), std::move(*identifier)}});
                    continue;
                }
                state.backtrack();
            }
            if (auto tplate = TemplateLiteral::parse(state, true)) {
                auto ptr = std::make_unique<CallExpression>(std::move(*call));
                call.emplace(CallExpression{std::pair{std::move(ptr), std::move(*tplate)}});
                continue;
            }

            state.restorePosition(start);
            break;
        } while (true);

        return call;
    }
};

using CallExpressionPtr = std::unique_ptr<CallExpression>;

struct OptionalChain {
    using OptionalChainPtr = std::unique_ptr<OptionalChain>;

    std::optional<OptionalChainPtr> chain;
    std::variant<
        Arguments,  // ?.
        Expression,  // ?.[   Expression[+In, Yield, Await]
        IdentifierName,  // ?.identifier
        PrivateIdentifier,  // ?.#identifier
        TemplateLiteral  // ?.tag   // TemplateLiteral[In, Yield, +Tagged]
    > value;
};

struct OptionalExpression {
    using OptionalExpressionPtr = std::unique_ptr<OptionalExpression>;

    std::variant<
        MemberExpression,
        CallExpression,
        OptionalExpressionPtr
    > value;
    OptionalChain chain;
};

struct LeftHandSideExpression {
    std::variant<
        CallExpression,
        NewExpression
        // OptionalExpression
    > value;

    static std::optional<LeftHandSideExpression> parse(ParserState& state) {
        if (auto call = CallExpression::parse(state)) {
            return LeftHandSideExpression{std::move(*call)};
        }
        if (auto newExpr = NewExpression::parse(state)) {
            return LeftHandSideExpression{std::move(*newExpr)};
        }
        // TODO: rest

        return std::nullopt;
    }
};

struct YieldExpression {
    bool star;
    std::optional<AssignmentExpressionPtr> expression;

    static std::optional<YieldExpression> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct ArrowParameters {
    std::variant<
        BindingIdentifier,
        CoverParenthesizedExpressionAndArrowParameterList
    > value;
};

struct ConciseBody {
    std::variant<
        AssignmentExpressionPtr,  // lookahead not {, AssignmentExpression[In, -Yield, -Await]
        FunctionBody  // FunctionBody[-Yield, -Await]
    > value;
};

struct ArrowFunction {
    ArrowParameters parameters;
    ConciseBody body;

    static std::optional<ArrowFunction> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct AsyncConciseBody {
    std::variant<
        AssignmentExpressionPtr,  // lookahead not {, AssignmentExpression[In, -Yield, -Await]
        AsyncFunctionBody
    > value;
};

struct AsyncArrowFunction {
    std::variant<
        BindingIdentifier,  // BindingIdentifier[Yield, +Await]
        CoverCallExpressionAndAsyncArrowHead
    > parameters;
    AsyncConciseBody body;

    static std::optional<AsyncArrowFunction> parse(ParserState&) {
        // XXX: ignore for now
        return std::nullopt;
    }
};

struct Assignment {
    LeftHandSideExpression lhs;
    AssignmentExpressionPtr rhs;
    std::string_view op;

    static std::optional<Assignment> parse(ParserState& state);
};

struct AssignmentExpression {
    std::variant<
        ConditionalExpression,
        YieldExpression,  // only when [+Yield]
        ArrowFunction,
        AsyncArrowFunction,
        Assignment
    > value;

    static std::optional<AssignmentExpression> parse(ParserState& state) {
        if (auto yield = YieldExpression::parse(state)) {
            return AssignmentExpression{std::move(*yield)};
        }
        if (auto arrow = ArrowFunction::parse(state)) {
            return AssignmentExpression{std::move(*arrow)};
        }
        if (auto asyncArrow = AsyncArrowFunction::parse(state)) {
            return AssignmentExpression{std::move(*asyncArrow)};
        }
        if (auto assignment = Assignment::parse(state)) {
            return AssignmentExpression{std::move(*assignment)};
        }
        if (auto cond = ConditionalExpression::parse(state)) {
            return AssignmentExpression{std::move(*cond)};
        }
        return std::nullopt;
    }
};


struct Script {
    std::optional<StatementList> statementList;  // StatementList[-Yield, -Await, -Return]

    static std::optional<Script> parse(ParserState& state) {
        if (state.isEnd()) {
            return Script{};
        }

        return state.pushCallPop<Yield(false), Await(false), Return(false)>([&]() -> std::optional<Script> {
            if (auto statementList = StatementList::parse(state)) {
                return Script{std::move(*statementList)};
            }

            return std::nullopt;
        });
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
        StatementListItem  // StatementListItem[-Yield, +Await, -Return]
    > value;
};

struct ModuleItemList {
    std::vector<ModuleItem> items;
};

struct Module {
    std::optional<ModuleItemList> moduleItemList;
};

inline std::optional<StatementList> StatementList::parse(ParserState& state) {
    StatementList list;
    while (true) {
        if (auto item = StatementListItem::parse(state)) {
            list.items.push_back(std::move(*item));
        }
        else {
            break;
        }
    }
    if (list.items.empty()) {
        return std::nullopt;
    }
    return list;
}


inline std::optional<UpdateExpression> UpdateExpression::parse(ParserState& state) {
    // TODO: handle situations like multiple ++ or -- operators
    // TODO: check that "AssignmentTargetType" is simple
    if (state.current().kind == lex::Token::Punctuator) {
        std::string_view op = state.current().text;
        if (op == "++" || op == "--") {
            state.advance();
            if (auto expr = UnaryExpression::parse(state)) {
                auto ptr = std::make_unique<UnaryExpression>(std::move(*expr));
                return UpdateExpression{std::move(ptr), (op == "++" ? UpdateKind::PreInc : UpdateKind::PreDec)};
            }
            state.backtrack();
        }
    }


    if (auto expr = LeftHandSideExpression::parse(state)) {
        auto ptr = std::make_unique<LeftHandSideExpression>(std::move(*expr));
        if (state.current().kind == lex::Token::Punctuator) {
            std::string_view op = state.current().text;
            if (op == "++" || op == "--") {
                state.advance();
                return UpdateExpression{std::move(ptr), (op == "++" ? UpdateKind::PostInc : UpdateKind::PostDec)};
            }
        }
        return UpdateExpression{std::move(ptr), UpdateKind::None};
    }

    return std::nullopt;
}


inline std::optional<ConditionalExpression> ConditionalExpression::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto shortCircuit = BinaryExpression::parse(state)) {
        if (state.current().kind == lex::Token::Punctuator && state.current().text == "?") {
            state.advance();
            if (auto consequent = AssignmentExpression::parse(state)) {
                if (state.current().kind == lex::Token::Punctuator && state.current().text == ":") {
                    state.advance();
                    if (auto alternate = AssignmentExpression::parse(state)) {
                        auto consPtr = std::make_unique<AssignmentExpression>(std::move(*consequent));
                        auto altPtr = std::make_unique<AssignmentExpression>(std::move(*alternate));
                        return ConditionalExpression{std::tuple{std::move(*shortCircuit), std::move(consPtr), std::move(altPtr)}};
                    }
                }
            }
            state.restorePosition(start);
            return std::nullopt;
        }
        return ConditionalExpression{std::move(*shortCircuit)};
    }
    return std::nullopt;
}


inline std::optional<BindingElement> BindingElement::parse(ParserState& state) {
    if (auto id = BindingIdentifier::parse(state)) {
        auto annotation = TypeAnnotation::parse(state);

        if (state.current().kind == lex::Token::Punctuator && state.current().text == "=") {
            state.advance();
            if (auto initializer = state.pushCallPop<In(true)>([&](){ return AssignmentExpression::parse(state); })) {
                auto ptr = std::make_unique<AssignmentExpression>(std::move(*initializer));
                return BindingElement{std::pair{std::move(*id), std::move(ptr)}, std::move(annotation)};
            }
            state.backtrack();
            return std::nullopt;
        }
        return BindingElement{std::move(*id), std::move(annotation)};
    }
    if (auto pattern = BindingPattern::parse(state)) {
        auto annotation = TypeAnnotation::parse(state);

        if (state.current().kind == lex::Token::Punctuator && state.current().text == "=") {
            state.advance();
            if (auto initializer = state.pushCallPop<In(true)>([&](){ return AssignmentExpression::parse(state); })) {
                auto ptr = std::make_unique<AssignmentExpression>(std::move(*initializer));
                return BindingElement{std::pair{std::move(*pattern), std::move(ptr)}, std::move(annotation)};
            }
            state.backtrack();
            return std::nullopt;
        }
        return BindingElement{std::move(*pattern), std::move(annotation)};
    }

    return std::nullopt;
}


inline std::optional<Assignment> Assignment::parse(ParserState& state) {
    auto start = state.getPosition();
    auto lhs = LeftHandSideExpression::parse(state);
    if (!lhs) {
        return std::nullopt;
    }

    if (state.current().kind != lex::Token::Punctuator || !assignmentOperator.contains(state.current().text)) {
        state.restorePosition(start);
        state.error("Expected assignment operator");
        return std::nullopt;
    }

    std::string_view op = state.current().text;
    state.advance();

    auto rhs = AssignmentExpression::parse(state);
    if (!rhs) {
        state.restorePosition(start);
        return std::nullopt;
    }

    return Assignment{std::move(*lhs), std::make_unique<AssignmentExpression>(std::move(*rhs)), op};
}


inline std::optional<Arguments> Arguments::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
        return std::nullopt;
    }
    state.advance();

    Arguments args;
    if (state.current().kind == lex::Token::Punctuator && state.current().text == ")") {
        state.advance();
        return args;
    }

    while (true) {
        bool isSpread = false;
        if (state.current().kind == lex::Token::Punctuator && state.current().text == "...") {
            isSpread = true;
            state.advance();
        }

        if (auto expr = state.pushCallPop<In(true)>([&](){ return AssignmentExpression::parse(state); })) {
            args.arguments.push_back(std::pair{isSpread, std::make_unique<AssignmentExpression>(std::move(*expr))});
        }
        else {
            state.restorePosition(start);
            state.error("Invalid arguments");
            return std::nullopt;
        }

        if (state.current().kind == lex::Token::Punctuator && state.current().text == ",") {
            state.advance();
            continue;
        }

        if (state.current().kind == lex::Token::Punctuator && state.current().text == ")") {
            state.advance();
            break;
        }

        state.restorePosition(start);
        state.error("Expected , or )");
        return std::nullopt;
    }

    return args;
}


inline std::optional<LexicalBinding> LexicalBinding::parse(ParserState& state) {
    if (auto id = BindingIdentifier::parse(state)) {
        auto annotation = TypeAnnotation::parse(state);

        if (state.current().kind == lex::Token::Punctuator && state.current().text == "=") {
            state.advance();
            if (auto initializer = AssignmentExpression::parse(state)) {
                auto ptr = std::make_unique<AssignmentExpression>(std::move(*initializer));
                return LexicalBinding{std::pair{std::move(*id), std::move(ptr)}, std::move(annotation)};
            }
            state.backtrack();
            return std::nullopt;
        }
        return LexicalBinding{std::move(*id), std::move(annotation)};
    }

    auto start = state.getPosition();
    if (auto pattern = BindingPattern::parse(state)) {
        auto annotation = TypeAnnotation::parse(state);

        if (state.current().kind == lex::Token::Punctuator && state.current().text == "=") {
            state.advance();
            if (auto initializer = AssignmentExpression::parse(state)) {
                auto ptr = std::make_unique<AssignmentExpression>(std::move(*initializer));
                return LexicalBinding{std::pair{std::move(*pattern), std::move(ptr)}, std::move(annotation)};
            }
        }
    }

    state.restorePosition(start);
    return std::nullopt;
}


inline std::optional<Expression> Expression::parse(ParserState& state) {
    Expression expr;
    auto last = state.getPosition();
    while (true) {
        if (auto assignment = AssignmentExpression::parse(state)) {
            expr.items.push_back(std::make_unique<AssignmentExpression>(std::move(*assignment)));
            last = state.getPosition();
        }
        else {
            break;
        }

        if (state.current().kind != lex::Token::Punctuator || state.current().text != ",") {
            break;
        }
        state.advance();
    }
    state.restorePosition(last);
    if (expr.items.empty()) {
        return std::nullopt;
    }
    return expr;
}


inline std::optional<IfStatement> IfStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "if") {
        return std::nullopt;
    }
    state.advance();

    auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parseParenthesised(state); });
    if (!expr) {
        state.restorePosition(start);
        return std::nullopt;
    }

    auto consequent = Statement::parse(state);
    if (!consequent) {
        state.error("Expected statement");
        state.restorePosition(start);
        return std::nullopt;
    }

    if (state.current().kind == lex::Token::Keyword && state.current().text == "else") {
        state.advance();
        auto alternate = Statement::parse(state);

        if (!alternate) {
            state.error("Expected statement");
            state.restorePosition(start);
            return std::nullopt;
        }

        StatementPtr consequentPtr = std::make_unique<Statement>(std::move(*consequent));
        StatementPtr alternatePtr = std::make_unique<Statement>(std::move(*alternate));
        return IfStatement{std::move(*expr), std::move(consequentPtr), std::move(alternatePtr)};
    }

    StatementPtr consequentPtr = std::make_unique<Statement>(std::move(*consequent));
    return IfStatement{std::move(*expr), std::move(consequentPtr), std::nullopt};
}


inline std::optional<DoWhileStatement> DoWhileStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "do") {
        return std::nullopt;
    }
    state.advance();

    auto statement = Statement::parse(state);
    if (!statement) {
        state.error("Expected statement");
        state.restorePosition(start);
        return std::nullopt;
    }

    if (state.current().kind != lex::Token::Keyword || state.current().text != "while") {
        state.error("Expected while");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parseParenthesised(state); });
    if (!expr) {
        state.error("Expected expression");
        state.restorePosition(start);
        return std::nullopt;
    }

    if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
        state.error("Expected ;");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    StatementPtr statementPtr = std::make_unique<Statement>(std::move(*statement));
    return DoWhileStatement{std::move(statementPtr), std::move(*expr)};
}


inline std::optional<WhileStatement> WhileStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "while") {
        return std::nullopt;
    }
    state.advance();

    auto expr = state.pushCallPop<In(true)>([&](){ return Expression::parseParenthesised(state); });
    if (!expr) {
        state.restorePosition(start);
        return std::nullopt;
    }

    auto statement = Statement::parse(state);
    if (!statement) {
        state.error("Expected statement");
        state.restorePosition(start);
        return std::nullopt;
    }

    StatementPtr statementPtr = std::make_unique<Statement>(std::move(*statement));
    return WhileStatement{std::move(*expr), std::move(statementPtr)};
}


inline std::optional<ForStatement> ForStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "for") {
        return std::nullopt;
    }
    state.advance();

    if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
        state.error("Expected (");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    ForStatement forStmt;

    if (state.current().kind == lex::Token::Keyword && state.current().text == "var") {
        throw std::runtime_error("Variable declarations in for loop are not supported");
    }
    if (state.current().kind == lex::Token::Keyword && (state.current().text == "let" || state.current().text == "const")) {
        if (auto decl = state.pushCallPop<In(false)>([&](){ return LexicalDeclaration::parse(state); })) {
            forStmt.init = std::move(*decl);
        }
        else {
            state.restorePosition(start);
            return std::nullopt;
        }
        // semicolon as part of the declaration statement
    }
    else {
        if (auto expr = state.pushCallPop<In(false)>([&](){ return Expression::parse(state); })) {
            forStmt.init = std::move(*expr);
        }
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();
    }


    if (auto cond = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
        forStmt.condition = std::move(*cond);
    }


    if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
        state.error("Expected ;");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    if (auto update = state.pushCallPop<In(true)>([&](){ return Expression::parse(state); })) {
        forStmt.update = std::move(*update);
    }

    if (state.current().kind != lex::Token::Punctuator || state.current().text != ")") {
        state.error("Expected )");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    auto statement = Statement::parse(state);
    if (!statement) {
        state.error("Expected statement");
        state.restorePosition(start);
        return std::nullopt;
    }

    forStmt.statement = std::make_unique<Statement>(std::move(*statement));
    return forStmt;
}

inline StatementList::~StatementList() = default;

inline StatementList::StatementList() = default;

inline StatementList::StatementList(StatementList&&) = default;

inline BindingPattern::~BindingPattern() = default;

inline BindingPattern::BindingPattern(BindingPattern&&) = default;


}  // namespace jac::ast
