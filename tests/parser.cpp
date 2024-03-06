#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdint>
#include <string>

#include <jac/machine/parser/grammar.h>


template<>
struct Catch::StringMaker<jac::Token> {
    static std::string convert(jac::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.kind) + ")";
    }
};


bool floatEq(auto a, auto b) {
    return std::fabs(a - b) < 0.001;
}


jac::PrimaryExpression<true, true>& unaryExpPrimary(jac::UnaryExpression<true, true>& expr) {
    return std::get<jac::PrimaryExpression<true, true>>(
      std::get<jac::MemberExpression<true, true>>(
        std::get<jac::NewExpression<true, true>>(
          std::get<jac::LeftHandSideExpressionPtr<true, true>>(
            std::get<jac::UpdateExpression<true, true>>(expr.value)
          .value)
        ->value)
      .value)
    .value);
}

jac::Literal& unaryExpLiteral(jac::UnaryExpression<true, true>& expr) {
    return std::get<jac::Literal>(
        unaryExpPrimary(expr)
    .value);
}

std::int32_t literalI32(jac::Literal& lit) {
    return std::get<std::int32_t>(
      std::get<jac::NumericLiteral>(
        lit.value)
      .value);
}

std::int32_t unaryExpI32(jac::UnaryExpression<true, true>& expr) {
    return literalI32(unaryExpLiteral(expr));
}

std::string_view unaryExpIdentifier(jac::UnaryExpression<true, true>& expr) {
    return std::get<jac::IdentifierReference<true, true>>(
      std::get<jac::PrimaryExpression<true, true>>(
        std::get<jac::MemberExpression<true, true>>(
          std::get<jac::NewExpression<true, true>>(
            std::get<jac::LeftHandSideExpressionPtr<true, true>>(
              std::get<jac::UpdateExpression<true, true>>(expr.value)
            .value)
          ->value)
        .value)
      .value)
    .value).identifier.name.name;
}

jac::BinaryExpression<true, true, true>& assignExpBinary(jac::AssignmentExpression<true, true, true>& expr) {
    return std::get<jac::BinaryExpression<true, true, true>>(
        std::get<jac::ConditionalExpression<true, true, true>>(expr.value)
    .value);
}

jac::UnaryExpression<true, true>& assignExpUnary(jac::AssignmentExpression<true, true, true>& expr) {
    return std::get<jac::UnaryExpression<true, true>>(assignExpBinary(expr).value);
}

using TokenVector = std::vector<jac::Token>;


TEST_CASE("NumericLiteral", "[parser]") {

    SECTION("123") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 123);
    }

    SECTION("123.456") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123.456", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(std::get<double>(result->value) == 123.456);
    }

    SECTION("123.456e-7") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123.456e-7", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e-7));
    }

    SECTION("123.456e+7") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123.456e+7", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e+7));
    }

    SECTION("12_3.45_6e+7") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "12_3.45_6e+7", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e+7));
    }

    SECTION("0x0123") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0x0123", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0x0123);
    }

    SECTION("0o0123") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0o0123", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0123);
    }

    SECTION("0b1010") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0b1010", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0b1010);
    }

    SECTION("0123456789") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0123456789", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 123456789);
    }

    SECTION("0x0123456789abcdef") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0x0123456789abcdef", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 0x0123456789abcdef));
    }

    SECTION("0x0123456789ABCDEF") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0x0123456789ABCDEF", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        CAPTURE(std::get<double>(result->value));
        CAPTURE(0x0123456789ABCDEF);
        REQUIRE(floatEq(std::get<double>(result->value), 0x0123456789ABCDEF));
    }

    SECTION("0") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 0);
    }

    SECTION("invalid") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123.456e", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("invalid 2") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "0x", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("BooleanLiteral", "[parser]") {

    SECTION("true") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "true", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == true);
    }

    SECTION("false") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "false", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == false);
    }

    SECTION("tru") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "tru", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("Identifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::Identifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name.name == "label");
    }

    SECTION("var") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "var", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::Identifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("IdentifierReference", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::IdentifierReference<false, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::IdentifierReference<true, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::IdentifierReference<false, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("BindingIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BindingIdentifier<true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->identifier.name.name == "label");
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BindingIdentifier<true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::BindingIdentifier<false, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("LabelIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::LabelIdentifier<false, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::LabelIdentifier<true, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "yield", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::LabelIdentifier<false, false>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("PrivateIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::PrivateIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("#label") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "#label", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::PrivateIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name.name == "#label");
    }
}


TEST_CASE("NullLiteral", "[parser]") {

    SECTION("null") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "null", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::NullLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("nul") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "nul", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::NullLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("ThisExpr", "[parser]") {

    SECTION("this") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "this", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::ThisExpr::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }
}


