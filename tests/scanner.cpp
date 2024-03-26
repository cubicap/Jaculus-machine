#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/compiler/scanner.h>


template<>
struct Catch::StringMaker<jac::Token> {
    static std::string convert(jac::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.kind) + ")";
    }
};


void report(int line, int column, std::string message) {
    CAPTURE(line);
    CAPTURE(column);
    CAPTURE(message);
    FAIL("Scanner error");
}


using TokenVector = std::vector<jac::Token>;


TEST_CASE("Punctuator", "[scanner]") {
    SECTION("Empty") {
        std::string input(""); // NOLINT
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan().empty());
    }

    SECTION("{[(<>)]}") {
        std::string input("{[(<>)]}");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "{", jac::Token::Punctuator),
            jac::Token(1, 2, "[", jac::Token::Punctuator),
            jac::Token(1, 3, "(", jac::Token::Punctuator),
            jac::Token(1, 4, "<", jac::Token::Punctuator),
            jac::Token(1, 5, ">", jac::Token::Punctuator),
            jac::Token(1, 6, ")", jac::Token::Punctuator),
            jac::Token(1, 7, "]", jac::Token::Punctuator),
            jac::Token(1, 8, "}", jac::Token::Punctuator)
        }));
    }

    SECTION("> >> >>> >= >>= >>>=") {
        std::string input("> >> >>> >>= >>>=");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, ">", jac::Token::Punctuator),
            jac::Token(1, 3, ">>", jac::Token::Punctuator),
            jac::Token(1, 6, ">>>", jac::Token::Punctuator),
            jac::Token(1, 10, ">>=", jac::Token::Punctuator),
            jac::Token(1, 13, ">>>=", jac::Token::Punctuator)
        }));
    }

    SECTION("< << <= <<=") {
        std::string input("< << <= <<=");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "<", jac::Token::Punctuator),
            jac::Token(1, 3, "<<", jac::Token::Punctuator),
            jac::Token(1, 6, "<=", jac::Token::Punctuator),
            jac::Token(1, 9, "<<=", jac::Token::Punctuator)
        }));
    }

    SECTION("=>") {
        std::string input("=>");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::Punctuator)}));
    }

    SECTION("...") {
        std::string input("...");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::Punctuator)}));
    }

    SECTION("++ --") {
        std::string input("++ --");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "++", jac::Token::Punctuator),
            jac::Token(1, 4, "--", jac::Token::Punctuator)
        }));
    }

    SECTION("+ - * % **") {
        std::string input("+ - * % **");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "+", jac::Token::Punctuator),
            jac::Token(1, 3, "-", jac::Token::Punctuator),
            jac::Token(1, 5, "*", jac::Token::Punctuator),
            jac::Token(1, 7, "%", jac::Token::Punctuator),
            jac::Token(1, 9, "**", jac::Token::Punctuator)
        }));
    }

    SECTION("+= -= *= %= **=") {
        std::string input("+= -= *= %= **=");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "+=", jac::Token::Punctuator),
            jac::Token(1, 4, "-=", jac::Token::Punctuator),
            jac::Token(1, 7, "*=", jac::Token::Punctuator),
            jac::Token(1, 10, "%=", jac::Token::Punctuator),
            jac::Token(1, 13, "**=", jac::Token::Punctuator)
        }));
    }

    SECTION("= == === ! != !==") {
        std::string input("= == === ! != !==");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "=", jac::Token::Punctuator),
            jac::Token(1, 3, "==", jac::Token::Punctuator),
            jac::Token(1, 6, "===", jac::Token::Punctuator),
            jac::Token(1, 10, "!", jac::Token::Punctuator),
            jac::Token(1, 12, "!=", jac::Token::Punctuator),
            jac::Token(1, 15, "!==", jac::Token::Punctuator)
        }));
    }

    SECTION("? ?? \?\?= ?.") {
        std::string input("? ?? \?\?= ?.");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "?", jac::Token::Punctuator),
            jac::Token(1, 3, "??", jac::Token::Punctuator),
            jac::Token(1, 6, "\?\?=", jac::Token::Punctuator),
            jac::Token(1, 9, "?.", jac::Token::Punctuator)
        }));
    }

    SECTION("& | ^ ~ && || &= |= ^= &&= ||=") {
        std::string input("& | ^ ~ && || &= |= ^= &&= ||=");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "&", jac::Token::Punctuator),
            jac::Token(1, 3, "|", jac::Token::Punctuator),
            jac::Token(1, 5, "^", jac::Token::Punctuator),
            jac::Token(1, 7, "~", jac::Token::Punctuator),
            jac::Token(1, 9, "&&", jac::Token::Punctuator),
            jac::Token(1, 12, "||", jac::Token::Punctuator),
            jac::Token(1, 15, "&=", jac::Token::Punctuator),
            jac::Token(1, 18, "|=", jac::Token::Punctuator),
            jac::Token(1, 21, "^=", jac::Token::Punctuator),
            jac::Token(1, 24, "&&=", jac::Token::Punctuator),
            jac::Token(1, 28, "||=", jac::Token::Punctuator)
        }));
    }

    SECTION("/ /=") {
        std::string input("/ /=");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "/", jac::Token::Punctuator),
            jac::Token(1, 3, "/=", jac::Token::Punctuator)
        }));
    }
}


