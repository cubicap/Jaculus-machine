#pragma once

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
    { "**", 1 }, // note: left not result of prefix unary operator
    { "*", 2 },
    { "/", 2 },
    { "%", 2 },
    { "+", 3 },
    { "-", 3 },
    { "<<", 4 },
    { ">>", 4 },
    { ">>>", 4 },
    { "<", 5 },
    { "<=", 5 },
    { ">", 5 },
    { ">=", 5 },
    { "in", 5 },
    { "instanceof", 5 },
    { "==", 6 },
    { "!=", 6 },
    { "===", 6 },
    { "!==", 6 },
    { "&", 7 },
    { "^", 8 },
    { "|", 9 },
    { "&&", 10 },
    { "||", 11 },
    { "??", 11 }  // note: neither operand is result of && or ||
};

} // namespace jac
