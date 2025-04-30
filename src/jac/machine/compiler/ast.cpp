#include "ast.h"


namespace jac::ast {


std::optional<IdentifierName> parseIdentifierName(ParserState& state, bool allowReserved) {
    if (state.current().kind == lex::Token::IdentifierName || (allowReserved && state.current().kind == lex::Token::Keyword)) {
        IdentifierName id;
        id = state.current().text;
        state.advance();
        return id;
    }

    return std::nullopt;
}


std::optional<Identifier> Identifier::parse(ParserState& state) {
    if (auto id = parseIdentifierName(state, false)) {
        return Identifier{*id};
    }

    return std::nullopt;
}


std::optional<IdentifierReference> IdentifierReference::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto id = Identifier::parse(state)) {
        if (state.getYield() && id->name == "yield") {
            state.restorePosition(start);
            state.error("Unexpected yield");
            return std::nullopt;
        }
        if (state.getAwait() && id->name == "await") {
            state.restorePosition(start);
            state.error("Unexpected await");
            return std::nullopt;
        }
        return IdentifierReference{*id};
    }

    return std::nullopt;
}


std::optional<BindingIdentifier> BindingIdentifier::parse(ParserState& state) {
    if (auto id = Identifier::parse(state)) {
        return BindingIdentifier{*id};
    }

    return std::nullopt;
}


std::optional<LabelIdentifier> LabelIdentifier::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto id = Identifier::parse(state)) {
        if (state.getYield() && id->name == "yield") {
            state.restorePosition(start);
            state.error("Unexpected yield");
            return std::nullopt;
        }
        if (state.getAwait() && id->name == "await") {
            state.restorePosition(start);
            state.error("Unexpected await");
            return std::nullopt;
        }
        return LabelIdentifier{*id};
    }

    return std::nullopt;
}


std::optional<PrivateIdentifier> PrivateIdentifier::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto id = parseIdentifierName(state, false)) {
        if (id->size() < 2 || (*id)[0] != '#') {
            state.restorePosition(start);
            state.error("Private identifier must start with #");
            return std::nullopt;
        }
        return PrivateIdentifier{*id};
    }

    return std::nullopt;
}


std::optional<ThisExpr> ThisExpr::parse(ParserState& state) {
    if (state.current().kind == lex::Token::Keyword && state.current().text == "this") {
        state.advance();
        return ThisExpr{};
    }

    return std::nullopt;
}


std::optional<NullLiteral> NullLiteral::parse(ParserState& state) {
    if (state.current().kind == lex::Token::Keyword && state.current().text == "null") {
        state.advance();
        return NullLiteral{};
    }

    return std::nullopt;
}