TEST_CASE("StringLiteral", "[scanner]") {
    SECTION("''") {
        std::string input("''");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::StringLiteral)}));
    }

    SECTION("''''") {
        std::string input("''''");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "''", jac::Token::StringLiteral),
            jac::Token(1, 3, "''", jac::Token::StringLiteral)
        }));
    }

    SECTION("'test\\\'test'") {
        std::string input("'test\\\'test'");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::StringLiteral)}));
    }

    SECTION("\"''\"") {
        std::string input("\"''\"");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::StringLiteral)}));
    }

    SECTION("\"test\\\"test\"") {
        std::string input("\"test\\\"test\"");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::StringLiteral)}));
    }

    SECTION("`${'test'}`") {
        std::string input("`${'test'}`");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::StringLiteral)}));
    }
}


TEST_CASE("Comments", "[scanner]") {
    SECTION("Single line") {
        std::string input("// test");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::Token(1, 1, input, jac::Token::Comment)
        }));
    }

    SECTION("Single line with newline") {
        std::string input("// comment\n 'string'");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::Token(1, 1, "// comment", jac::Token::Comment),
            jac::Token(2, 2, "'string'", jac::Token::StringLiteral)
        }));
    }

    SECTION("Multi line") {
        std::string input("/* test */");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::Token(1, 1, input, jac::Token::Comment)
        }));
    }

    SECTION("Multi line with newline") {
        std::string input("/* comment\nline2 */ 'string'");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::Token(1, 1, "/* comment\nline2 */", jac::Token::Comment),
            jac::Token(2, 2, "'string'", jac::Token::StringLiteral)
        }));
    }
}


