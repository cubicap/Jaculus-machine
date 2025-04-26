#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdint>
#include <string>
#include <variant>

#include <jac/machine/compiler/ast.h>


template<>
struct Catch::StringMaker<jac::lex::Token> {
    static std::string convert(jac::lex::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.kind) + ")";
    }
};


bool floatEq(auto a, auto b) {
    return std::fabs(a - b) < 0.001;
}


jac::ast::PrimaryExpression& unaryExpPrimary(jac::ast::UnaryExpressionPtr& expr) {
    return std::get<jac::ast::PrimaryExpression>(
      std::get<jac::ast::MemberExpression>(
        std::get<jac::ast::NewExpression>(
          std::get<jac::ast::LeftHandSideExpressionPtr>(
            std::get<jac::ast::UpdateExpression>(expr->value)
          .value)
        ->value)
      .value)
    .value);
}

jac::ast::Literal& unaryExpLiteral(jac::ast::UnaryExpressionPtr& expr) {
    return std::get<jac::ast::Literal>(
        unaryExpPrimary(expr)
    .value);
}

std::int32_t literalI32(jac::ast::Literal& lit) {
    return std::get<std::int32_t>(
      std::get<jac::ast::NumericLiteral>(
        lit.value)
      .value);
}

std::int32_t unaryExpI32(jac::ast::UnaryExpressionPtr& expr) {
    return literalI32(unaryExpLiteral(expr));
}

std::string_view unaryExpIdentifier(jac::ast::UnaryExpressionPtr& expr) {
    return std::get<jac::ast::IdentifierReference>(
      std::get<jac::ast::PrimaryExpression>(
        std::get<jac::ast::MemberExpression>(
          std::get<jac::ast::NewExpression>(
            std::get<jac::ast::LeftHandSideExpressionPtr>(
              std::get<jac::ast::UpdateExpression>(expr->value)
            .value)
          ->value)
        .value)
      .value)
    .value).identifier.name;
}

jac::ast::BinaryExpression& assignExpBinary(jac::ast::AssignmentExpression& expr) {
    return std::get<jac::ast::BinaryExpression>(
        std::get<jac::ast::ConditionalExpression>(expr.value)
    .value);
}

jac::ast::UnaryExpressionPtr& assignExpUnary(jac::ast::AssignmentExpression& expr) {
    return std::get<jac::ast::UnaryExpressionPtr>(assignExpBinary(expr).value);
}

auto lhsExpIdentifier(jac::ast::LeftHandSideExpression& expr) {
    return std::get<jac::ast::IdentifierReference>(
      std::get<jac::ast::PrimaryExpression>(
        std::get<jac::ast::MemberExpression>(
          std::get<jac::ast::NewExpression>(expr.value)
          .value)
      .value)
    .value).identifier.name;
}


using TokenVector = std::vector<jac::lex::Token>;


