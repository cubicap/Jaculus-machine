#pragma once

#include <list>
#include <memory>
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

    bool isVariable() const {
        return std::holds_alternative<Variable>(value);
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


struct Operation {
    Opcode op;

    std::vector<Arg> args;
};


struct Call {
    Identifier name;
    std::vector<Arg> args;
};


struct Conversion {
    Arg from;
    Variable to;
};


struct Statement {
    std::variant<Operation, Call, Conversion> op;
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

    static Jump retVal(Variable value) {
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
    Nesting(const Nesting&) = delete;
    Nesting& operator=(const Nesting&) = delete;
    Nesting(Nesting&&) = delete;
    Nesting& operator=(Nesting&&) = delete;

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
    std::unique_ptr<Nesting> _root = std::make_unique<Nesting>(nullptr, 0);
    bool _blockClosed = true;

    Nesting* _current;
public:
    FunctionBody() : _current(_root.get()) { }

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
        _current->nodes.emplace_back(std::in_place_type_t<Nesting>{}, _current, _current->nodes.size());
        _current = &std::get<Nesting>(_current->nodes.back());

        return *_current;
    }

    void nestingOut() {
        endBlock(Jump::nextParent());
        _current = _current->location.first;
    }

    bool isRootNesting() {
        return _current == _root.get();
    }

    const Nesting& root() const {
        return *_root;
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
    std::string _name;
    std::vector<Variable> _args;
    std::vector<Variable> _innerVars;
    ValueType _returnType;

    Locals _locals;

    std::set<std::string> _requiredFunctions;
public:
    FunctionBody body;

    Function(std::string name, std::vector<Variable> args, ValueType returnType):
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

    const std::vector<Variable>& getInnerVars() const {
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


using ResMapping = ValueType(*)(ValueType, ValueType);
using ArgsMapping = std::vector<ValueType>(*)(const Operation&);

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


    inline ValueType additiveRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        if (anyFloating(a, b)) {
            return ValueType::Double;
        }
        return ValueType::I32;
    }

    inline ValueType divRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType powRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType shiftRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType booleanRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType bitwiseRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType relationalRes(ValueType a, ValueType b) {
        if (anyVoid(a, b) || anyPtr(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType copyRes(ValueType a, ValueType) {
        if (a == ValueType::Void || a == ValueType::Ptr) {
            throw std::runtime_error("Invalid argument type");
        }
        return a;
    }


    const std::unordered_map<Opcode, ResMapping> opResults = {
        { Opcode::Add, additiveRes },
        { Opcode::Sub, additiveRes },
        { Opcode::Mul, additiveRes },
        { Opcode::Div, divRes },
        { Opcode::Mod, divRes },
        { Opcode::Pow, powRes },
        { Opcode::LShift, shiftRes },
        { Opcode::RShift, shiftRes },
        { Opcode::URShift, shiftRes },
        { Opcode::BoolAnd, booleanRes },
        { Opcode::BoolOr, booleanRes },
        { Opcode::BitAnd, bitwiseRes },
        { Opcode::BitOr, bitwiseRes },
        { Opcode::BitXor, bitwiseRes },
        { Opcode::Eq, relationalRes },
        { Opcode::Neq, relationalRes },
        { Opcode::Gt, relationalRes },
        { Opcode::Gte, relationalRes },
        { Opcode::Lt, relationalRes },
        { Opcode::Lte, relationalRes },
        { Opcode::Copy, copyRes },
        { Opcode::BoolNot, booleanRes },
        { Opcode::BitNot, bitwiseRes },
        { Opcode::UnPlus, additiveRes },
        { Opcode::UnMinus, additiveRes }
    };


    inline std::vector<ValueType> arithmeticArgs(const Operation& op) {
        if (op.args.size() < 2) {
            throw std::runtime_error("Invalid number of arguments");
        }
        return std::vector<ValueType>(op.args.size() - 1, op.args[0].type());
    }

    inline std::vector<ValueType> bitwiseArgs(const Operation& op) {
        if (op.args.size() < 2) {
            throw std::runtime_error("Invalid number of arguments");
        }
        return std::vector<ValueType>(op.args.size() - 1, ValueType::I32);
    }

    inline std::vector<ValueType> booleanArgs(const Operation& op) {
        if (op.args.size() < 2) {
            throw std::runtime_error("Invalid number of arguments");
        }
        return std::vector<ValueType>(op.args.size() - 1, ValueType::Bool);
    }

    inline std::vector<ValueType> relationalArgs(const Operation& op) {
        if (op.args.size() < 2) {
            throw std::runtime_error("Invalid number of arguments");
        }
        if (anyFloating(op.args[1].type(), op.args[2].type())) {
            return std::vector<ValueType>(op.args.size() - 1, ValueType::Double);
        }
        return std::vector<ValueType>(op.args.size() - 1, ValueType::I32);
    }

    inline std::vector<ValueType> copyArgs(const Operation& op) {
        if (op.args.size() != 2) {
            throw std::runtime_error("Invalid number of arguments");
        }
        return { op.args[0].type() };
    }

    const std::unordered_map<Opcode, ArgsMapping> argTypes = {
        { Opcode::Add, arithmeticArgs },
        { Opcode::Sub, arithmeticArgs },
        { Opcode::Mul, arithmeticArgs },
        { Opcode::Div, arithmeticArgs },
        { Opcode::Mod, arithmeticArgs },
        { Opcode::Pow, arithmeticArgs },
        { Opcode::LShift, bitwiseArgs },
        { Opcode::RShift, bitwiseArgs },
        { Opcode::URShift, bitwiseArgs },
        { Opcode::BoolAnd, booleanArgs },
        { Opcode::BoolOr, booleanArgs },
        { Opcode::BitAnd, bitwiseArgs },
        { Opcode::BitOr, bitwiseArgs },
        { Opcode::BitXor, bitwiseArgs },
        { Opcode::Eq, relationalArgs },
        { Opcode::Neq, relationalArgs },
        { Opcode::Gt, relationalArgs },
        { Opcode::Gte, relationalArgs },
        { Opcode::Lt, relationalArgs },
        { Opcode::Lte, relationalArgs },
        { Opcode::Copy, copyArgs },
        { Opcode::BoolNot, booleanArgs },
        { Opcode::BitNot, bitwiseArgs },
        { Opcode::UnPlus, arithmeticArgs },
        { Opcode::UnMinus, arithmeticArgs }
    };

} // namespace detail


inline ValueType resultType(Opcode op, ValueType a, ValueType b) {
    return detail::opResults.at(op)(a, b);
}


inline std::vector<ValueType> argTypes(const Operation& op) {
    return detail::argTypes.at(op.op)(op);
}


} // namespace jac::tac
