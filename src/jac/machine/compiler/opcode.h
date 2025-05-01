#pragma once

#include <cassert>
#include <unordered_map>


namespace jac::cfg {


enum class ValueType {  // XXX: replace Void with undefined?
    Void,
    I32,
    F64,
    Bool,
    Object,
    StringConst,
    Any
};

inline bool isNumeric(ValueType type) {
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:  case ValueType::F64:  case ValueType::Bool:
            return true;
        case ValueType::Object:  case ValueType::StringConst:
        case ValueType::Any:
            return false;
    }
    assert(false);
}

inline bool isIntegral(ValueType type) {
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:  case ValueType::Bool:
            return true;
        case ValueType::F64:  case ValueType::Object:
        case ValueType::StringConst:  case ValueType::Any:
            return false;
    }
    assert(false);
}

inline ValueType toNumber(ValueType type) {
    return isIntegral(type) ? ValueType::I32 : ValueType::F64;
}

inline ValueType toPrimitive(ValueType type) {  // XXX: not by spec
    switch (type) {
        case ValueType::Void:
            assert(false);
        case ValueType::I32:  case ValueType::F64:  case ValueType::Bool:
            return type;
        case ValueType::StringConst:
            return type;
        case ValueType::Object:  case ValueType::Any:
            return ValueType::Any;
    }
    assert(false);
}


enum class Opcode {
    // Binary
    Add = 1,    // a a -> a
    Sub,        // a a -> a
    Mul,        // a a -> a
    Div,        // a a -> a
    Rem,        // a a -> a
    Pow,        // float64 float64 -> float64
    LShift,     // int32 int32 -> int32
    RShift,     // int32 int32 -> int32
    URShift,    // int32 int32 -> int32
    BitAnd,     // int32 int32 -> int32
    BitOr,      // int32 int32 -> int32
    BitXor,     // int32 int32 -> int32
    Eq,         // a a -> bool
    Neq,        // a a -> bool
    Gt,         // a a -> bool
    Gte,        // a a -> bool
    Lt,         // a a -> bool
    Lte,        // a a -> bool
    GetMember,  // a b -> any        (a: object | any)
    SetMember,  // id val -> parent  (id: StringConst, parent: object | any)

    // Unary
    Set,        // a -> b
    BoolNot,    // a -> bool
    BitNot,     // int32 -> int32
    UnPlus,     // a -> number
    UnMinus,    // a -> a
    Dup,        // a -> void
    PushFree    // a -> void
};
constexpr Opcode MIN_UNARY_OP = Opcode::Set;

inline bool isBinary(Opcode op) {
    return op < MIN_UNARY_OP;
}

inline bool isArithmetic(Opcode op) {
    return op >= Opcode::Add && op <= Opcode::Pow;
}

inline bool isBitwise(Opcode op) {
    return op >= Opcode::LShift && op <= Opcode::BitXor;
}

inline bool isComparison(Opcode op) {
    return op >= Opcode::Eq && op <= Opcode::Lte;
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
        if (a == ValueType::F64 || b == ValueType::F64) {
            return ValueType::F64;
        }
        return ValueType::I32;
    }

    inline ValueType additiveRes(ValueType a, ValueType b) {
        auto aPrim = toPrimitive(a);
        auto bPrim = toPrimitive(b);

        if (aPrim == bPrim && aPrim != ValueType::Bool) {
            return aPrim;
        }
        if (!isNumeric(aPrim) || !isNumeric(bPrim)) {
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

    template<ValueType T>
    inline ValueType alwaysRes(ValueType, ValueType) {
        return T;
    }

    const std::unordered_map<Opcode, ResMapping> opResults = {
        { Opcode::Add, additiveRes },
        { Opcode::Sub, subtractiveRes },
        { Opcode::Mul, subtractiveRes },
        { Opcode::Div, subtractiveRes },
        { Opcode::Rem, subtractiveRes },
        { Opcode::Pow, alwaysRes<ValueType::F64> },
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
    return detail::opResults.at(op)(a, b);
}

inline ValueType commonUpcast(ValueType a, ValueType b) {
    if (detail::anyVoid(a, b)) {
        return ValueType::Void;
    }
    if (detail::anyAny(a, b) || detail::anyObject(a, b)) {
        return ValueType::Any;
    }
    if (isNumeric(a) && isNumeric(b)) {
        return detail::numericUpcast(a, b);
    }
    assert(false && "Invalid commonUpcast");
}


}  // namespace jac::cfg