TEST_CASE("NumericLiteral", "[parser]") {

    SECTION("123") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 123);
    }

    SECTION("123.456") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123.456", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(std::get<double>(result->value) == 123.456);
    }

    SECTION("123.456e-7") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123.456e-7", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e-7));
    }

    SECTION("123.456e+7") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123.456e+7", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e+7));
    }

    SECTION("12_3.45_6e+7") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "12_3.45_6e+7", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 123.456e+7));
    }

    SECTION("0x0123") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0x0123", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0x0123);
    }

    SECTION("0o0123") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0o0123", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0123);
    }

    SECTION("0b1010") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0b1010", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<std::int32_t>(result->value));
        REQUIRE(std::get<std::int32_t>(result->value) == 0b1010);
    }

    SECTION("0123456789") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0123456789", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 123456789);
    }

    SECTION("0x0123456789abcdef") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0x0123456789abcdef", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<double>(result->value));
        REQUIRE(floatEq(std::get<double>(result->value), 0x0123456789abcdef));
    }

    SECTION("0x0123456789ABCDEF") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0x0123456789ABCDEF", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
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
            jac::lex::Token(1, 1, "0", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<int>(result->value));
        REQUIRE(std::get<int>(result->value) == 0);
    }

    SECTION("invalid") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123.456e", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("invalid 2") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "0x", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NumericLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("BooleanLiteral", "[parser]") {

    SECTION("true") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "true", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == true);
    }

    SECTION("false") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "false", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == false);
    }

    SECTION("tru") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "tru", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BooleanLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("Identifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Identifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name == "label");
    }

    SECTION("var") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "var", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Identifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("IdentifierReference", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IdentifierReference::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IdentifierReference::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IdentifierReference::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("BindingIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BindingIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->identifier.name == "label");
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BindingIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BindingIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("LabelIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LabelIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("yield fail") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LabelIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("yield fail2") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "yield", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LabelIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("PrivateIdentifier", "[parser]") {

    SECTION("label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::PrivateIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("#label") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "#label", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::PrivateIdentifier::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name == "#label");
    }
}


TEST_CASE("NullLiteral", "[parser]") {

    SECTION("null") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "null", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NullLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("nul") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "nul", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::NullLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("ThisExpr", "[parser]") {

    SECTION("this") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "this", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::ThisExpr::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }
}


TEST_CASE("StringLiteral", "[parser]") {

    SECTION("\"hello\"") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "\"hello\"", jac::lex::Token::StringLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::StringLiteral::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->value == "hello");
    }

    SECTION("\"hello\\nworld\"") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "\"hello\\nworld\"", jac::lex::Token::StringLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::StringLiteral::parse(state);
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
            jac::lex::Token(1, 1, "123", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::ast::NumericLiteral>(result->value));
        auto lit = std::get<jac::ast::NumericLiteral>(result->value);
        REQUIRE(std::holds_alternative<int>(lit.value));
        REQUIRE(std::get<int>(lit.value) == 123);
    }

    SECTION("BooleanLiteral") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "true", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::ast::BooleanLiteral>(result->value));
        REQUIRE(std::get<jac::ast::BooleanLiteral>(result->value).value == true);
    }

    SECTION("NullLiteral") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "null", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::ast::NullLiteral>(result->value));
    }

    SECTION("StringLiteral") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "\"hello\"", jac::lex::Token::StringLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(std::holds_alternative<jac::ast::StringLiteral>(result->value));
        REQUIRE(std::get<jac::ast::StringLiteral>(result->value).value == "hello");
    }

    SECTION("invalid") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "123.456e", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Literal::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }
}


