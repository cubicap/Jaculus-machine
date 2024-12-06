#pragma once

#include <stdexcept>
#include <unordered_map>


namespace jac::cfg {


enum class ValueType {
    Void,
    I32,
    Double,
    Bool,
    Object,
    String,
    Buffer,
    Any
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


enum class Opcode {
    // Binary
    Add = 1,
    Sub,
    Mul,
    Div,
    Rem,
    Pow,
    LShift,
    RShift,
    URShift,
    BitAnd,
    BitOr,
    BitXor,
    Eq,
    Neq,
    Gt,
    Gte,
    Lt,
    Lte,
    GetMember,

    // Unary
    Set,  // TODO: must support type conversion
    BoolNot,
    BitNot,
    UnPlus,
    UnMinus,
    Dup,
    PushFree
};
constexpr Opcode MIN_UNARY_OP = Opcode::Set;

inline bool isBinary(Opcode op) {
    return op < MIN_UNARY_OP;
}

inline bool hasResult(Opcode op) {
    return op != Opcode::PushFree && op != Opcode::Dup;
}


using ResMapping = ValueType(*)(ValueType, ValueType);

namespace detail {  // FIXME: check conversions

    inline bool anyFloating(ValueType a, ValueType b) {
        return isFloating(a) || isFloating(b);
    }

    inline bool anyVoid(ValueType a, ValueType b) {
        return a == ValueType::Void || b == ValueType::Void;
    }

    inline bool anyObject(ValueType a, ValueType b) {
        return a == ValueType::Object || b == ValueType::Object;
    }

    inline bool anyAny(ValueType a, ValueType b) {
        return a == ValueType::Any || b == ValueType::Any;
    }


    inline ValueType additiveRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        if (anyFloating(a, b)) {
            return ValueType::Double;
        }
        return ValueType::I32;
    }

    inline ValueType divRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType powRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Double;
    }

    inline ValueType shiftRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType booleanRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType bitwiseRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::I32;
    }

    inline ValueType relationalRes(ValueType a, ValueType b) {
        if (anyVoid(a, b)) {
            throw std::runtime_error("Invalid argument types");
        }
        return ValueType::Bool;
    }

    inline ValueType setRes(ValueType a, ValueType) {
        if (a == ValueType::Void) {
            throw std::runtime_error("Invalid argument type");
        }
        return a;
    }


    const std::unordered_map<Opcode, ResMapping> opResults = {
        { Opcode::Add, additiveRes },
        { Opcode::Sub, additiveRes },
        { Opcode::Mul, additiveRes },
        { Opcode::Div, divRes },
        { Opcode::Rem, divRes },
        { Opcode::Pow, powRes },
        { Opcode::LShift, shiftRes },
        { Opcode::RShift, shiftRes },
        { Opcode::URShift, shiftRes },
        { Opcode::BitAnd, bitwiseRes },
        { Opcode::BitOr, bitwiseRes },
        { Opcode::BitXor, bitwiseRes },
        { Opcode::Eq, relationalRes },
        { Opcode::Neq, relationalRes },
        { Opcode::Gt, relationalRes },
        { Opcode::Gte, relationalRes },
        { Opcode::Lt, relationalRes },
        { Opcode::Lte, relationalRes },
        { Opcode::Set, setRes },
        { Opcode::BoolNot, booleanRes },
        { Opcode::BitNot, bitwiseRes },
        { Opcode::UnPlus, additiveRes },
        { Opcode::UnMinus, additiveRes }
    };

} // namespace detail


inline ValueType resultType(Opcode op, ValueType a, ValueType b) {
    return detail::opResults.at(op)(a, b);
}

inline ValueType commonUpcast(ValueType a, ValueType b) {
    if (detail::anyVoid(a, b)) {
        return ValueType::Void;
    }
    if (detail::anyAny(a, b) || detail::anyObject(a, b)) {
        return ValueType::Any;
    }
    if (detail::anyFloating(a, b)) {
        return ValueType::Double;
    }
    return ValueType::I32;
}


// inline std::vector<ValueType> argTypes(const Operation& op) {
//     return detail::argTypes.at(op.op)(op);
// }


}  // namespace jac::cfg
