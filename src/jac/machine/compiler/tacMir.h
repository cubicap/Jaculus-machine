#pragma once

#include <cstdint>
#include <ostream>
#include <variant>

#include "tacTree.h"


namespace jac::tac::mir {


inline std::ostream& printIndent(std::ostream& os) {
    for (int i = 0; i < 4; ++i) {
        os << ' ';
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, ValueType type) {
    switch (type) {
        case ValueType::I32: os << "i32"; break;
        case ValueType::Double: os << "d"; break;
        case ValueType::Ptr: os << "p"; break;
        case ValueType::Void: os << "v"; break;
        case ValueType::Bool: os << "i32"; break;
    }
    return os;
}

inline std::ostream& generateLocalType(std::ostream& os, ValueType type) {
    switch (type) {
        case ValueType::I32: os << "i64"; break;
        case ValueType::Double: os << "d"; break;
        case ValueType::Ptr: os << "i64"; break;
        case ValueType::Void: throw std::runtime_error("Void type not supported");
        case ValueType::Bool: os << "i64"; break;
    }
    return os;
}

namespace detail {

    struct ArgVisitor {
        std::ostream& os;

        void operator()(const Variable& v) {
            os << v.first;
        }

        void operator()(auto i) {
            os << i;
        }
    };

} // namespace detail

inline std::ostream& generate(std::ostream& os, const Arg& arg_) {
    std::visit(detail::ArgVisitor{os}, arg_.value);
    return os;
}


inline std::ostream& generate(std::ostream& os, ValueType type, Opcode op) {
    switch (type) {
        case ValueType::I32: switch (op) {
            case Opcode::Add: os << "adds"; break;
            case Opcode::Sub: os << "subs"; break;
            case Opcode::Mul: os << "muls"; break;
            case Opcode::Div: os << "div"; throw std::runtime_error("Division not supported on integers");
            case Opcode::Mod: os << "mods"; break;
            case Opcode::Pow: throw std::runtime_error("Pow not implemented");
            case Opcode::LShift: os << "lshs"; break;
            case Opcode::RShift: os << "rshs"; break;
            case Opcode::URShift: os << "urshs"; break;
            case Opcode::BoolAnd: throw std::runtime_error("BoolAnd not supported on integers");
            case Opcode::BoolOr: throw std::runtime_error("BoolOr not supported on integers");
            case Opcode::BoolNot: throw std::runtime_error("BoolNot not supported on integers");
            case Opcode::BitAnd: os << "ands"; break;
            case Opcode::BitOr: os << "ors"; break;
            case Opcode::BitXor: os << "xors"; break;
            case Opcode::BitNot: os << "xors"; break;
            case Opcode::Eq: os << "eqs"; break;
            case Opcode::Neq: os << "nes"; break;
            case Opcode::Gt: os << "gts"; break;
            case Opcode::Gte: os << "ges"; break;
            case Opcode::Lt: os << "lts"; break;
            case Opcode::Lte: os << "les"; break;
            case Opcode::Copy: os << "mov"; break;
            case Opcode::UnMinus: os << "negs"; break;
            case Opcode::UnPlus: os << "mov"; break;
        } break;
        case ValueType::Double: switch (op) {
            case Opcode::Add: os << "dadd"; break;
            case Opcode::Sub: os << "dsub"; break;
            case Opcode::Mul: os << "dmul"; break;
            case Opcode::Div: os << "ddiv"; break;
            case Opcode::Mod: throw std::runtime_error("Mod not supported on doubles");
            case Opcode::Pow: throw std::runtime_error("Pow not implemented");
            case Opcode::LShift: throw std::runtime_error("LShift not supported on doubles");
            case Opcode::RShift: throw std::runtime_error("RShift not supported on doubles");
            case Opcode::URShift: throw std::runtime_error("URShift not supported on doubles");
            case Opcode::BoolAnd: throw std::runtime_error("BoolAnd not supported on doubles");
            case Opcode::BoolOr: throw std::runtime_error("BoolOr not supported on doubles");
            case Opcode::BoolNot: throw std::runtime_error("BoolNot not supported on doubles");
            case Opcode::BitAnd: throw std::runtime_error("BitAnd not supported on doubles");
            case Opcode::BitOr: throw std::runtime_error("BitOr not supported on doubles");
            case Opcode::BitXor: throw std::runtime_error("BitXor not supported on doubles");
            case Opcode::BitNot: throw std::runtime_error("BitNot not supported on doubles");
            case Opcode::Eq: os << "deq"; break;
            case Opcode::Neq: os << "dne"; break;
            case Opcode::Gt: os << "dgt"; break;
            case Opcode::Gte: os << "dge"; break;
            case Opcode::Lt: os << "dlt"; break;
            case Opcode::Lte: os << "dle"; break;
            case Opcode::Copy: os << "dmov"; break;
            case Opcode::UnMinus: os << "dneg"; break;
            case Opcode::UnPlus: os << "dmov"; break;
        } break;
        case ValueType::Ptr: throw std::runtime_error("Pointer arithmetic not supported");
        case ValueType::Void: throw std::runtime_error("Void type not supported");
        case ValueType::Bool: switch (op) {
            case Opcode::BoolAnd: os << "ands"; break;
            case Opcode::BoolOr: os << "ors"; break;
            case Opcode::BoolNot: os << "xors"; break;
            case Opcode::Copy: os << "mov"; break;
            default: throw std::runtime_error("Unsupported operation on bool " + std::to_string(static_cast<int>(op)));
        }
    }
    return os;
}


inline std::ostream& generateOpSuffix(std::ostream& os, ValueType type, Opcode op) {
    switch (type) {
        case ValueType::I32: switch (op) {
            case Opcode::BitXor: os << ", 0xffffffff"; break;
            default: break;
        } break;
        case ValueType::Bool: switch (op) {
            case Opcode::BoolNot: os << ", 1"; break;
            default: break;
        } break;
        default: break;
    }
    return os;
}


inline Identifier convId() {
    static unsigned counter = 0;
    std::string name = "tc_" + std::to_string(counter++);
    return Identifier{name};
}


inline void generateConversion(std::ostream& os, Arg value, Variable to) {
    auto& id = to.first;

    printIndent(os);
    switch (value.type()) {
        case ValueType::I32:
            switch (to.second) {
                case ValueType::I32: os << "mov " << id << ", "; generate(os, value); break;
                case ValueType::Bool: os << "nes " << id << ", "; generate(os, value) << ", 0"; break;
                case ValueType::Double: os << "i2d " << id << ", "; generate(os, value); break;
                case ValueType::Ptr: throw std::runtime_error("Unsupported conversion to Ptr");
                case ValueType::Void: throw std::runtime_error("Unsupported conversion to Void");
            } break;
        case ValueType::Bool:
            switch (to.second) {
                case ValueType::I32: os << "mov " << id << ", "; generate(os, value); break;
                case ValueType::Bool: os << "mov " << id << ", "; generate(os, value); break;
                case ValueType::Double: os << "i2d " << id << ", "; generate(os, value); break;
                case ValueType::Ptr: throw std::runtime_error("Unsupported conversion to Ptr");
                case ValueType::Void: throw std::runtime_error("Unsupported conversion to Void");
            } break;
        case ValueType::Double:
            switch (to.second) {
                case ValueType::I32: os << "d2i " << id << ", "; generate(os, value); break;
                case ValueType::Bool: os << "dne " << id << ", "; generate(os, value) << ", 0"; break;
                case ValueType::Double: os << "dmov " << id << ", "; generate(os, value); break;
                case ValueType::Ptr: throw std::runtime_error("Unsupported conversion to Ptr");
                case ValueType::Void: throw std::runtime_error("Unsupported conversion to Void");
            } break;
        case ValueType::Ptr: throw std::runtime_error("Unsupported conversion from Ptr");
        case ValueType::Void: throw std::runtime_error("Unsupported conversion from Void");
    }

    os << '\n';
}

inline std::ostream& generateDummy(std::ostream& os, Variable ident) {
    printIndent(os);
    generate(os, ident.second, Opcode::Copy);
    os << " " << ident.first << ", " << ident.first << '\n';
    return os;

}

inline std::ostream& generate(std::ostream& os, const Operation& op) {
    if (op.args.size() < 2) {
        throw std::runtime_error("Operation with less than 2 arguments");
    }
    std::vector<Variable> args;
    std::vector<ValueType> newTypes = argTypes(op);
    args.reserve(op.args.size());

    args.push_back(std::get<Variable>(op.args[0].value));

    bool needsDummy = false;
    for (size_t i = 1; i < op.args.size(); ++i) {
        if (op.args[i].isVariable() && op.args[i].type() == newTypes[i - 1]) {
            continue;
        }
        needsDummy = true;
    }
    if (needsDummy) {
        generateDummy(os, args[0]);
    }
    for (size_t i = 1; i < op.args.size(); ++i) {
        if (op.args[i].isVariable() && op.args[i].type() == newTypes[i - 1]) {
            args.push_back(std::get<Variable>(op.args[i].value));
            continue;
        }
        Variable arg(convId(), newTypes[i - 1]);

        printIndent(os);
        os << "local ";
        generateLocalType(os, arg.second);
        os << ":" << arg.first << '\n';

        generateConversion(os, op.args[i], arg);
        args.push_back(arg);
    }

    printIndent(os);
    generate(os, newTypes[0], op.op);
    bool first = true;
    for (const auto& var : args) {
        if (!first) {
            os << ", ";
        }
        else {
            os << ' ';
            first = false;
        }
        generate(os, var);
    }
    generateOpSuffix(os, newTypes[0], op.op);
    return os;
}


inline std::ostream& generate(std::ostream& os, const Call& c) {
    printIndent(os);
    os << "call p_" << c.name << ", f_" << c.name;
    for (const auto& arg : c.args) {
        os << ", ";
        generate(os, arg);
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Conversion& c) {
    generateConversion(os, c.from, c.to);
    return os;
}


inline std::ostream& generate(std::ostream& os, const Statement& s) {
    return std::visit([&os](const auto& s_) -> std::ostream& {
        return generate(os, s_);
    }, s.op);
}


inline std::ostream& generate(std::ostream& os, const StatementList& sl) {
    for (const auto& s : sl.statements) {
        generate(os, s);
        os << '\n';
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Block& b, const Nesting& parent) {
    os << b.name << ":\n";
    generate(os, b.statements);

    Jump j = b.jump;
    printIndent(os);
    switch (j.type) {
        case Jump::Next:
            if (b.statements.statements.empty()) {
                // TODO: find better solution
                os << "bnes " << b.name << ", 0, 0\n";
            }
            break;
        case Jump::NextParent:
            os << "jmp " << parent.nextParentLabel();
            break;
        case Jump::Unconditional:
            os << "jmp " << j.labelA;
            break;
        case Jump::Jnz:
            os << "bts " << j.labelA << ", ";
            generate(os, j.value);
            os << "\n";
            printIndent(os);
            os << "jmp " << j.labelB;
            break;
        case Jump::Return:
            os << "ret";
            break;
        case Jump::ReturnValue:
            os << "ret ";
            generate(os, j.value);
            break;
    }
    os << '\n';
    return os;
}


inline std::ostream& generate(std::ostream& os, const Nesting& n) {
    struct visitor {
        std::ostream& os;
        const Nesting& parent;

        void operator()(const Block& b) {
            generate(os, b, parent);
        }

        void operator()(const Nesting& n) {
            generate(os, n);
        }
    };

    for (const auto& node : n.nodes) {
        std::visit(visitor{os, n}, node);
    }

    return os;
}


inline std::ostream& generate(std::ostream& os, const FunctionBody& fb) {
    generate(os, fb.root());
    return os;
}


inline std::ostream& generate(std::ostream& os, const Function& f) {
    os << "f_";
    os << f.name() << ": func ";
    if (f.returnType() != ValueType::Void) {
        generate(os, f.returnType());
    }
    if (!f.args().empty()) {
        bool first = f.returnType() == ValueType::Void;
        for (const auto& [arg, type] : f.args()) {
            if (!first) {
                os << ", ";
            }
            else {
                first = false;
            }
            generate(os, type);
            os << ":" << arg;
        }
    }
    os << '\n';

    if (!f.getInnerVars().empty()) {
        printIndent(os);
        os << "local ";
        bool first = true;
        for (const auto& [id, type] : f.getInnerVars()) {
            if (!first) {
                os << ", ";
            }
            else {
                first = false;
            }

            generateLocalType(os, type);
            os << ":" << id;
        }
        os << '\n';
    }

    generate(os, f.body);

    os << "endfunc\n";

    return os;
}


inline std::ostream& generateProto(std::ostream& os, const Function& f) {
    os << "p_" + f.name() << ": proto ";
    if (f.returnType() != ValueType::Void) {
        generate(os, f.returnType());
    }
    if (!f.args().empty()) {
        bool first = f.returnType() == ValueType::Void;
        for (const auto& [arg, type] : f.args()) {
            if (!first) {
                os << ", ";
            }
            else {
                first = false;
            }
            generate(os, type);
            os << ":" << arg;
        }
    }
    os << '\n';

    return os;
}


inline std::ostream& generateCaller(std::ostream& os, const Function& f) {
    std::string callerName(std::string("caller_") + f.name());
    os << callerName << ": func ";
    generate(os, f.returnType());
    os << ", i64:argc, p:argv\n";

    os << "    local i64:pos, ";
    generateLocalType(os, f.returnType());
    os << ":res";
    for (size_t i = 0; i < f.args().size(); ++i) {
        os << ", ";
        generateLocalType(os, f.args()[i].second);
        os << ":arg" << i;
    }
    os << '\n';
    os << "body:\n";

    // check number of arguments
    os << "    bgt fail, argc, " << f.args().size() << '\n';

    // prepare arguments
    for (size_t i = 0; i < f.args().size(); ++i) {
        const auto& [arg, type] = f.args()[i];
        os << "    mov pos, " << i * 2 << "\n";
        switch (type) {
            case tac::ValueType::I32:
                os << "    mov arg" << i << ", i32:(argv, pos, 8)\n";
                break;
            case tac::ValueType::Double:
                os << "    dmov arg" << i << ", d:(argv, pos, 8)\n";
                break;
            case tac::ValueType::Bool:
                os << "    nes arg" << i << ", i32:(argv, pos, 8), 0\n";
                break;
            default:
                throw std::runtime_error("Unsupported argument type " + std::to_string(static_cast<int>(type)));
        }
    }

    // call
    os << "    call p_" << f.name() << ", f_" << f.name() << ", res";
    for (size_t i = 0; i < f.args().size(); ++i) {
        os << ", arg" << i;
    }
    os << '\n';
    os << "    ret res\n";

    os << "fail:\n";
    // TODO: throw exception
    generateConversion(os, Arg{0}, { "res", f.returnType() });
    os << "    ret res\n";
    os << "endfunc\n\n";

    return os;
}

} // namespace jac::tac::mir