TEST_CASE("LexicalDeclaration", "[parser]") {

    SECTION("let x;") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "let", jac::lex::Token::Keyword),
            jac::lex::Token(1, 5, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 7, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LexicalDeclaration::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == false);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<jac::ast::BindingIdentifier>(binding.value));
        REQUIRE(std::get<jac::ast::BindingIdentifier>(binding.value).identifier.name == "x");
    }

    SECTION("const x = 123;") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "const", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "123", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 14, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LexicalDeclaration::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == true);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(binding.value));
        auto& pair = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(binding.value);
        REQUIRE(pair.first.identifier.name == "x");
        REQUIRE(
            std::get<std::int32_t>(
              std::get<jac::ast::NumericLiteral>(
                std::get<jac::ast::Literal>(
                  std::get<jac::ast::PrimaryExpression>(
                    std::get<jac::ast::MemberExpression>(
                      std::get<jac::ast::NewExpression>(
                        std::get<jac::ast::LeftHandSideExpressionPtr>(
                          std::get<jac::ast::UpdateExpression>(
                            std::get<jac::ast::UnaryExpressionPtr>(
                              std::get<jac::ast::BinaryExpression>(
                                std::get<jac::ast::ConditionalExpression>(pair.second->value)
                              .value)
                            .value)
                          ->value)
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
            jac::lex::Token(1, 1, "const", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "abc", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 15, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 17, "def", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 20, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LexicalDeclaration::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->isConst == true);
        REQUIRE(result->bindings.size() == 1);

        auto& binding = result->bindings[0];
        REQUIRE(std::holds_alternative<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(binding.value));
        auto& pair = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(binding.value);
        REQUIRE(pair.first.identifier.name == "x");
        auto& exp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(
            std::get<jac::ast::BinaryExpression>(
                std::get<jac::ast::ConditionalExpression>(pair.second->value)
            .value)
        .value);

        REQUIRE(std::get<2>(exp) == "+");
        REQUIRE(
            std::get<jac::ast::IdentifierReference>(
              std::get<jac::ast::PrimaryExpression>(
                std::get<jac::ast::MemberExpression>(
                  std::get<jac::ast::NewExpression>(
                    std::get<jac::ast::LeftHandSideExpressionPtr>(
                        std::get<jac::ast::UpdateExpression>(
                        std::get<jac::ast::UnaryExpressionPtr>(
                          std::get<0>(exp)
                        ->value)
                      ->value)
                    .value)
                  ->value)
                .value)
              .value)
            .value).identifier.name == "abc"
        );
        REQUIRE(
            std::get<jac::ast::IdentifierReference>(
              std::get<jac::ast::PrimaryExpression>(
                std::get<jac::ast::MemberExpression>(
                  std::get<jac::ast::NewExpression>(
                    std::get<jac::ast::LeftHandSideExpressionPtr>(
                      std::get<jac::ast::UpdateExpression>(
                        std::get<jac::ast::UnaryExpressionPtr>(
                          std::get<1>(exp)
                        ->value)
                      ->value)
                    .value)
                  ->value)
                .value)
              .value)
            .value).identifier.name == "def"
        );
    }
}


TEST_CASE("UnaryExpression", "[parser]") {

    SECTION("+ - 123") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "123", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::UnaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);


        auto& un1 = std::get<std::pair<jac::ast::UnaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(un1.second == "+");

        auto& inner = std::get<std::pair<jac::ast::UnaryExpressionPtr, std::string_view>>(un1.first->value);
        REQUIRE(inner.second == "-");

        REQUIRE(literalI32(unaryExpLiteral(inner.first)) == 123);
    }

    SECTION("- a++") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 4, "++", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::UnaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& un = std::get<std::pair<jac::ast::UnaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(un.second == "-");

        REQUIRE(std::get<jac::ast::UpdateExpression>(un.first->value).kind == jac::ast::UpdateKind::PostInc);

        REQUIRE(unaryExpIdentifier(un.first) == "a");
    }
}


TEST_CASE("BinaryExpression", "[parser]") {

    SECTION("1 + 2") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "2", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& exp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(exp) == "+");
        auto& lhs = std::get<0>(exp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(lhs->value)) == 1);

        auto& rhs = std::get<1>(exp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(rhs->value)) == 2);
    }

    SECTION("1 + 2 * 3") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "3", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& addExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(addExp) == "+");
        auto& addLhs = std::get<0>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(addLhs->value)) == 1);

        auto& addRhs = std::get<1>(addExp);
        auto& mulExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(addRhs->value);
        REQUIRE(std::get<2>(mulExp) == "*");
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(mulExp)->value)) == 2);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(mulExp)->value)) == 3);
    }

    SECTION("1 + 2 * 3 * 4 - 5") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 11, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "4", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 15, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 17, "5", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& minExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(minExp) == "-");
        auto& minLhs = std::get<0>(minExp);
        auto& minRhs = std::get<1>(minExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(minRhs->value)) == 5);

        auto& addExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(minLhs->value);
        REQUIRE(std::get<2>(addExp) == "+");
        auto& addLhs = std::get<0>(addExp);
        auto& addRhs = std::get<1>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(addLhs->value)) == 1);

        auto& mulExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(addRhs->value);
        REQUIRE(std::get<2>(mulExp) == "*");
        auto& mulLhs = std::get<0>(mulExp);
        auto& mulRhs = std::get<1>(mulExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(mulRhs->value)) == 4);

        auto& mulExp2 = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(mulLhs->value);
        REQUIRE(std::get<2>(mulExp2) == "*");
        auto& mulLhs2 = std::get<0>(mulExp2);
        auto& mulRhs2 = std::get<1>(mulExp2);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(mulLhs2->value)) == 2);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(mulRhs2->value)) == 3);
    }

    SECTION("1 + (2 * 3)") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 8, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 12, ")", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        auto& addExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(addExp) == "+");
        auto& addLhs = std::get<0>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(addLhs->value)) == 1);

        auto& addRhs = std::get<1>(addExp);
        auto& primary = unaryExpPrimary(std::get<jac::ast::UnaryExpressionPtr>(addRhs->value));
        auto& parExp = std::get<jac::ast::ParenthesizedExpression>(primary.value).expression;
        REQUIRE(parExp.items.size() == 1);
        auto& mulExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(
            std::get<jac::ast::BinaryExpression>(
            std::get<jac::ast::ConditionalExpression>(parExp.items[0]->value).value
        ).value);
        REQUIRE(std::get<2>(mulExp) == "*");
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(mulExp)->value)) == 2);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(mulExp)->value)) == 3);
    }

    SECTION("2 * 4 / 2 % 3") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "4", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, "/", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 11, "%", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "3", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& modExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(modExp) == "%");

        auto& modLhs = std::get<0>(modExp);
        auto& modRhs = std::get<1>(modExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(modRhs->value)) == 3);

        auto& divExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(modLhs->value);
        REQUIRE(std::get<2>(divExp) == "/");

        auto& divLhs = std::get<0>(divExp);
        auto& divRhs = std::get<1>(divExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(divRhs->value)) == 2);

        auto& mulExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(divLhs->value);
        REQUIRE(std::get<2>(mulExp) == "*");

        auto& mulLhs = std::get<0>(mulExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(mulLhs->value)) == 2);
        auto& mulRhs = std::get<1>(mulExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(mulRhs->value)) == 4);
    }

    SECTION("1 + + 2 - 3") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 9, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "3", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& subExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(result->value);
        REQUIRE(std::get<2>(subExp) == "-");

        auto& subLhs = std::get<0>(subExp);
        auto& subRhs = std::get<1>(subExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(subRhs->value)) == 3);

        auto& addExp = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(subLhs->value);
        REQUIRE(std::get<2>(addExp) == "+");

        auto& addLhs = std::get<0>(addExp);
        auto& addRhs = std::get<1>(addExp);
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(addLhs->value)) == 1);

        auto& unaryExp = std::get<jac::ast::UnaryExpressionPtr>(addRhs->value);
        REQUIRE(std::get<std::pair<jac::ast::UnaryExpressionPtr, std::string_view>>(unaryExp->value).second == "+");
        auto& inner = std::get<std::pair<jac::ast::UnaryExpressionPtr, std::string_view>>(unaryExp->value);
        REQUIRE(unaryExpI32(inner.first) == 2);
    }

    SECTION("1 +") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("1 * 2 +") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, "+", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(!result);
    }

    SECTION("1 + 2 (3)") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "2", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 10, ")", jac::lex::Token::Punctuator)
        };
        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BinaryExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(result);
    }
}


