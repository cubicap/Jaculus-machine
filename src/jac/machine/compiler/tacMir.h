#pragma once

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
        case ValueType::I8: os << "i8"; break;
        case ValueType::U8: os << "u8"; break;
        case ValueType::I16: os << "i16"; break;
        case ValueType::U16: os << "u16"; break;
        case ValueType::I32: os << "i32"; break;
        case ValueType::U32: os << "u32"; break;
        case ValueType::I64: os << "i64"; break;
        case ValueType::U64: os << "u64"; break;
        case ValueType::Float: os << "f"; break;
        case ValueType::Double: os << "d"; break;
        case ValueType::Ptr: os << "p"; break;
        case ValueType::Void: os << "v"; break;
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Arg& arg_) {
    std::visit([&os](const auto& arg) {
        os << arg;
    }, arg_);
    return os;
}


inline std::ostream& generate(std::ostream& os, Opcode op) {
    // XXX: does not support other than word comparisons
    switch (op) {
        case Opcode::Add: os << "add"; break;
        case Opcode::Sub: os << "sub"; break;
        case Opcode::Mul: os << "mul"; break;
        case Opcode::Div: os << "div"; break;
        case Opcode::Mod: os << "rem"; break;
        case Opcode::Pow: throw std::runtime_error("Pow not implemented");
        case Opcode::LShift: os << "lsh"; break;
        case Opcode::RShift: os << "rsh"; break;
        case Opcode::URShift: os << "ursh"; break;
        case Opcode::BoolAnd: os << "and"; break;
        case Opcode::BoolOr: os << "or"; break;
        case Opcode::BitAnd: os << "and"; break;
        case Opcode::BitOr: os << "or"; break;
        case Opcode::BitXor: os << "xor"; break;
        case Opcode::Eq: os << "eq"; break;
        case Opcode::Neq: os << "ne"; break;
        case Opcode::SignedGt: os << "gt"; break;
        case Opcode::SignedGte: os << "ge"; break;
        case Opcode::SignedLt: os << "lt"; break;
        case Opcode::SignedLte: os << "le"; break;
        case Opcode::Copy: os << "mov"; break;
        case Opcode::UnMinus: os << "neg"; break;
        case Opcode::UnPlus: os << "mov"; break;
        default:
            throw std::runtime_error("Unknown operation");
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Operation& op) {
    printIndent(os);
    generate(os, op.op);
    bool first = true;
    for (const auto& arg : op.args) {
        if (!first) {
            os << ", ";
        }
        else {
            os << ' ';
            first = false;
        }
        generate(os, arg);
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Call& c) {
    printIndent(os);
    os << "call p_" << c.name << ", " << c.name;
    for (const auto& arg : c.args) {
        os << ", ";
        generate(os, arg);
    }
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


static inline std::ostream& generate(std::ostream& os, const Jump& j) {
    printIndent(os);
    switch (j.type) {
        case Jump::Next:
            break;
        case Jump::Unconditional:
            os << "jmp " << j.labelA;
            break;
        case Jump::Jnz:
            os << "beqs " << j.labelA << ", ";
            generate(os, j.value);
            os << ", 0\n";
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


inline std::ostream& generate(std::ostream& os, const Block& b) {
    os << b.name << ":\n";
    generate(os, b.statements);
    generate(os, b.jump);
    return os;
}


inline std::ostream& generate(std::ostream& os, const FunctionBody& fb) {
    for (const auto& b : fb.blocks) {
        generate(os, b);
    }
    return os;
}


inline std::ostream& generate(std::ostream& os, const Function& f) {
    os << f.name << ": " << "func ";
    if (f.returnType != ValueType::Void) {
        generate(os, f.returnType);
    }
    if (!f.args.empty()) {
        bool first = f.returnType == ValueType::Void;
        for (const auto& [arg, type] : f.args) {
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

    if (!f.locals.locals.empty()) {
        printIndent(os);
        os << "local ";
        bool first = true;
        for (const auto& [local, type] : f.locals.locals) {
            if (!first) {
                os << ", ";
            }
            else {
                first = false;
            }
            generate(os, type);
            os << ":" << local;
        }
        os << '\n';
    }

    generate(os, f.body);

    os << "endfunc\n";

    return os;
}


inline std::ostream& generateProto(std::ostream& os, const Function& f) {
    os << "p_" + f.name << ": proto ";
    if (f.returnType != ValueType::Void) {
        generate(os, f.returnType);
    }
    if (!f.args.empty()) {
        bool first = f.returnType == ValueType::Void;
        for (const auto& [arg, type] : f.args) {
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

} // namespace jac::tac::mir
