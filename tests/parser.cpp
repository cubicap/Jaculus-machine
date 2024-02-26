#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/parser/grammar.h>


template<>
struct Catch::StringMaker<jac::Token> {
    static std::string convert(jac::Token const& value) {
        return std::string("Token(") + std::to_string(value.line) + ", " + std::to_string(value.column) + ", " + std::string(value.text) + ", " + std::to_string(value.type) + ")";
    }
};


using TokenVector = std::vector<jac::Token>;


TEST_CASE("", "") {

}
