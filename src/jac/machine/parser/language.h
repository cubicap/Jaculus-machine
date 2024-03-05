#pragma once

#include <string_view>
#include <unordered_set>
#include <unordered_map>


namespace jac {


const std::unordered_set<std::string_view> keywords = {
    "await",   "break",  "case",       "catch",     "class",   "const",  "continue",   "debugger",  // reserved
    "default", "delete", "do",         "else",      "enum",    "export", "extends",    "false",
    "finally", "for",    "function",   "if",        "import",  "in",     "instanceof", "new",
    "null",    "return", "super",      "switch",    "this",    "throw",  "true",       "try",
    "typeof",  "var",    "void",       "while",     "with",    "yield",
    "as",      "async",  "from",       "get",       "meta",    "of",      "set",       "target",  // contextually reserved
    "let",     "static", "implements", "interface", "package", "private", "protected", "public"  // strict mode reserved
};

const std::unordered_map<std::string_view, int> binaryPrecedence = {
    { "??", 1 },  // note: neither operand is result of && or ||
    { "||", 1 },
    { "&&", 2 },
    { "|", 3 },
    { "^", 4 },
    { "&", 5 },
    { "==", 6 },
    { "!=", 6 },
    { "===", 6 },
    { "!==", 6 },
    { "instanceof", 7 },
    { "<", 7 },
    { "<=", 7 },
    { ">", 7 },
    { ">=", 7 },
    { "in", 7 },
    { "<<", 8 },
    { ">>", 8 },
    { ">>>", 8 },
    { "+", 9 },
    { "-", 9 },
    { "*", 10 },
    { "/", 10 },
    { "%", 10 },
    { "**", 11 }, // note: left not result of prefix unary operator
};
const int binaryPrecedenceMax = 11;

const std::unordered_set<std::string_view> unaryOperator = {
    "!",      "~",    "+",      "-",
    "typeof", "void", "delete", "await"
};


const std::unordered_set<std::string_view> updateOperator = {
    "++x",  // note: "AssignmentTargetType must be simple"
    "--x",
    "x++",
    "x--"
};

} // namespace jac
