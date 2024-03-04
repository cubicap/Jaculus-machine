#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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
        REQUIRE(std::holds_alternative<int32_t>(result->value));
        REQUIRE(std::get<int32_t>(result->value) == 0x0123);
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
        REQUIRE(std::holds_alternative<int32_t>(result->value));
        REQUIRE(std::get<int32_t>(result->value) == 0123);
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
        REQUIRE(std::holds_alternative<int32_t>(result->value));
        REQUIRE(std::get<int32_t>(result->value) == 0b1010);
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


TEST_CASE("Literal") {
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
        REQUIRE(std::holds_alternative<jac::BindingIdentifier<true, true>>(binding.value));
        REQUIRE(std::get<jac::BindingIdentifier<true, true>>(binding.value).identifier.name.name == "x");
    }
}