TEST_CASE("ConditionalExpression", "[parser]") {

    SECTION("a && b ? c : d + e") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "&&", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 8, "?", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "c", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 12, ":", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "d", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 16, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 18, "e", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::ConditionalExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& condTup = std::get<std::tuple<
            jac::ast::BinaryExpression,
            jac::ast::AssignmentExpressionPtr,
            jac::ast::AssignmentExpressionPtr
        >>(result->value);

        auto& andTup = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(std::get<0>(condTup).value);
        REQUIRE(std::get<2>(andTup) == "&&");

        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(andTup)->value)) == "a");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(andTup)->value)) == "b");

        auto& consequent = assignExpUnary(*std::get<1>(condTup));
        REQUIRE(unaryExpIdentifier(consequent) == "c");

        auto& alternate = assignExpBinary(*std::get<2>(condTup));
        auto& addTuple = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(alternate.value);
        REQUIRE(std::get<2>(addTuple) == "+");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(addTuple)->value)) == "d");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(addTuple)->value)) == "e");
    }
}


TEST_CASE("FunctionDeclaration", "[parser]") {

    SECTION("function f() {}") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "function", jac::lex::Token::Keyword),
            jac::lex::Token(1, 10, "f", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 11, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 12, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 15, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::FunctionDeclaration::parse(state, false);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name->identifier.name == "f");
        REQUIRE(result->parameters.parameterList.empty());
        REQUIRE_FALSE(result->parameters.restParameter);
        REQUIRE(!result->body);
    }

    SECTION("function fun(a, b) { const x = 3; let y = 4; return x + y; }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "function", jac::lex::Token::Keyword),
            jac::lex::Token(1, 10, "fun", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 13, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 15, ",", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 17, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 18, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 20, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 22, "const", jac::lex::Token::Keyword),
            jac::lex::Token(1, 28, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 30, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 32, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 33, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 35, "let", jac::lex::Token::Keyword),
            jac::lex::Token(1, 39, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 41, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 43, "4", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 44, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 46, "return", jac::lex::Token::Keyword),
            jac::lex::Token(1, 53, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 55, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 57, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 58, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 60, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::FunctionDeclaration::parse(state, false);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name->identifier.name == "fun");
        REQUIRE(result->parameters.parameterList.size() == 2);
        REQUIRE_FALSE(result->parameters.restParameter);
        REQUIRE(result->body);

        auto& argIdentA = std::get<jac::ast::BindingIdentifier>(result->parameters.parameterList[0].value);
        REQUIRE(argIdentA.identifier.name == "a");
        auto& argIdentB = std::get<jac::ast::BindingIdentifier>(result->parameters.parameterList[1].value);
        REQUIRE(argIdentB.identifier.name == "b");


        auto& statementList = result->body->statementList;
        REQUIRE(statementList.items.size() == 3);

        auto& declXStat = std::get<jac::ast::Declaration>(statementList.items[0].value);
        auto& declX = std::get<jac::ast::LexicalDeclaration>(declXStat.value);
        REQUIRE(declX.isConst == true);
        REQUIRE(declX.bindings.size() == 1);
        auto& bindingX = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(declX.bindings[0].value);
        REQUIRE(std::get<0>(bindingX).identifier.name == "x");

        auto& declYStat = std::get<jac::ast::Declaration>(statementList.items[1].value);
        auto& declY = std::get<jac::ast::LexicalDeclaration>(declYStat.value);
        REQUIRE(declY.isConst == false);
        REQUIRE(declY.bindings.size() == 1);
        auto& bindingY = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(declY.bindings[0].value);
        REQUIRE(std::get<0>(bindingY).identifier.name == "y");

        auto& retStat = std::get<jac::ast::Statement>(statementList.items[2].value);
        auto& ret = std::get<jac::ast::ReturnStatement>(retStat.value);
        REQUIRE(ret.expression);
        REQUIRE(ret.expression->items.size() == 1);

        auto& addExp = assignExpBinary(*ret.expression->items[0]);
        auto& addVar = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(addExp.value);
        REQUIRE(std::get<2>(addVar) == "+");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(addVar)->value)) == "x");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(addVar)->value)) == "y");
    }

    SECTION("function fun(a, b=3) {}") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "function", jac::lex::Token::Keyword),
            jac::lex::Token(1, 10, "fun", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 13, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 15, ",", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 17, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 18, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 19, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 20, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 22, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 23, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::FunctionDeclaration::parse(state, false);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
        REQUIRE(result->name->identifier.name == "fun");
        REQUIRE(result->parameters.parameterList.size() == 2);
        REQUIRE_FALSE(result->parameters.restParameter);
        REQUIRE(!result->body);

        auto& argIdentA = std::get<jac::ast::BindingIdentifier>(result->parameters.parameterList[0].value);
        REQUIRE(argIdentA.identifier.name == "a");

        auto& argB = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(result->parameters.parameterList[1].value);
        REQUIRE(argB.first.identifier.name == "b");
        REQUIRE(unaryExpI32(assignExpUnary(*argB.second)) == 3);
    }
}


