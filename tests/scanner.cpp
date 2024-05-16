#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/compiler/scanner.h>


template<>
struct Catch::StringMaker<jac::lex::Token> {
    static std::string convert(jac::lex::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.kind) + ")";
    }
};


void report(int line, int column, std::string message) {
    CAPTURE(line);
    CAPTURE(column);
    CAPTURE(message);
    FAIL("Scanner error");
}


using TokenVector = std::vector<jac::lex::Token>;


TEST_CASE("Punctuator", "[scanner]") {
    SECTION("Empty") {
        std::string input(""); // NOLINT
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan().empty());
    }

    SECTION("{[(<>)]}") {
        std::string input("{[(<>)]}");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 2, "[", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 4, "<", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, ">", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "]", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 8, "}", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("> >> >>> >= >>= >>>=") {
        std::string input("> >> >>> >>= >>>=");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, ">", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, ">>", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, ">>>", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, ">>=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, ">>>=", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("< << <= <<=") {
        std::string input("< << <= <<=");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "<", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "<<", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "<=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "<<=", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("=>") {
        std::string input("=>");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::Punctuator)}));
    }

    SECTION("...") {
        std::string input("...");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::Punctuator)}));
    }

    SECTION("++ --") {
        std::string input("++ --");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "++", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 4, "--", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("+ - * % **") {
        std::string input("+ - * % **");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "*", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "%", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "**", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("+= -= *= %= **=") {
        std::string input("+= -= *= %= **=");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "+=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 4, "-=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "*=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "%=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 13, "**=", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("= == === ! != !==") {
        std::string input("= == === ! != !==");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "==", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "===", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 10, "!", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 12, "!=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 15, "!==", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("? ?? \?\?= ?.") {
        std::string input("? ?? \?\?= ?.");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "?", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "??", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 6, "\?\?=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "?.", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("& | ^ ~ && || &= |= ^= &&= ||=") {
        std::string input("& | ^ ~ && || &= |= ^= &&= ||=");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "&", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "|", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 5, "^", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "~", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 9, "&&", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 12, "||", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 15, "&=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 18, "|=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 21, "^=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 24, "&&=", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 28, "||=", jac::lex::Token::Punctuator)
        }));
    }

    SECTION("/ /=") {
        std::string input("/ /=");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "/", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 3, "/=", jac::lex::Token::Punctuator)
        }));
    }
}


TEST_CASE("StringLiteral", "[scanner]") {
    SECTION("''") {
        std::string input("''");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::StringLiteral)}));
    }

    SECTION("''''") {
        std::string input("''''");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "''", jac::lex::Token::StringLiteral),
            jac::lex::Token(1, 3, "''", jac::lex::Token::StringLiteral)
        }));
    }

    SECTION("'test\\\'test'") {
        std::string input("'test\\\'test'");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::StringLiteral)}));
    }

    SECTION("\"''\"") {
        std::string input("\"''\"");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::StringLiteral)}));
    }

    SECTION("\"test\\\"test\"") {
        std::string input("\"test\\\"test\"");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::StringLiteral)}));
    }

    SECTION("`${'test'}`") {
        std::string input("`${'test'}`");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::StringLiteral)}));
    }
}


TEST_CASE("Comments", "[scanner]") {
    SECTION("Single line") {
        std::string input("// test");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::lex::Token(1, 1, input, jac::lex::Token::Comment)
        }));
    }

    SECTION("Single line with newline") {
        std::string input("// comment\n 'string'");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::lex::Token(1, 1, "// comment", jac::lex::Token::Comment),
            jac::lex::Token(2, 2, "'string'", jac::lex::Token::StringLiteral)
        }));
    }

    SECTION("Multi line") {
        std::string input("/* test */");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::lex::Token(1, 1, input, jac::lex::Token::Comment)
        }));
    }

    SECTION("Multi line with newline") {
        std::string input("/* comment\nline2 */ 'string'");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            // jac::lex::Token(1, 1, "/* comment\nline2 */", jac::lex::Token::Comment),
            jac::lex::Token(2, 2, "'string'", jac::lex::Token::StringLiteral)
        }));
    }
}


