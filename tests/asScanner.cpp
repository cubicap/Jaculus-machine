#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/compiler/assembler.h>


template<>
struct Catch::StringMaker<jac::as::Token> {
    static std::string convert(jac::as::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.kind) + ")";
    }
};


void report(int line, int column, std::string message) {
    CAPTURE(line);
    CAPTURE(column);
    CAPTURE(message);
    FAIL("Scanner error");
}


using TokenVector = std::vector<jac::as::Token>;


TEST_CASE("Section", "[assembler]") {
    SECTION("Pair") {
        std::string input = ".section\n.text\n";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next();
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, "section", jac::as::Token::Section},
            jac::as::Token{1, 9, "\n", jac::as::Token::Newline},
            jac::as::Token{2, 1, "text", jac::as::Token::Section},
            jac::as::Token{2, 6, "\n", jac::as::Token::Newline}
        }));
    }
}


TEST_CASE("Label", "[assembler]") {
    SECTION("Single") {
        std::string input = "label:\n";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next();
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, "label", jac::as::Token::Label},
            jac::as::Token{1, 7, "\n", jac::as::Token::Newline}
        }));
    }
}


TEST_CASE("Instruction", "[assembler]") {
    SECTION("Single") {
        std::string input = "add %r1, %r2, %r3\n";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next();
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, "add", jac::as::Token::Instruction},
            jac::as::Token{1, 5, "r1", jac::as::Token::Register},
            jac::as::Token{1, 8, ",", jac::as::Token::Comma},
            jac::as::Token{1, 10, "r2", jac::as::Token::Register},
            jac::as::Token{1, 13, ",", jac::as::Token::Comma},
            jac::as::Token{1, 15, "r3", jac::as::Token::Register},
            jac::as::Token{1, 18, "\n", jac::as::Token::Newline}
        }));
    }
}


TEST_CASE("Immediate", "[assembler]") {
    SECTION("Single") {
        std::string input = "add $123456, %r2, %r3\n";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next();
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, "add", jac::as::Token::Instruction},
            jac::as::Token{1, 5, "123456", jac::as::Token::Immediate},
            jac::as::Token{1, 12, ",", jac::as::Token::Comma},
            jac::as::Token{1, 14, "r2", jac::as::Token::Register},
            jac::as::Token{1, 17, ",", jac::as::Token::Comma},
            jac::as::Token{1, 19, "r3", jac::as::Token::Register},
            jac::as::Token{1, 22, "\n", jac::as::Token::Newline}
        }));
    }
}


TEST_CASE("Comment", "[assembler]") {
    SECTION("Multiline") {
        std::string input = "/* comment \n next line */\n";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next(false);
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, " comment \n next line ", jac::as::Token::Comment},
            jac::as::Token{2, 14, "\n", jac::as::Token::Newline}
        }));
    }
}


TEST_CASE("Real", "[assembler]") {
    SECTION("Single") {
        std::string input = R"(
.text
i_x:
        pushq %rbp
        movq %rsp, %rbp
        movl %esi, %eax
        addl $3, %eax
        addl %edi, %eax
        imull $2, %eax, %eax
        leave
        ret
.type i_x, @function
.size i_x, .-i_x
/* end function i_x */
)";
        jac::as::Scanner scanner(input, report);
        TokenVector tokens;

        while (true) {
            auto token = scanner.next(false);
            if (!token) {
                break;
            }
            tokens.push_back(token);
        }

        REQUIRE(tokens == TokenVector({
            jac::as::Token{1, 1, "\n", jac::as::Token::Newline},
            jac::as::Token{2, 1, "text", jac::as::Token::Section},
            jac::as::Token{2, 6, "\n", jac::as::Token::Newline},
            jac::as::Token{3, 1, "i_x", jac::as::Token::Label},
            jac::as::Token{3, 5, "\n", jac::as::Token::Newline},
            jac::as::Token{4, 9, "pushq", jac::as::Token::Instruction},
            jac::as::Token{4, 15, "rbp", jac::as::Token::Register},
            jac::as::Token{4, 19, "\n", jac::as::Token::Newline},
            jac::as::Token{5, 9, "movq", jac::as::Token::Instruction},
            jac::as::Token{5, 14, "rsp", jac::as::Token::Register},
            jac::as::Token{5, 18, ",", jac::as::Token::Comma},
            jac::as::Token{5, 20, "rbp", jac::as::Token::Register},
            jac::as::Token{5, 24, "\n", jac::as::Token::Newline},
            jac::as::Token{6, 9, "movl", jac::as::Token::Instruction},
            jac::as::Token{6, 14, "esi", jac::as::Token::Register},
            jac::as::Token{6, 18, ",", jac::as::Token::Comma},
            jac::as::Token{6, 20, "eax", jac::as::Token::Register},
            jac::as::Token{6, 24, "\n", jac::as::Token::Newline},
            jac::as::Token{7, 9, "addl", jac::as::Token::Instruction},
            jac::as::Token{7, 14, "3", jac::as::Token::Immediate},
            jac::as::Token{7, 16, ",", jac::as::Token::Comma},
            jac::as::Token{7, 18, "eax", jac::as::Token::Register},
            jac::as::Token{7, 22, "\n", jac::as::Token::Newline},
            jac::as::Token{8, 9, "addl", jac::as::Token::Instruction},
            jac::as::Token{8, 14, "edi", jac::as::Token::Register},
            jac::as::Token{8, 18, ",", jac::as::Token::Comma},
            jac::as::Token{8, 20, "eax", jac::as::Token::Register},
            jac::as::Token{8, 24, "\n", jac::as::Token::Newline},
            jac::as::Token{9, 9, "imull", jac::as::Token::Instruction},
            jac::as::Token{9, 15, "2", jac::as::Token::Immediate},
            jac::as::Token{9, 17, ",", jac::as::Token::Comma},
            jac::as::Token{9, 19, "eax", jac::as::Token::Register},
            jac::as::Token{9, 23, ",", jac::as::Token::Comma},
            jac::as::Token{9, 25, "eax", jac::as::Token::Register},
            jac::as::Token{9, 29, "\n", jac::as::Token::Newline},
            jac::as::Token{10, 9, "leave", jac::as::Token::Instruction},
            jac::as::Token{10, 14, "\n", jac::as::Token::Newline},
            jac::as::Token{11, 9, "ret", jac::as::Token::Instruction},
            jac::as::Token{11, 12, "\n", jac::as::Token::Newline},
            jac::as::Token{12, 1, "type i_x, @function", jac::as::Token::Section},
            jac::as::Token{12, 21, "\n", jac::as::Token::Newline},
            jac::as::Token{13, 1, "size i_x, .-i_x", jac::as::Token::Section},
            jac::as::Token{13, 17, "\n", jac::as::Token::Newline},
            jac::as::Token{14, 1, " end function i_x ", jac::as::Token::Comment},
            jac::as::Token{14, 23, "\n", jac::as::Token::Newline}
        }));
    }
}