TEST_CASE("Assignment", "[parser]") {

    SECTION("x = 4") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "4", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Assignment::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->op == "=");
        REQUIRE(lhsExpIdentifier(result->lhs) == "x");
        REQUIRE(unaryExpI32(assignExpUnary(*result->rhs)) == 4);
    }

    SECTION("x = x / 4") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 7, "/", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "4", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Assignment::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->op == "=");
        REQUIRE(lhsExpIdentifier(result->lhs) == "x");

        auto& divExp = assignExpBinary(*result->rhs);
        auto& divExpTup = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(divExp.value);
        REQUIRE(std::get<2>(divExpTup) == "/");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(divExpTup)->value)) == "x");
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(divExpTup)->value)) == 4);
    }

    SECTION("x += y -= 4") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "+=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 8, "-=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "4", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Assignment::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->op == "+=");
        REQUIRE(lhsExpIdentifier(result->lhs) == "x");

        auto& inner = std::get<jac::ast::Assignment>(result->rhs->value);
        REQUIRE(inner.op == "-=");
        REQUIRE(lhsExpIdentifier(inner.lhs) == "y");
        REQUIRE(unaryExpI32(assignExpUnary(*inner.rhs)) == 4);
    }
}


TEST_CASE("Expression", "[parser]") {

    SECTION("x < 10") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "<", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "10", jac::lex::Token::NumericLiteral)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Expression::parse(state);

        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->items.size() == 1);
    }

    SECTION("x++") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 2, "++", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Expression::parse(state);

        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->items.size() == 1);
    }

    SECTION("empty expr") {
        auto tokens = TokenVector{};

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Expression::parse(state);

        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("true literal expr") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "true", jac::lex::Token::Keyword)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Expression::parse(state);

        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->items.size() == 1);
    }
}