TEST_CASE("StringLiteral", "[parser]") {

    SECTION("\"hello\"") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "\"hello\"", jac::Token::StringLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::StringLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == "hello");
    }

    SECTION("\"hello\\nworld\"") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "\"hello\\nworld\"", jac::Token::StringLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::StringLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == "hello\nworld");
    }
}


TEST_CASE("Literal", "[parser]") {
    SECTION("NumericLiteral") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::NumericLiteral>(result->value));
        auto lit = std::get<jac::NumericLiteral>(result->value);
        REQUIRE(std::holds_alternative<int>(lit.value));
        REQUIRE(std::get<int>(lit.value) == 123);
    }

    SECTION("BooleanLiteral") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "true", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::BooleanLiteral>(result->value));
        REQUIRE(std::get<jac::BooleanLiteral>(result->value).value == true);
    }

    SECTION("NullLiteral") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "null", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::NullLiteral>(result->value));
    }

    SECTION("StringLiteral") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "\"hello\"", jac::Token::StringLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::StringLiteral>(result->value));
        REQUIRE(std::get<jac::StringLiteral>(result->value).value == "hello");
    }

    SECTION("invalid") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "123.456e", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("LexicalDeclaration", "[parser]") {

    SECTION("let x;") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "let", jac::Token::Keyword),
            jac::Token(1, 5, "x", jac::Token::IdentifierName),
            jac::Token(1, 7, ";", jac::Token::Punctuator)
        };

        jac::ParserState state(tokens);

        auto result = jac::LexicalDeclaration<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == false);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<jac::BindingIdentifier<true, true>>(binding.value));
        REQUIRE(std::get<jac::BindingIdentifier<true, true>>(binding.value).identifier.name.name == "x");
    }

    SECTION("const x = 123;") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "const", jac::Token::Keyword),
            jac::Token(1, 7, "x", jac::Token::IdentifierName),
            jac::Token(1, 9, "=", jac::Token::Punctuator),
            jac::Token(1, 11, "123", jac::Token::NumericLiteral),
            jac::Token(1, 14, ";", jac::Token::Punctuator)
        };

        jac::ParserState state(tokens);

        auto result = jac::LexicalDeclaration<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == true);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<std::pair<jac::BindingIdentifier<true, true>, jac::InitializerPtr<true, true, true>>>(binding.value));
        auto& pair = std::get<std::pair<jac::BindingIdentifier<true, true>, jac::InitializerPtr<true, true, true>>>(binding.value);
        REQUIRE(pair.first.identifier.name.name == "x");
        REQUIRE(
            std::get<std::int32_t>(
              std::get<jac::NumericLiteral>(
                std::get<jac::Literal>(
                  std::get<jac::PrimaryExpression<true, true>>(
                    std::get<jac::MemberExpression<true, true>>(
                      std::get<jac::NewExpression<true, true>>(
                        std::get<jac::LeftHandSideExpressionPtr<true, true>>(
                          std::get<jac::UpdateExpression<true, true>>(
                            std::get<jac::UnaryExpression<true, true>>(
                              std::get<jac::BinaryExpression<true, true, true>>(
                                std::get<jac::ConditionalExpression<true, true, true>>(pair.second->value)
                              .value)
                            .value)
                          .value)
                        .value)
                      ->value)
                    .value)
                  .value)
                .value)
              .value)
            .value) == 123
        );
    }

    SECTION("const x = abc + def;") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "const", jac::Token::Keyword),
            jac::Token(1, 7, "x", jac::Token::IdentifierName),
            jac::Token(1, 9, "=", jac::Token::Punctuator),
            jac::Token(1, 11, "abc", jac::Token::IdentifierName),
            jac::Token(1, 15, "+", jac::Token::Punctuator),
            jac::Token(1, 17, "def", jac::Token::IdentifierName),
            jac::Token(1, 20, ";", jac::Token::Punctuator)
        };

        jac::ParserState state(tokens);

        auto result = jac::LexicalDeclaration<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == true);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<std::pair<jac::BindingIdentifier<true, true>, jac::InitializerPtr<true, true, true>>>(binding.value));
        auto& pair = std::get<std::pair<jac::BindingIdentifier<true, true>, jac::InitializerPtr<true, true, true>>>(binding.value);
        REQUIRE(pair.first.identifier.name.name == "x");
        auto& exp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(
            std::get<jac::BinaryExpression<true, true, true>>(
                std::get<jac::ConditionalExpression<true, true, true>>(pair.second->value)
            .value)
        .value);

        REQUIRE((std::get<2>(exp) == "+"));
        REQUIRE(
            std::get<jac::IdentifierReference<true, true>>(
              std::get<jac::PrimaryExpression<true, true>>(
                std::get<jac::MemberExpression<true, true>>(
                  std::get<jac::NewExpression<true, true>>(
                    std::get<jac::LeftHandSideExpressionPtr<true, true>>(
                        std::get<jac::UpdateExpression<true, true>>(
                        std::get<jac::UnaryExpression<true, true>>(
                          std::get<0>(exp)
                        ->value)
                      .value)
                    .value)
                  ->value)
                .value)
              .value)
            .value).identifier.name.name == "abc"
        );
        REQUIRE(
            std::get<jac::IdentifierReference<true, true>>(
              std::get<jac::PrimaryExpression<true, true>>(
                std::get<jac::MemberExpression<true, true>>(
                  std::get<jac::NewExpression<true, true>>(
                    std::get<jac::LeftHandSideExpressionPtr<true, true>>(
                      std::get<jac::UpdateExpression<true, true>>(
                        std::get<jac::UnaryExpression<true, true>>(
                          std::get<1>(exp)
                        ->value)
                      .value)
                    .value)
                  ->value)
                .value)
              .value)
            .value).identifier.name.name == "def"
        );
    }
}