TEST_CASE("NumericLiteral", "[scanner]") {
    SECTION("0 0.0 .0 0e0 0.0e0 0.e0 .0e0 0n") {
        std::string input("0 0.0 .0 0e0 0.0e0 0.e0 .0e0 0n");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 3, "0.0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 7, ".0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 10, "0e0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 14, "0.0e0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 19, "0.e0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 23, ".0e0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 27, "0n", jac::lex::Token::NumericLiteral)
        }));
    }

    SECTION("1_2_3_4_5_6_7_8_9_0 1_2_3_4_5_6_7_8_9_0n 1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0") {
        std::string input("1_2_3_4_5_6_7_8_9_0 1_2_3_4_5_6_7_8_9_0n 1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "1_2_3_4_5_6_7_8_9_0", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 21, "1_2_3_4_5_6_7_8_9_0n", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 43, "1_2_3_4_5_6_7_8_9_0e1_2_3_4_5_6_7_8_9_0", jac::lex::Token::NumericLiteral)
        }));
    }

    SECTION("01234567") {
        std::string input("01234567");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({jac::lex::Token(1, 1, input, jac::lex::Token::NumericLiteral)}));
    }

    SECTION("0x0123456789abcdefABCDEF 0x0123456789_abcdef_ABCDEFn") {
        std::string input("0x0123456789abcdefABCDEF 0x0123456789_abcdef_ABCDEFn");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "0x0123456789abcdefABCDEF", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 26, "0x0123456789_abcdef_ABCDEFn", jac::lex::Token::NumericLiteral)
        }));
    }

    SECTION("0b01 0b0_1n") {
        std::string input("0b01 0b0_1n");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "0b01", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 5, "0b0_1n", jac::lex::Token::NumericLiteral)
        }));
    }

    SECTION("0o01234567 0o0_1_2_3_4_5_6_7n") {
        std::string input("0o01234567 0o0_1_2_3_4_5_6_7n");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "0o01234567", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 11, "0o0_1_2_3_4_5_6_7n", jac::lex::Token::NumericLiteral)
        }));
    }

    SECTION("-123 +123") {
        std::string input("-123 +123");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "-", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 2, "123", jac::lex::Token::NumericLiteral),
            jac::lex::Token(1, 6, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 7, "123", jac::lex::Token::NumericLiteral)
        }));
    }
}


TEST_CASE("IdentifierName", "[scanner]") {
    SECTION("a _a a_ a1 _1") {
        std::string input("a _a a_ a1 _1");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "_a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 6, "a_", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 9, "a1", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 12, "_1", jac::lex::Token::IdentifierName)
        }));
    }

    SECTION("$ _ $a $1 $a1 $1a a$") {
        std::string input("$ _ $a $1 $a1 $1a a$");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "$", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, "_", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 5, "$a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 8, "$1", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 11, "$a1", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 15, "$1a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 18, "a$", jac::lex::Token::IdentifierName)
        }));
    }

    SECTION("#priv #_priv #priv_ #priv1 #$") {
        std::string input("#priv #_priv #priv_ #priv1 #$");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "#priv", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 7, "#_priv", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 14, "#priv_", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 21, "#priv1", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 28, "#$", jac::lex::Token::IdentifierName)
        }));
    }
}