TEST_CASE("BlockStatement", "[parser]") {

    SECTION("{}") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 2, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BlockStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE_FALSE(result->statementList);
    }

    SECTION("{ let x = 3; x <<= x % 4; }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "let", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "3", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 12, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 16, "<<=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 19, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 21, "%", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 23, "4", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 24, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 26, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::BlockStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& list = result->statementList;
        REQUIRE(list->items.size() == 2);

        auto& decl = std::get<jac::ast::Declaration>(list->items[0].value);
        auto& declLex = std::get<jac::ast::LexicalDeclaration>(decl.value);
        REQUIRE(declLex.isConst == false);
        REQUIRE(declLex.bindings.size() == 1);
        auto& binding = std::get<std::pair<jac::ast::BindingIdentifier, jac::ast::InitializerPtr>>(declLex.bindings[0].value);
        REQUIRE(binding.first.identifier.name == "x");

        auto& assignStat = std::get<jac::ast::Statement>(list->items[1].value);
        auto& assignExpStat = std::get<jac::ast::ExpressionStatement>(assignStat.value).expression;
        REQUIRE(assignExpStat.items.size() == 1);
        auto& assignment = std::get<jac::ast::Assignment>(assignExpStat.items[0]->value);

        REQUIRE(assignment.op == "<<=");
        REQUIRE(lhsExpIdentifier(assignment.lhs) == "x");
        auto& modExp = assignExpBinary(*assignment.rhs);

        auto& modExpTup = std::get<std::tuple<jac::ast::BinaryExpressionPtr, jac::ast::BinaryExpressionPtr, std::string_view>>(modExp.value);
        REQUIRE(std::get<2>(modExpTup) == "%");
        REQUIRE(unaryExpIdentifier(std::get<jac::ast::UnaryExpressionPtr>(std::get<0>(modExpTup)->value)) == "x");
        REQUIRE(unaryExpI32(std::get<jac::ast::UnaryExpressionPtr>(std::get<1>(modExpTup)->value)) == 4);
    }
}


TEST_CASE("Script", "[parser]") {
    SECTION("x;") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 2, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Script::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& list = result->statementList;
        REQUIRE(list->items.size() == 1);

        jac::ast::StatementListItem& expStat = list->items[0];
        auto& statement = std::get<jac::ast::Statement>(expStat.value);
        auto& expr = std::get<jac::ast::ExpressionStatement>(statement.value).expression;
        REQUIRE(expr.items.size() == 1);
        REQUIRE(std::string(unaryExpIdentifier(assignExpUnary(*expr.items[0]))) == "x");
    }
}


TEST_CASE("Call expression", "[parser]") {

    SECTION("f()") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "f", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 2, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, ")", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::CallExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& [mem, args] = std::get<std::pair<jac::ast::MemberExpression, jac::ast::Arguments>>(result->value);
        auto& primary = std::get<jac::ast::PrimaryExpression>(mem.value);
        auto& ident = std::get<jac::ast::IdentifierReference>(primary.value);
        REQUIRE(ident.identifier.name == "f");

        REQUIRE(args.arguments.empty());
    }

    SECTION("func(1, 'abc', variable)") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "func", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "1", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 11, ",", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "'abc'", jac::lex::Token::StringLiteral),
            jac::lex::Token(1, 18, ",", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 20, "variable", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 28, ")", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::CallExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& [mem, args] = std::get<std::pair<jac::ast::MemberExpression, jac::ast::Arguments>>(result->value);
        auto& primary = std::get<jac::ast::PrimaryExpression>(mem.value);
        auto& ident = std::get<jac::ast::IdentifierReference>(primary.value);
        REQUIRE(ident.identifier.name == "func");

        REQUIRE(args.arguments.size() == 3);
    }

    SECTION("call(true)") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "call", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 5, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "true", jac::lex::Token::Keyword),
            jac::lex::Token(1, 10, ")", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::CallExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& [mem, args] = std::get<std::pair<jac::ast::MemberExpression, jac::ast::Arguments>>(result->value);
        auto& primary = std::get<jac::ast::PrimaryExpression>(mem.value);
        auto& ident = std::get<jac::ast::IdentifierReference>(primary.value);
        REQUIRE(ident.identifier.name == "call");

        REQUIRE(args.arguments.size() == 1);
    }

    SECTION("call(fun(true))") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "call", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 5, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "fun", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "true", jac::lex::Token::Keyword),
            jac::lex::Token(1, 14, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 16, ")", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::CallExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        auto& [mem, args] = std::get<std::pair<jac::ast::MemberExpression, jac::ast::Arguments>>(result->value);
        auto& primary = std::get<jac::ast::PrimaryExpression>(mem.value);
        auto& ident = std::get<jac::ast::IdentifierReference>(primary.value);
        REQUIRE(ident.identifier.name == "call");

        REQUIRE(args.arguments.size() == 1);
    }
}


