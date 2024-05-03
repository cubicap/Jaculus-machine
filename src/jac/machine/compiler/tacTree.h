#pragma once

#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


namespace jac::tac {


enum class ValueType {
    Void,
    I32,
    Double,
    Bool,
    Ptr
};

inline bool isSigned(ValueType type) {
    switch (type) {
        case ValueType::I32:
        case ValueType::Double:
            return true;
        default:
            return false;
    }
}

inline bool isIntegral(ValueType type) {
    switch (type) {
        case ValueType::I32:
        case ValueType::Bool:
            return true;
        default:
            return false;
    }
}

inline bool isFloating(ValueType type) {
    switch (type) {
        case ValueType::Double:
            return true;
        default:
            return false;
    }
}

using Identifier = std::string;
using Variable = std::pair<Identifier, ValueType>;

struct Arg {
    std::variant<Variable, int32_t, double> value;

    Arg(auto value_) : value(value_) { }

    ValueType type() const {
        struct visitor {
            ValueType operator()(const Variable& v) const {
                return v.second;
            }

            ValueType operator()(int32_t) const {
                return ValueType::I32;
            }

            ValueType operator()(double) const {
                return ValueType::Double;
            }
        };

        return std::visit(visitor{}, value);
    }
};

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
    SignedGt,  // TODO: unify signed/float
    SignedGte,
    SignedLt,
    SignedLte,
    FloatGt,
    FloatGte,
    FloatLt,
    FloatLte,

    // Unary
    Copy,  // TODO: must support type conversion
    BoolNot,
    BitNot,
    UnPlus,
    UnMinus
};
constexpr Opcode MIN_UNARY_OP = Opcode::Copy;

using ResMapping = ValueType(*)(ValueType, ValueType);

namespace detail {

    inline bool anyFloating(ValueType a, ValueType b) {
        return isFloating(a) || isFloating(b);
    }

    inline bool anyVoid(ValueType a, ValueType b) {
        return a == ValueType::Void || b == ValueType::Void;
    }

    inline bool anyPtr(ValueType a, ValueType b) {
        return a == ValueType::Ptr || b == ValueType::Ptr;
    }


    inline ValueType additive(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        if (anyFloating(a, b)) {
            return ValueType::Double;
        }
        return ValueType::I32;
    }

    inline ValueType div(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType pow(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType shift(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType boolean(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType bitwise(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType relational(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType copy(ValueType a, ValueType) {
        if (a == ValueType::Void || a == ValueType::Ptr) {
            throw std::runtime_error("Invalid argument type");
        }
        return a;
    }


    const std::unordered_map<Opcode, ResMapping> opResults = {
        { Opcode::Add, additive },
        { Opcode::Sub, additive },
        { Opcode::Mul, additive },
        { Opcode::Div, div },
        { Opcode::Mod, div },
        { Opcode::Pow, pow },
        { Opcode::LShift, shift },
        { Opcode::RShift, shift },
        { Opcode::URShift, shift },
        { Opcode::BoolAnd, boolean },
        { Opcode::BoolOr, boolean },
        { Opcode::BitAnd, bitwise },
        { Opcode::BitOr, bitwise },
        { Opcode::BitXor, bitwise },
        { Opcode::Eq, relational },
        { Opcode::Neq, relational },
        { Opcode::SignedGt, relational },
        { Opcode::SignedGte, relational },
        { Opcode::SignedLt, relational },
        { Opcode::SignedLte, relational },
        { Opcode::FloatGt, relational },
        { Opcode::FloatGte, relational },
        { Opcode::FloatLt, relational },
        { Opcode::FloatLte, relational },
        { Opcode::Copy, copy },
        { Opcode::BoolNot, boolean },
        { Opcode::BitNot, bitwise },
        { Opcode::UnPlus, additive },
        { Opcode::UnMinus, additive }
    };

} // namespace detail


inline ValueType resultType(Opcode op, ValueType a, ValueType b) {
    return detail::opResults.at(op)(a, b);
}


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

    Variable get(Identifier name) const {
        return { name, locals.at(name) };
    }

    bool has(Identifier name) const {
        return locals.contains(name);
    }
};


struct Function {
    Identifier name;
    std::vector<Variable> args;
    std::vector<Variable> innerVars;
    ValueType returnType;
    FunctionBody body;

    Locals locals;

    std::set<std::string> requiredFunctions;

    Block& currentBlock() {
        // TODO: block+locals tree
        return body.blocks.back();
    }

    Locals& currentLocals() {
        locals = {};
        for (const auto& arg : args) {
            locals.add(arg.first, arg.second);
        }
        for (const auto& var : innerVars) {
            locals.add(var.first, var.second);
        }
        return locals;
    }

    void addLocal(tac::Variable var) {
        innerVars.push_back(var);
    }

    std::vector<Variable> getInnerVars() const {
        return innerVars;
    }

    void addRequiredFunction(std::string name_) {
        requiredFunctions.insert(name_);
    }
};


struct TacEmitor {
    Function out;
};


} // namespace jac::tac