std::optional<BooleanLiteral> BooleanLiteral::parse(ParserState& state) {
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


std::optional<NumericLiteral> NumericLiteral::parse(ParserState& state) {
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


std::optional<StringLiteral> StringLiteral::parse(ParserState& state) {
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


std::optional<Literal> Literal::parse(ParserState& state) {
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


std::optional<UnaryExpression> UnaryExpression::parse(ParserState& state) {
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


std::optional<BinaryExpression> BinaryExpression::parse(ParserState& state) {
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
        output.emplace_back(std::make_unique<UnaryExpression>(std::move(*unary)));
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
            output.emplace_back(top);
        }
        operators.push_back(next.text);
        state.advance();
    }
    while (!operators.empty()) {
        output.emplace_back(operators.back());
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


std::optional<BindingPattern> BindingPattern::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<TypeAnnotation> TypeAnnotation::parse(ParserState& state) {
    if (state.current().kind == lex::Token::Punctuator && state.current().text == ":") {
        state.advance();
        if (state.current().kind == lex::Token::Keyword && !allowedKWTypes.contains(state.current().text)) {
            state.error("Invalid type annotation");
            return std::nullopt;
        }
        if (auto id = parseIdentifierName(state, true)) {
            return TypeAnnotation{*id};
        }
    }

    return std::nullopt;
}


std::optional<BindingRestElement> BindingRestElement::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<FormalParameters> FormalParameters::parse(ParserState& state) {
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


std::optional<FunctionBody> FunctionBody::parse(ParserState& state) {
    if (auto stmts = (state.pushTemplate<Return{true}>(), StatementList::parse(state))) {
        return FunctionBody{std::move(*stmts)};
    }
    return std::nullopt;
}


std::optional<Block> Block::parse(ParserState& state) {
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



std::optional<LexicalDeclaration> LexicalDeclaration::parse(ParserState& state) {
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


std::optional<VariableStatement> VariableStatement::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<EmptyStatement> EmptyStatement::parse(ParserState& state) {
    if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
        return std::nullopt;
    }
    state.advance();
    return EmptyStatement{};
}


std::optional<ExpressionStatement> ExpressionStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto expr = (state.pushTemplate<In{false}>(), Expression::parse(state))) {
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


std::optional<IterationStatement> IterationStatement::parse(ParserState& state) {
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


std::optional<SwitchStatement> SwitchStatement::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<BreakableStatement> BreakableStatement::parse(ParserState& state) {
    if (auto iter = IterationStatement::parse(state)) {
        return BreakableStatement{std::move(*iter)};
    }
    if (auto sw = SwitchStatement::parse(state)) {
        return BreakableStatement{std::move(*sw)};
    }

    return std::nullopt;
}


std::optional<ContinueStatement> ContinueStatement::parse(ParserState& state) {
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


std::optional<BreakStatement> BreakStatement::parse(ParserState& state) {
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


std::optional<ReturnStatement> ReturnStatement::parse(ParserState& state) {
    if (state.current().kind != lex::Token::Keyword || state.current().text != "return") {
        return std::nullopt;
    }
    state.advance();

    ReturnStatement statement;

    auto start = state.getPosition();
    if (auto expr = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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


std::optional<WithStatement> WithStatement::parse(ParserState&) {
    return std::nullopt;
}


std::optional<FunctionDeclaration> FunctionDeclaration::parse(ParserState& state, bool Default) {
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

    auto params = (state.pushTemplate<Yield{false}, Await{false}>(), FormalParameters::parse(state));
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

    auto body = (state.pushTemplate<Yield{false}, Await{false}>(), FunctionBody::parse(state));

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


std::optional<LabeledStatement> LabeledStatement::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<ThrowStatement> ThrowStatement::parse(ParserState& state) {
    if (state.current().kind != lex::Token::Keyword || state.current().text != "throw") {
        return std::nullopt;
    }
    state.advance();

    ThrowStatement statement;

    auto start = state.getPosition();
    if (auto expr = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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


std::optional<TryStatement> TryStatement::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<DebuggerStatement> DebuggerStatement::parse(ParserState& state) {
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


std::optional<Statement> Statement::parse(ParserState& state) {
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


std::optional<GeneratorDeclaration> GeneratorDeclaration::parse(ParserState&, bool) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<AsyncFunctionDeclaration> AsyncFunctionDeclaration::parse(ParserState&, bool) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<AsyncGeneratorDeclaration> AsyncGeneratorDeclaration::parse(ParserState&, bool) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<HoistableDeclaration> HoistableDeclaration::parse(ParserState& state, bool Default) {
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


std::optional<ClassDeclaration> ClassDeclaration::parse(ParserState&, bool) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<Declaration> Declaration::parse(ParserState& state) {
    if (auto hoistable = HoistableDeclaration::parse(state, false)) {
        return Declaration{std::move(*hoistable)};
    }
    if (auto klass = ClassDeclaration::parse(state, false)) {
        return Declaration{std::move(*klass)};
    }
    if (auto lexical = (state.pushTemplate<In{true}>(), LexicalDeclaration::parse(state))) {
        return Declaration{std::move(*lexical)};
    }
    return std::nullopt;
}


std::optional<StatementListItem> StatementListItem::parse(ParserState& state) {
    if (auto statement = Statement::parse(state)) {
        return StatementListItem{std::move(*statement)};
    }
    if (auto declaration = Declaration::parse(state)) {
        return StatementListItem{std::move(*declaration)};
    }

    return std::nullopt;
}


std::optional<RegularExpressionLiteral> RegularExpressionLiteral::parse(ParserState&) {
    return std::nullopt;
}


std::optional<TemplateLiteral> TemplateLiteral::parse(ParserState&, bool) {
    return std::nullopt;
}


std::optional<CoverParenthesizedExpressionAndArrowParameterList> CoverParenthesizedExpressionAndArrowParameterList::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Punctuator || state.current().text != "(") {
        return std::nullopt;
    }
    state.advance();

    CoverParenthesizedExpressionAndArrowParameterList result;
    bool canContinue = true;

    if (auto expr = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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
                result.parameters.template emplace<BindingIdentifier>(std::move(*binding));
            }
            else if (auto pattern = BindingPattern::parse(state)) {
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


std::optional<ParenthesizedExpression> CoverParenthesizedExpressionAndArrowParameterList::refineParExp() {
    // XXX: investigate more
    if (!std::holds_alternative<std::monostate>(parameters)) {
        return std::nullopt;
    }
    if (!expression) {
        return std::nullopt;
    }
    return ParenthesizedExpression{std::move(*expression)};
}


std::optional<PrimaryExpression> PrimaryExpression::parse(ParserState& state) {
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


std::optional<SuperProperty> SuperProperty::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<MetaProperty> MetaProperty::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<MemberExpression> MemberExpression::parse(ParserState& state) {
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
            if (auto expr = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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
            if (auto identifier = parseIdentifierName(state, false)) {
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


std::optional<NewExpression> NewExpression::parse(ParserState& state) {
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


std::optional<SuperCall> SuperCall::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<ImportCall> ImportCall::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<CoverCallExpressionAndAsyncArrowHead> CoverCallExpressionAndAsyncArrowHead::parse(ParserState& state) {
    auto start = state.getPosition();
    if (auto member = MemberExpression::parse(state)) {
        if (auto args = Arguments::parse(state)) {
            return CoverCallExpressionAndAsyncArrowHead{std::move(*member), std::move(*args)};
        }
    }

    state.restorePosition(start);
    return std::nullopt;
}


std::optional<CallExpression> CallExpression::parse(ParserState& state) {
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
            if (auto expr = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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
            if (auto identifier = parseIdentifierName(state, false)) {
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


std::optional<LeftHandSideExpression> LeftHandSideExpression::parse(ParserState& state) {
    if (auto call = CallExpression::parse(state)) {
        return LeftHandSideExpression{std::move(*call)};
    }
    if (auto newExpr = NewExpression::parse(state)) {
        return LeftHandSideExpression{std::move(*newExpr)};
    }
    // TODO: rest

    return std::nullopt;
}


std::optional<YieldExpression> YieldExpression::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<ArrowFunction> ArrowFunction::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<AsyncArrowFunction> AsyncArrowFunction::parse(ParserState&) {
    // XXX: ignore for now
    return std::nullopt;
}


std::optional<AssignmentExpression> AssignmentExpression::parse(ParserState& state) {
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


std::optional<Script> Script::parse(ParserState& state) {
    if (state.isEnd()) {
        return Script{};
    }

    auto _ = state.pushTemplate<Yield{false}, Await{false}, Return{false}>();

    if (auto statementList = StatementList::parse(state)) {
        return Script{std::move(*statementList)};
    }

    return std::nullopt;
}


std::optional<UpdateExpression> UpdateExpression::parse(ParserState& state) {
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


std::optional<ConditionalExpression> ConditionalExpression::parse(ParserState& state) {
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


std::optional<BindingElement> BindingElement::parse(ParserState& state) {
    if (auto id = BindingIdentifier::parse(state)) {
        auto annotation = TypeAnnotation::parse(state);

        if (state.current().kind == lex::Token::Punctuator && state.current().text == "=") {
            state.advance();
            if (auto initializer = (state.pushTemplate<In{true}>(), AssignmentExpression::parse(state))) {
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
            if (auto initializer = (state.pushTemplate<In{true}>(), AssignmentExpression::parse(state))) {
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


std::optional<Assignment> Assignment::parse(ParserState& state) {
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


std::optional<Arguments> Arguments::parse(ParserState& state) {
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

        if (auto expr = (state.pushTemplate<In{true}>(), AssignmentExpression::parse(state))) {
            args.arguments.emplace_back(std::pair{isSpread, std::make_unique<AssignmentExpression>(std::move(*expr))});
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


std::optional<LexicalBinding> LexicalBinding::parse(ParserState& state) {
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


std::optional<Expression> Expression::parse(ParserState& state) {
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


std::optional<IfStatement> IfStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "if") {
        return std::nullopt;
    }
    state.advance();

    auto expr = (state.pushTemplate<In{true}>(), Expression::parseParenthesised(state));
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


std::optional<DoWhileStatement> DoWhileStatement::parse(ParserState& state) {
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

    auto expr = (state.pushTemplate<In{true}>(), Expression::parseParenthesised(state));
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


std::optional<WhileStatement> WhileStatement::parse(ParserState& state) {
    auto start = state.getPosition();
    if (state.current().kind != lex::Token::Keyword || state.current().text != "while") {
        return std::nullopt;
    }
    state.advance();

    auto expr = (state.pushTemplate<In{true}>(), Expression::parseParenthesised(state));
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


std::optional<ForInOfStatement> ForInOfStatement::parse(ParserState&) {
    return std::nullopt;
}


std::optional<ForStatement> ForStatement::parse(ParserState& state) {
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
        if (auto decl = (state.pushTemplate<In{false}>(), LexicalDeclaration::parse(state))) {
            forStmt.init = std::move(*decl);
        }
        else {
            state.restorePosition(start);
            return std::nullopt;
        }
        // semicolon as part of the declaration statement
    }
    else {
        if (auto expr = (state.pushTemplate<In{false}>(), Expression::parse(state))) {
            forStmt.init = std::move(*expr);
        }
        if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
            state.error("Expected ;");
            state.restorePosition(start);
            return std::nullopt;
        }
        state.advance();
    }


    if (auto cond = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
        forStmt.condition = std::move(*cond);
    }


    if (state.current().kind != lex::Token::Punctuator || state.current().text != ";") {
        state.error("Expected ;");
        state.restorePosition(start);
        return std::nullopt;
    }
    state.advance();

    if (auto update = (state.pushTemplate<In{true}>(), Expression::parse(state))) {
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


std::optional<StatementList> StatementList::parse(ParserState& state) {
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


}  // namespace jac::ast
