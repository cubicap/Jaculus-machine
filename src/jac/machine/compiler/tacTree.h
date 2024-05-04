#pragma once

#include <list>
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
    Gt,
    Gte,
    Lt,
    Lte,

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
        { Opcode::Gt, relational },
        { Opcode::Gte, relational },
        { Opcode::Lt, relational },
        { Opcode::Lte, relational },
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
        NextParent,
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

    static Jump nextParent() {
        return { NextParent };
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


struct Nesting {
    using Node = std::variant<Block, Nesting>;
    std::list<Node> nodes;
    std::pair<Nesting*, std::size_t> location;

    Nesting(Nesting* parent, std::size_t index) : location(parent, index) { }

    Identifier firstLabel() const {
        if (nodes.empty()) {
            throw std::runtime_error("No label to return");
        }
        if (std::holds_alternative<Block>(nodes.front())) {
            return std::get<Block>(nodes.front()).name;
        }
        return std::get<Nesting>(nodes.front()).firstLabel();
    }

    Identifier nextParentLabel() const {
        if (location.first == nullptr) {
            throw std::runtime_error("No parent label to return");
        }
        return location.first->nextUpLabelAfter(location.second);
    }

    Identifier nextUpLabelAfter(std::size_t index) const {
        // TODO: optimize
        for (auto it = std::next(nodes.begin(), index); it != nodes.end(); ++it) {
            if (std::holds_alternative<Block>(*it)) {
                return std::get<Block>(*it).name;
            }
        }

        if (location.first == nullptr) {
            throw std::runtime_error("No parent label to return");
        }
        return location.first->nextUpLabelAfter(location.second);
    }
};


inline tac::Identifier newBlockName(std::string prefix) {
    static unsigned counter = 0;
    return prefix + std::to_string(counter++);
}


class FunctionBody {
    Nesting _root{ nullptr, 0 };
    bool _blockClosed = true;

    Nesting* _current;
public:
    FunctionBody() : _current(&_root) { }

    Block& currentBlock() {
        if (_blockClosed || _current->nodes.empty()) {
            _current->nodes.emplace_back(Block{ newBlockName("L") });
            _blockClosed = false;
        }
        return std::get<Block>(_current->nodes.back());
    }

    void emitStatement(Statement statement) {
        auto& block = currentBlock();
        block.statements.statements.push_back(statement);
    }

    Block& endBlock(Jump jump) {
        auto& block = currentBlock();
        block.jump = jump;
        _blockClosed = true;

        return block;
    }

    bool isClosed() {
        return _blockClosed;
    }

    Nesting& nestingIn() {
        if (!_blockClosed) {
            endBlock(Jump::next());
        }
        _current->nodes.emplace_back(Nesting{ _current, _current->nodes.size() });
        _current = &std::get<Nesting>(_current->nodes.back());

        return *_current;
    }

    void nestingOut() {
        endBlock(Jump::nextParent());
        _current = _current->location.first;
    }

    bool isRootNesting() {
        return _current == &_root;
    }

    const Nesting& root() const {
        return _root;
    }
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


class Function {
    Identifier _name;
    std::vector<Variable> _args;
    std::vector<Variable> _innerVars;
    ValueType _returnType;

    Locals _locals;

    std::set<std::string> _requiredFunctions;

public:
    FunctionBody body;

    Function(Identifier name, std::vector<Variable> args, ValueType returnType):
        _name(name), _args(args), _returnType(returnType) { }

    Locals& currentLocals() {
        // TODO: optimize
        _locals = {};
        for (const auto& arg : _args) {
            _locals.add(arg.first, arg.second);
        }
        for (const auto& var : _innerVars) {
            _locals.add(var.first, var.second);
        }
        return _locals;
    }

    std::vector<Variable> getInnerVars() const {
        return _innerVars;
    }

    void addRequiredFunction(std::string name) {
        _requiredFunctions.insert(name);
    }

    void addLocal(tac::Variable var) {
        // TODO: add to nested locals
        _innerVars.push_back(var);
    }

    const auto& args() const {
        return _args;
    }

    const auto& name() const {
        return _name;
    }

    const auto& requiredFunctions() const {
        return _requiredFunctions;
    }

    auto returnType() const {
        return _returnType;
    }
};


struct TacEmitor {
    Function out;
};


} // namespace jac::tac