TEST_CASE("UnaryExpression", "[parser]") {

    SECTION("+ - 123") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "+", jac::Token::Punctuator),
            jac::Token(1, 3, "-", jac::Token::Punctuator),
            jac::Token(1, 5, "123", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::UnaryExpression<true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE((result->op == "+"));

        auto& inner = *std::get<jac::UnaryExpressionPtr<true, true>>(result->value);
        REQUIRE((inner.op == "-"));

        auto& numExpr = std::get<jac::UnaryExpressionPtr<true, true>>(inner.value);

        REQUIRE(literalI32(unaryExpLiteral(*numExpr)) == 123);
    }

    SECTION("- a++") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "-", jac::Token::Punctuator),
            jac::Token(1, 3, "a", jac::Token::IdentifierName),
            jac::Token(1, 4, "++", jac::Token::Punctuator)
        };

        jac::ParserState state(tokens);

        auto result = jac::UnaryExpression<true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE((result->op == "-"));

        auto& inner = *std::get<jac::UnaryExpressionPtr<true, true>>(result->value);
        REQUIRE((std::get<jac::UpdateExpression<true, true>>(inner.value).op == "x++"));

        REQUIRE((unaryExpIdentifier(inner) == "a"));
    }
}


TEST_CASE("BinaryExpression", "[parser]") {

    SECTION("1 + 2") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "1", jac::Token::NumericLiteral),
            jac::Token(1, 3, "+", jac::Token::Punctuator),
            jac::Token(1, 5, "2", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::BinaryExpression<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& exp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(result->value);
        REQUIRE((std::get<2>(exp) == "+"));
        auto& lhs = std::get<0>(exp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(lhs->value)) == 1);

        auto& rhs = std::get<1>(exp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(rhs->value)) == 2);
    }

    SECTION("1 + 2 * 3") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "1", jac::Token::NumericLiteral),
            jac::Token(1, 3, "+", jac::Token::Punctuator),
            jac::Token(1, 5, "2", jac::Token::NumericLiteral),
            jac::Token(1, 7, "*", jac::Token::Punctuator),
            jac::Token(1, 9, "3", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::BinaryExpression<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& addExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(result->value);
        REQUIRE((std::get<2>(addExp) == "+"));
        auto& addLhs = std::get<0>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(addLhs->value)) == 1);

        auto& addRhs = std::get<1>(addExp);
        auto& mulExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(addRhs->value);
        REQUIRE((std::get<2>(mulExp) == "*"));
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<0>(mulExp)->value)) == 2);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<1>(mulExp)->value)) == 3);
    }

    SECTION("1 + 2 * 3 * 4 - 5") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "1", jac::Token::NumericLiteral),
            jac::Token(1, 3, "+", jac::Token::Punctuator),
            jac::Token(1, 5, "2", jac::Token::NumericLiteral),
            jac::Token(1, 7, "*", jac::Token::Punctuator),
            jac::Token(1, 9, "3", jac::Token::NumericLiteral),
            jac::Token(1, 11, "*", jac::Token::Punctuator),
            jac::Token(1, 13, "4", jac::Token::NumericLiteral),
            jac::Token(1, 15, "-", jac::Token::Punctuator),
            jac::Token(1, 17, "5", jac::Token::NumericLiteral)
        };

        jac::ParserState state(tokens);

        auto result = jac::BinaryExpression<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& addExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(result->value);
        REQUIRE((std::get<2>(addExp) == "+"));
        auto& addLhs = std::get<0>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(addLhs->value)) == 1);

        auto& addRhs = std::get<1>(addExp);
        auto& subExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(addRhs->value);
        REQUIRE((std::get<2>(subExp) == "-"));
        auto& subRhs = std::get<1>(subExp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(subRhs->value)) == 5);

        auto& subLhs = std::get<0>(subExp);
        auto& mulExp1 = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(subLhs->value);
        REQUIRE((std::get<2>(mulExp1) == "*"));

        auto& mulExp1Lhs = std::get<0>(mulExp1);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(mulExp1Lhs->value)) == 2);

        auto& mulExp1Rhs = std::get<1>(mulExp1);
        auto& mulExp2 = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(mulExp1Rhs->value);
        REQUIRE((std::get<2>(mulExp2) == "*"));
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<0>(mulExp2)->value)) == 3);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<1>(mulExp2)->value)) == 4);
    }

    SECTION("1 + (2 * 3)") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "1", jac::Token::NumericLiteral),
            jac::Token(1, 3, "+", jac::Token::Punctuator),
            jac::Token(1, 5, "(", jac::Token::Punctuator),
            jac::Token(1, 6, "2", jac::Token::NumericLiteral),
            jac::Token(1, 8, "*", jac::Token::Punctuator),
            jac::Token(1, 10, "3", jac::Token::NumericLiteral),
            jac::Token(1, 12, ")", jac::Token::Punctuator)
        };

        jac::ParserState state(tokens);

        auto result = jac::BinaryExpression<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& addExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(result->value);
        REQUIRE((std::get<2>(addExp) == "+"));
        auto& addLhs = std::get<0>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(addLhs->value)) == 1);

        auto& addRhs = std::get<1>(addExp);
        auto& primary = unaryExpPrimary(std::get<jac::UnaryExpression<true, true>>(addRhs->value));
        auto& parExp = std::get<jac::ParenthesizedExpression<true, true>>(primary.value).expression;
        REQUIRE(parExp.items.size() == 1);
        auto& mulExp = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(
            std::get<jac::BinaryExpression<true, true, true>>(
            std::get<jac::ConditionalExpression<true, true, true>>(parExp.items[0]->value).value
        ).value);
        REQUIRE((std::get<2>(mulExp) == "*"));
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<0>(mulExp)->value)) == 2);
        REQUIRE(unaryExpI32(std::get<jac::UnaryExpression<true, true>>(std::get<1>(mulExp)->value)) == 3);
    }
}


