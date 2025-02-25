#pragma once

#include <cassert>
#include <unordered_map>


namespace jac::cfg {


enum class ValueType {  // XXX: replace Void with undefined?
    Void,
    I32,
    Double,
    Bool,
    Object,
    String,
    Buffer,
    Any
};

inline  bool isNumeric(ValueType type) {
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:  case ValueType::Double:  case ValueType::Bool:
            return true;
        case ValueType::Object:  case ValueType::String:
        case ValueType::Buffer:  case ValueType::Any:
            return false;
    }
    assert(false);
}

inline bool isIntegral(ValueType type) {
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:
        case ValueType::Bool:
            return true;
        case ValueType::Double:  case ValueType::Object:  case ValueType::String:
        case ValueType::Buffer:  case ValueType::Any:
            return false;
    }
    assert(false);
}

inline ValueType toNumber(ValueType type) {
    return isIntegral(type) ? ValueType::I32 : ValueType::Double;
}

inline ValueType toPrimitive(ValueType type) {  // XXX: not by spec
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:  case ValueType::Double:  case ValueType::Bool:
            return type;
        case ValueType::String:
            return ValueType::String;
        case ValueType::Object:  case ValueType::Buffer:  case ValueType::Any:
            return ValueType::Any;
    }
    assert(false);
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

    inline bool anyVoid(ValueType a, ValueType b) {
        return a == ValueType::Void || b == ValueType::Void;
    }

    inline bool anyObject(ValueType a, ValueType b) {
        return a == ValueType::Object || b == ValueType::Object;
    }

    inline bool anyAny(ValueType a, ValueType b) {
        return a == ValueType::Any || b == ValueType::Any;
    }

    inline ValueType numericUpcast(ValueType a, ValueType b) {
        if (a == ValueType::Double || b == ValueType::Double) {
            return ValueType::Double;
        }
        return ValueType::I32;
    }

    inline ValueType additiveRes(ValueType a, ValueType b) {
        auto aPrim = toPrimitive(a);
        auto bPrim = toPrimitive(b);

        if (aPrim == bPrim) {
            return aPrim;
        }
        if (aPrim == ValueType::String || bPrim == ValueType::String) {
            return ValueType::String;
        }
        if (aPrim == ValueType::Any || bPrim == ValueType::Any) {
            return ValueType::Any;
        }
        return numericUpcast(a, b);
    }

    inline ValueType subtractiveRes(ValueType a, ValueType b) {
        auto aPrim = toPrimitive(a);
        auto bPrim = toPrimitive(b);

        if (isNumeric(aPrim) && isNumeric(bPrim)) {
            return numericUpcast(a, b);
        }
        return ValueType::Any;
    }

    inline ValueType idRes(ValueType a, ValueType) {
        assert(a != ValueType::Void);
        return a;
    }

    inline ValueType unBitwiseRes(ValueType a, ValueType) {
        assert(a != ValueType::Void);
        return ValueType::I32;
    }

    inline ValueType bitwiseRes(ValueType a, ValueType b) {
        assert(!anyVoid(a, b));
        return ValueType::I32;
    }

    inline ValueType booleanRes(ValueType a, ValueType) {
        assert(a != ValueType::Void);
        return ValueType::Bool;
    }

    inline ValueType relationalRes(ValueType a, ValueType b) {
        assert(!anyVoid(a, b));
        return ValueType::Bool;
    }

    inline ValueType toNumberRes(ValueType a, ValueType) {
        return toNumber(a);
    }

    const std::unordered_map<Opcode, ResMapping> opResults = {
        { Opcode::Add, additiveRes },
        { Opcode::Sub, subtractiveRes },
        { Opcode::Mul, subtractiveRes },
        { Opcode::Div, subtractiveRes },
        { Opcode::Rem, subtractiveRes },
        { Opcode::Pow, subtractiveRes },
        { Opcode::LShift, bitwiseRes },
        { Opcode::RShift, bitwiseRes },
        { Opcode::URShift, bitwiseRes },
        { Opcode::BitAnd, bitwiseRes },
        { Opcode::BitOr, bitwiseRes },
        { Opcode::BitXor, bitwiseRes },
        { Opcode::Eq, relationalRes },
        { Opcode::Neq, relationalRes },
        { Opcode::Gt, relationalRes },
        { Opcode::Gte, relationalRes },
        { Opcode::Lt, relationalRes },
        { Opcode::Lte, relationalRes },
        { Opcode::Set, idRes },
        { Opcode::BoolNot, booleanRes },
        { Opcode::BitNot, unBitwiseRes },
        { Opcode::UnPlus, toNumberRes },
        { Opcode::UnMinus, toNumberRes }
    };

} // namespace detail


inline ValueType resultType(Opcode op, ValueType a, ValueType b) {
    // FIXME: completely wrong
    return detail::opResults.at(op)(a, b);
}

inline ValueType commonUpcast(ValueType a, ValueType b) {
    if (detail::anyVoid(a, b)) {
        return ValueType::Void;
    }
    if (detail::anyAny(a, b) || detail::anyObject(a, b)) {
        return ValueType::Any;
    }
    if (a == ValueType::Double || b == ValueType::Double) {
        return ValueType::Double;
    }
    return ValueType::I32;
}


}  // namespace jac::cfg