TEST_CASE("NumericLiteral", "[scanner]") {
    SECTION("0 0.0 .0 0e0 0.0e0 0.e0 .0e0 0n") {
        std::string input("0 0.0 .0 0e0 0.0e0 0.e0 .0e0 0n");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "0", jac::Token::NumericLiteral),
            jac::Token(1, 3, "0.0", jac::Token::NumericLiteral),
            jac::Token(1, 7, ".0", jac::Token::NumericLiteral),
            jac::Token(1, 10, "0e0", jac::Token::NumericLiteral),
            jac::Token(1, 14, "0.0e0", jac::Token::NumericLiteral),
            jac::Token(1, 19, "0.e0", jac::Token::NumericLiteral),
            jac::Token(1, 23, ".0e0", jac::Token::NumericLiteral),
            jac::Token(1, 27, "0n", jac::Token::NumericLiteral)
        }));
    }

    SECTION("1_2_3_4_5_6_7_8_9_0 1_2_3_4_5_6_7_8_9_0n 1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0") {
        std::string input("1_2_3_4_5_6_7_8_9_0 1_2_3_4_5_6_7_8_9_0n 1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "1_2_3_4_5_6_7_8_9_0", jac::Token::NumericLiteral),
            jac::Token(1, 21, "1_2_3_4_5_6_7_8_9_0n", jac::Token::NumericLiteral),
            jac::Token(1, 43, "1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0", jac::Token::NumericLiteral)
        }));
    }

    SECTION("01234567") {
        std::string input("01234567");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::Token(1, 1, input, jac::Token::NumericLiteral)}));
    }

    SECTION("0x0123456789abcdefABCDEF 0x0123456789_abcdef_ABCDEFn") {
        std::string input("0x0123456789abcdefABCDEF 0x0123456789_abcdef_ABCDEFn");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "0x0123456789abcdefABCDEF", jac::Token::NumericLiteral),
            jac::Token(1, 26, "0x0123456789_abcdef_ABCDEFn", jac::Token::NumericLiteral)
        }));
    }

    SECTION("0b01 0b0_1n") {
        std::string input("0b01 0b0_1n");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "0b01", jac::Token::NumericLiteral),
            jac::Token(1, 5, "0b0_1n", jac::Token::NumericLiteral)
        }));
    }

    SECTION("0o01234567 0o0_1_2_3_4_5_6_7n") {
        std::string input("0o01234567 0o0_1_2_3_4_5_6_7n");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "0o01234567", jac::Token::NumericLiteral),
            jac::Token(1, 11, "0o0_1_2_3_4_5_6_7n", jac::Token::NumericLiteral)
        }));
    }

    SECTION("-123 +123") {
        std::string input("-123 +123");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "-", jac::Token::Punctuator),
            jac::Token(1, 2, "123", jac::Token::NumericLiteral),
            jac::Token(1, 6, "+", jac::Token::Punctuator),
            jac::Token(1, 7, "123", jac::Token::NumericLiteral)
        }));
    }
}


TEST_CASE("IdentifierName", "[scanner]") {
    SECTION("a _a a_ a1 _1") {
        std::string input("a _a a_ a1 _1");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "a", jac::Token::IdentifierName),
            jac::Token(1, 3, "_a", jac::Token::IdentifierName),
            jac::Token(1, 6, "a_", jac::Token::IdentifierName),
            jac::Token(1, 9, "a1", jac::Token::IdentifierName),
            jac::Token(1, 12, "_1", jac::Token::IdentifierName)
        }));
    }

    SECTION("$ _ $a $1 $a1 $1a a$") {
        std::string input("$ _ $a $1 $a1 $1a a$");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "$", jac::Token::IdentifierName),
            jac::Token(1, 3, "_", jac::Token::IdentifierName),
            jac::Token(1, 5, "$a", jac::Token::IdentifierName),
            jac::Token(1, 8, "$1", jac::Token::IdentifierName),
            jac::Token(1, 11, "$a1", jac::Token::IdentifierName),
            jac::Token(1, 15, "$1a", jac::Token::IdentifierName),
            jac::Token(1, 18, "a$", jac::Token::IdentifierName)
        }));
    }

    SECTION("#priv #_priv #priv_ #priv1 #$") {
        std::string input("#priv #_priv #priv_ #priv1 #$");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "#priv", jac::Token::IdentifierName),
            jac::Token(1, 7, "#_priv", jac::Token::IdentifierName),
            jac::Token(1, 14, "#priv_", jac::Token::IdentifierName),
            jac::Token(1, 21, "#priv1", jac::Token::IdentifierName),
            jac::Token(1, 28, "#$", jac::Token::IdentifierName)
        }));
    }
}