TEST_CASE("If statement", "[parser]") {

    SECTION("if (x) {}") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "if", jac::lex::Token::Keyword),
            jac::lex::Token(1, 4, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 6, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IfStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE_FALSE(result->alternate);
    }

    SECTION("if (x) { y; } else { z; }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "if", jac::lex::Token::Keyword),
            jac::lex::Token(1, 4, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 6, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 11, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "}", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 15, "else", jac::lex::Token::Keyword),
            jac::lex::Token(1, 20, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 22, "z", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 23, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 25, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IfStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->alternate);
    }

    SECTION("if (a == 4) b; else c;") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "if", jac::lex::Token::Keyword),
            jac::lex::Token(1, 4, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 7, "==", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "4", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 11, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 14, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 16, "else", jac::lex::Token::Keyword),
            jac::lex::Token(1, 21, "c", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 22, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::IfStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->alternate);
    }
}


TEST_CASE("Loop statement", "[parser]") {

    SECTION("while (x) {}") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "while", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 12, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::WhileStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->statement);
    }

    SECTION("while (x) { y; }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "while", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 14, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 16, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::WhileStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->statement);
    }

    SECTION("while (true);") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "while", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "true", jac::lex::Token::Keyword),
            jac::lex::Token(1, 12, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::WhileStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->statement);
    }

    SECTION("do { x; } while (y > 0);") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "do", jac::lex::Token::Keyword),
            jac::lex::Token(1, 4, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 7, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "}", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 11, "while", jac::lex::Token::Keyword),
            jac::lex::Token(1, 17, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 18, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 20, ">", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 22, "0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 23, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 24, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::DoWhileStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->statement);
    }

    SECTION("for (let x = 0; x < 10; x++) { y; }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "for", jac::lex::Token::Keyword),
            jac::lex::Token(1, 5, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "let", jac::lex::Token::Keyword),
            jac::lex::Token(1, 10, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 12, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 15, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 17, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 19, "<", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 21, "10", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 23, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 25, "x", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 26, "++", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 28, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 30, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 32, "y", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 33, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 35, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::ForStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(std::holds_alternative<jac::ast::LexicalDeclaration>(result->init));
        REQUIRE(result->condition);
        REQUIRE(result->update);
        REQUIRE(result->statement);
    }

    SECTION("for (;;) { }") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "for", jac::lex::Token::Keyword),
            jac::lex::Token(1, 5, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 12, "}", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::ForStatement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }
}


TEST_CASE("Statement", "[parser]") {

    SECTION("report(fun(true));") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "report", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 8, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "fun", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 13, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 14, "true", jac::lex::Token::Keyword),
            jac::lex::Token(1, 18, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 20, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 21, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::Statement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(std::holds_alternative<jac::ast::ExpressionStatement>(result->value));
    }
}


TEST_CASE("Member access", "[parser]") {

    SECTION("a.b") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 2, ".", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "b", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::LeftHandSideExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("return a.b;") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "return", jac::lex::Token::Keyword),
            jac::lex::Token(1, 8, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, ".", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 11, ";", jac::lex::Token::Punctuator)
        };

        jac::ast::ParserState state(tokens);

        auto _ = state.pushTemplate<jac::ast::Return(true)>();
        auto result = jac::ast::Statement::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }

    SECTION("a.b.c[\"test\"]") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 1, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 2, ".", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 4, ".", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "c", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::MemberExpression::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);
    }
}

TEST_CASE("Type annotation", "[parser]") {

    SECTION(": void") {
        auto tokens = TokenVector{
            jac::lex::Token(1, 2, ":", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 4, "i32", jac::lex::Token::IdentifierName)
        };

        jac::ast::ParserState state(tokens);

        auto result = jac::ast::TypeAnnotation::parse(state);
        CAPTURE(state.getErrorMessage());
        CAPTURE(state.getErrorToken());
        REQUIRE(state.isEnd());
        REQUIRE(result);

        REQUIRE(result->type == "i32");
    }
}
