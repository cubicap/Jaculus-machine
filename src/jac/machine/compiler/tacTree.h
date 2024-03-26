#pragma once

#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>


namespace jac::tac {


inline std::ostream& printIndent(std::ostream& os) {
    for (int i = 0; i < 4; ++i) {
        os << ' ';
    }
    return os;
}


enum class ValueType {
    Word,
    Long,
    Float,
    Double
};

inline std::ostream& printQbe(std::ostream& os, ValueType type) {
    switch (type) {
        case ValueType::Word: os << "w"; break;
        case ValueType::Long: os << "l"; break;
        case ValueType::Float: os << "s"; break;
        case ValueType::Double: os << "d"; break;
    }
    return os;
}

using Identifier = std::string;
using Arg = std::variant<Identifier, int, long, float, double>;

inline std::ostream& printQbe(std::ostream& os, const Arg& arg_) {
    std::visit([&os](const auto& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Identifier>) {
            os << '%' << arg;
        } else {
            os << arg;
        }
    }, arg_);
    return os;
}

enum class Operation {
    // Binary
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Pow,
    LShift,
    RShift,
    URShift,
    BoolAnd,
    BoolOr,
    BitAnd,
    BitOr,
    BitXor,
    Eq,
    Neq,
    SignedGt,
    SignedGte,
    SignedLt,
    SignedLte,
    FloatGt,
    FloatGte,
    FloatLt,
    FloatLte,

    // Unary
    Copy,
    BoolNot,
    BitNot,
    UnPlus,
    UnMinus
};
constexpr Operation MIN_UNARY_OP = Operation::Copy;

inline std::ostream& printQbe(std::ostream& os, Operation op) {
    // XXX: does not support other than word comparisons
    switch (op) {
        case Operation::Add: os << "add"; break;
        case Operation::Sub: os << "sub"; break;
        case Operation::Mul: os << "mul"; break;
        case Operation::Div: os << "div"; break;
        case Operation::Mod: os << "rem"; break;
        case Operation::Pow: throw std::runtime_error("Pow not implemented");
        case Operation::LShift: os << "shl"; break;
        case Operation::RShift: os << "sar"; break;
        case Operation::URShift: os << "shr"; break;
        case Operation::BoolAnd: os << "and"; break;
        case Operation::BoolOr: os << "or"; break;
        case Operation::BitAnd: os << "and"; break;
        case Operation::BitOr: os << "or"; break;
        case Operation::BitXor: os << "xor"; break;
        case Operation::Eq: os << "ceqw"; break;
        case Operation::Neq: os << "cnew"; break;
        case Operation::SignedGt: os << "csgtw"; break;
        case Operation::SignedGte: os << "csgew"; break;
        case Operation::SignedLt: os << "csltw"; break;
        case Operation::SignedLte: os << "cslew"; break;
        case Operation::Copy: os << "copy"; break;
        default:
            throw std::runtime_error("Unknown operation");
    }
    return os;
}


struct Statement {

    Operation op;

    Arg arg1;
    Arg arg2;

    ValueType resultType;
    Identifier result;
};

inline std::ostream& printQbe(std::ostream& os, const Statement& s) {
    os << '%' << s.result << " =";
    printQbe(os, s.resultType);
    os << ' ';
    printQbe(os, s.op);
    os << ' ';
    printQbe(os, s.arg1);
    if (s.op < MIN_UNARY_OP) {
        os << ", ";
        printQbe(os, s.arg2);
    }
    return os;
}


struct StatementList {
    std::vector<Statement> statements;
};

inline std::ostream& printQbe(std::ostream& os, const StatementList& sl) {
    for (const auto& s : sl.statements) {
        printIndent(os);
        printQbe(os, s);
        os << '\n';
    }
    return os;
}


struct Jump {
    enum Type {
        Next,
        Unconditional,
        Jnz,
        Return,
        ReturnValue,
        Halt
    };

    Type type;
    Identifier labelA;
    Identifier labelB;
    Arg value = 0;

    static Jump next() {
        return { Next };
    }

    static Jump unconditional(Identifier label) {
        return { Unconditional, label };
    }

    static Jump jnz(Arg value, Identifier labelA, Identifier labelB) {
        return { Jnz, labelA, labelB, value };
    }

    static Jump ret() {
        return { Return };
    }

    static Jump retVal(Arg value) {
        return { .type = ReturnValue, .value = value };
    }

    static Jump halt() {
        return { Halt };
    }
};

static inline std::ostream& printQbe(std::ostream& os, const Jump& j) {
    switch (j.type) {
        case Jump::Next:
            break;
        case Jump::Unconditional:
            os << "jmp @" << j.labelA;
            break;
        case Jump::Jnz:
            os << "jnz ";
            printQbe(os, j.value);
            os << ", @" << j.labelA << ", @" << j.labelB;
            break;
        case Jump::Return:
            os << "ret";
            break;
        case Jump::ReturnValue:
            os << "ret ";
            printQbe(os, j.value);
            break;
        case Jump::Halt:
            os << "halt";
            break;
    }
    return os;
}

struct Block {
    Identifier name;
    StatementList statements;
    Jump jump;
};

inline std::ostream& printQbe(std::ostream& os, const Block& b) {
    os << '@' << b.name << "\n";
    printQbe(os, b.statements);
    printIndent(os);
    printQbe(os, b.jump);
    os << '\n';
    return os;
}


struct FunctionBody {
    std::vector<Block> blocks;
};

inline std::ostream& printQbe(std::ostream& os, const FunctionBody& fb) {
    for (const auto& b : fb.blocks) {
        printQbe(os, b);
    }
    return os;
}


struct Function {
    Identifier name;
    std::vector<std::pair<Identifier, ValueType>> args;
    ValueType returnType;
    FunctionBody body;
};

inline std::ostream& printQbe(std::ostream& os, const Function& f) {
    os << "function ";
    printQbe(os, f.returnType);
    os << ' ';
    os << "$" << f.name;
    os << '(';
    bool first = true;
    for (const auto& [arg, type] : f.args) {
        if (!first) {
            os << ", ";
        }
        else {
            first = false;
        }
        printQbe(os, type);
        os << " %" << arg;
    }
    os << ") {\n";

    printQbe(os, f.body);

    os << "}\n";
    return os;
}


struct TacEmitor {
    Function out;
};


} // namespace jac::tac