TEST_CASE("Keywords", "[scanner]") {
    SECTION("break case catch class const continue debugger default delete do else enum export extends false finally for function if import in instanceof new null return super switch this throw true try typeof var void while with yield") {
        std::string input("break case catch class const continue debugger default delete do else enum export extends false finally for function if import in instanceof new null return super switch this throw true try typeof var void while with yield");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "break", jac::Token::Keyword),
            jac::Token(1, 7, "case", jac::Token::Keyword),
            jac::Token(1, 12, "catch", jac::Token::Keyword),
            jac::Token(1, 18, "class", jac::Token::Keyword),
            jac::Token(1, 24, "const", jac::Token::Keyword),
            jac::Token(1, 30, "continue", jac::Token::Keyword),
            jac::Token(1, 39, "debugger", jac::Token::Keyword),
            jac::Token(1, 48, "default", jac::Token::Keyword),
            jac::Token(1, 56, "delete", jac::Token::Keyword),
            jac::Token(1, 63, "do", jac::Token::Keyword),
            jac::Token(1, 66, "else", jac::Token::Keyword),
            jac::Token(1, 71, "enum", jac::Token::Keyword),
            jac::Token(1, 76, "export", jac::Token::Keyword),
            jac::Token(1, 83, "extends", jac::Token::Keyword),
            jac::Token(1, 91, "false", jac::Token::Keyword),
            jac::Token(1, 97, "finally", jac::Token::Keyword),
            jac::Token(1, 105, "for", jac::Token::Keyword),
            jac::Token(1, 109, "function", jac::Token::Keyword),
            jac::Token(1, 118, "if", jac::Token::Keyword),
            jac::Token(1, 121, "import", jac::Token::Keyword),
            jac::Token(1, 128, "in", jac::Token::Keyword),
            jac::Token(1, 131, "instanceof", jac::Token::Keyword),
            jac::Token(1, 142, "new", jac::Token::Keyword),
            jac::Token(1, 146, "null", jac::Token::Keyword),
            jac::Token(1, 151, "return", jac::Token::Keyword),
            jac::Token(1, 158, "super", jac::Token::Keyword),
            jac::Token(1, 164, "switch", jac::Token::Keyword),
            jac::Token(1, 171, "this", jac::Token::Keyword),
            jac::Token(1, 176, "throw", jac::Token::Keyword),
            jac::Token(1, 182, "true", jac::Token::Keyword),
            jac::Token(1, 187, "try", jac::Token::Keyword),
            jac::Token(1, 191, "typeof", jac::Token::Keyword),
            jac::Token(1, 198, "var", jac::Token::Keyword),
            jac::Token(1, 202, "void", jac::Token::Keyword),
            jac::Token(1, 207, "while", jac::Token::Keyword),
            jac::Token(1, 213, "with", jac::Token::Keyword),
            jac::Token(1, 218, "yield", jac::Token::Keyword)
        }));
    }
}

TEST_CASE("Mixed", "[scanner]") {
    SECTION("Function") {
        std::string input(
R"(function test(a, b) {
    return a + b;
})");
        jac::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::Token(1, 1, "function", jac::Token::IdentifierName),
            jac::Token(1, 10, "test", jac::Token::IdentifierName),
            jac::Token(1, 14, "(", jac::Token::Punctuator),
            jac::Token(1, 15, "a", jac::Token::IdentifierName),
            jac::Token(1, 16, ",", jac::Token::Punctuator),
            jac::Token(1, 18, "b", jac::Token::IdentifierName),
            jac::Token(1, 19, ")", jac::Token::Punctuator),
            jac::Token(1, 21, "{", jac::Token::Punctuator),
            jac::Token(2, 5, "return", jac::Token::IdentifierName),
            jac::Token(2, 12, "a", jac::Token::IdentifierName),
            jac::Token(2, 14, "+", jac::Token::Punctuator),
            jac::Token(2, 16, "b", jac::Token::IdentifierName),
            jac::Token(2, 17, ";", jac::Token::Punctuator),
            jac::Token(3, 1, "}", jac::Token::Punctuator)
        }));
    }
}
