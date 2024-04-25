#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


namespace jac::tac {


enum class ValueType {
    Void,
    I8,
    U8,
    I16,
    U16,
    I32,
    U32,
    I64,
    U64,
    Float,
    Double,
    Ptr
};

using Identifier = std::string;
using Arg = std::variant<Identifier, int, long, float, double>;

enum class Opcode {
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
constexpr Opcode MIN_UNARY_OP = Opcode::Copy;


struct Operation {
    Opcode op;

    std::vector<Arg> args;
};


struct Call {
    Identifier name;
    std::vector<Arg> args;
};


struct Statement {
    std::variant<Operation, Call> op;
};


struct StatementList {
    std::vector<Statement> statements;
};


struct Jump {
    enum Type {
        Next,
        Unconditional,
        Jnz,
        Return,
        ReturnValue
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
};


struct Block {
    Identifier name;
    StatementList statements;
    Jump jump;
};


struct FunctionBody {
    std::vector<Block> blocks;
};


struct Locals {
    std::unordered_map<Identifier, ValueType> locals;

    void add(Identifier name, ValueType type) {
        locals[name] = type;
    }
};


struct Function {
    Identifier name;
    std::vector<std::pair<Identifier, ValueType>> args;
    Locals locals;
    ValueType returnType;
    FunctionBody body;

    std::set<std::string> requiredFunctions;

    Block& currentBlock() {
        // TODO: block+locals tree
        return body.blocks.back();
    }

    Locals& currentLocals() {
        return locals;
    }

    void addRequiredFunction(std::string name_) {
        requiredFunctions.insert(name_);
    }
};


struct TacEmitor {
    Function out;
};


} // namespace jac::tac