TEST_CASE("Keywords", "[scanner]") {
    SECTION("break case catch class const continue debugger default delete do else enum export extends false finally for function if import in instanceof new null return super switch this throw true try typeof var void while with yield") {
        std::string input("break case catch class const continue debugger default delete do else enum export extends false finally for function if import in instanceof new null return super switch this throw true try typeof var void while with yield");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "break", jac::lex::Token::Keyword),
            jac::lex::Token(1, 7, "case", jac::lex::Token::Keyword),
            jac::lex::Token(1, 12, "catch", jac::lex::Token::Keyword),
            jac::lex::Token(1, 18, "class", jac::lex::Token::Keyword),
            jac::lex::Token(1, 24, "const", jac::lex::Token::Keyword),
            jac::lex::Token(1, 30, "continue", jac::lex::Token::Keyword),
            jac::lex::Token(1, 39, "debugger", jac::lex::Token::Keyword),
            jac::lex::Token(1, 48, "default", jac::lex::Token::Keyword),
            jac::lex::Token(1, 56, "delete", jac::lex::Token::Keyword),
            jac::lex::Token(1, 63, "do", jac::lex::Token::Keyword),
            jac::lex::Token(1, 66, "else", jac::lex::Token::Keyword),
            jac::lex::Token(1, 71, "enum", jac::lex::Token::Keyword),
            jac::lex::Token(1, 76, "export", jac::lex::Token::Keyword),
            jac::lex::Token(1, 83, "extends", jac::lex::Token::Keyword),
            jac::lex::Token(1, 91, "false", jac::lex::Token::Keyword),
            jac::lex::Token(1, 97, "finally", jac::lex::Token::Keyword),
            jac::lex::Token(1, 105, "for", jac::lex::Token::Keyword),
            jac::lex::Token(1, 109, "function", jac::lex::Token::Keyword),
            jac::lex::Token(1, 118, "if", jac::lex::Token::Keyword),
            jac::lex::Token(1, 121, "import", jac::lex::Token::Keyword),
            jac::lex::Token(1, 128, "in", jac::lex::Token::Keyword),
            jac::lex::Token(1, 131, "instanceof", jac::lex::Token::Keyword),
            jac::lex::Token(1, 142, "new", jac::lex::Token::Keyword),
            jac::lex::Token(1, 146, "null", jac::lex::Token::Keyword),
            jac::lex::Token(1, 151, "return", jac::lex::Token::Keyword),
            jac::lex::Token(1, 158, "super", jac::lex::Token::Keyword),
            jac::lex::Token(1, 164, "switch", jac::lex::Token::Keyword),
            jac::lex::Token(1, 171, "this", jac::lex::Token::Keyword),
            jac::lex::Token(1, 176, "throw", jac::lex::Token::Keyword),
            jac::lex::Token(1, 182, "true", jac::lex::Token::Keyword),
            jac::lex::Token(1, 187, "try", jac::lex::Token::Keyword),
            jac::lex::Token(1, 191, "typeof", jac::lex::Token::Keyword),
            jac::lex::Token(1, 198, "var", jac::lex::Token::Keyword),
            jac::lex::Token(1, 202, "void", jac::lex::Token::Keyword),
            jac::lex::Token(1, 207, "while", jac::lex::Token::Keyword),
            jac::lex::Token(1, 213, "with", jac::lex::Token::Keyword),
            jac::lex::Token(1, 218, "yield", jac::lex::Token::Keyword)
        }));
    }
}

TEST_CASE("Mixed", "[scanner]") {
    SECTION("Function") {
        std::string input(
R"(function test(a, b) {
    return a + b;
})");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "function", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 10, "test", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 14, "(", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 15, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 16, ",", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 18, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 19, ")", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 21, "{", jac::lex::Token::Punctuator),
            jac::lex::Token(2, 5, "return", jac::lex::Token::IdentifierName),
            jac::lex::Token(2, 12, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(2, 14, "+", jac::lex::Token::Punctuator),
            jac::lex::Token(2, 16, "b", jac::lex::Token::IdentifierName),
            jac::lex::Token(2, 17, ";", jac::lex::Token::Punctuator),
            jac::lex::Token(3, 1, "}", jac::lex::Token::Punctuator)
        }));
    }
}

TEST_CASE("Member access", "[scanner]") {
    SECTION("a.b") {
        std::string input("a.b");
        jac::lex::Scanner scanner(input, report);
        REQUIRE(scanner.scan() == TokenVector({
            jac::lex::Token(1, 1, "a", jac::lex::Token::IdentifierName),
            jac::lex::Token(1, 3, ".", jac::lex::Token::Punctuator),
            jac::lex::Token(1, 4, "b", jac::lex::Token::IdentifierName)
        }));
    }
}