TEST_CASE("ConditionalExpression", "[parser]") {

    SECTION("a && b ? c : d + e") {
        auto tokens = TokenVector{
            jac::Token(1, 1, "a", jac::Token::IdentifierName),
            jac::Token(1, 3, "&&", jac::Token::Punctuator),
            jac::Token(1, 6, "b", jac::Token::IdentifierName),
            jac::Token(1, 8, "?", jac::Token::Punctuator),
            jac::Token(1, 10, "c", jac::Token::IdentifierName),
            jac::Token(1, 12, ":", jac::Token::Punctuator),
            jac::Token(1, 14, "d", jac::Token::IdentifierName),
            jac::Token(1, 16, "+", jac::Token::Punctuator),
            jac::Token(1, 18, "e", jac::Token::IdentifierName)
        };

        jac::ParserState state(tokens);

        auto result = jac::ConditionalExpression<true, true, true>::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& condTup = std::get<std::tuple<
            jac::BinaryExpression<true, true, true>,
            jac::AssignmentExpressionPtr<true, true, true>,
            jac::AssignmentExpressionPtr<true, true, true>
        >>(result->value);

        auto& andTup = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(std::get<0>(condTup).value);
        REQUIRE((std::get<2>(andTup) == "&&"));

        REQUIRE((unaryExpIdentifier(std::get<jac::UnaryExpression<true, true>>(std::get<0>(andTup)->value)) == "a"));
        REQUIRE((unaryExpIdentifier(std::get<jac::UnaryExpression<true, true>>(std::get<1>(andTup)->value)) == "b"));

        auto& consequent = assignExpUnary(*std::get<1>(condTup));
        REQUIRE((unaryExpIdentifier(consequent) == "c"));

        auto& alternate = assignExpBinary(*std::get<2>(condTup));
        auto& addTuple = std::get<std::tuple<jac::BinaryExpressionPtr<true, true, true>, jac::BinaryExpressionPtr<true, true, true>, std::string_view>>(alternate.value);
        REQUIRE((std::get<2>(addTuple) == "+"));
        REQUIRE((unaryExpIdentifier(std::get<jac::UnaryExpression<true, true>>(std::get<0>(addTuple)->value)) == "d"));
        REQUIRE((unaryExpIdentifier(std::get<jac::UnaryExpression<true, true>>(std::get<1>(addTuple)->value)) == "e"));
    }
}
