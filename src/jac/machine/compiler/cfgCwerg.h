#pragma once

#include "cfg.h"
#include "opcode.h"

#include "BE/Base/ir.h"
#include "BE/Base/opcode_gen.h"
#include <sstream>
#include <variant>


namespace jac::cfg {


inline cwerg::base::DK getDK(ValueType type) {
    using namespace cwerg::base;

    switch (type) {
        // case ValueType::Void:
        //     return DK::INVALID;
        case ValueType::I32:
            return DK::S32;
        case ValueType::Double:
            return DK::R64;
        case ValueType::Bool:
            return DK::U8;
        default:
            throw std::runtime_error("Invalid type");
    }
}


inline cwerg::base::Str newName(const auto& prefix) {
    static int counter = 0;
    std::stringstream ss;
    ss << "_" << prefix << counter++;
    return cwerg::base::StrNew(ss.str());
}


inline cwerg::base::Reg createConversion(cwerg::base::Bbl& bbl, cwerg::base::Fun fun, RValue from, ValueType to, const auto& regs) {
    using namespace cwerg::base;

    if (from.type == to) {
        return regs.at(from.id);
    }

    auto convReg = FunRegAdd(fun, RegNew(getDK(to), newName("conv")));
    BblInsAdd(bbl, InsNew(OPC::CONV, convReg, regs.at(from.id)));

    return convReg;
}


inline std::tuple<cwerg::base::Reg, cwerg::base::Reg, ValueType> createUpcast(
    cwerg::base::Bbl& bbl, cwerg::base::Fun fun, RValue a, RValue b, const auto& regs
) {
    using namespace cwerg::base;

    if (a.type == b.type) {
        return { regs.at(a.id), regs.at(b.id), a.type };
    }

    ValueType common = commonUpcast(a.type, b.type);

    auto convA = createConversion(bbl, fun, a, common, regs);
    auto convB = createConversion(bbl, fun, b, common, regs);

    return { convA, convB, common };
}


inline void createRelational(cwerg::base::Bbl& bbl, cwerg::base::Fun fun, const Operation& op, const auto& regs) {
    using namespace cwerg::base;

    assert(op.res.type == ValueType::Bool);
    assert(op.a.type == op.b.type);

    auto res = regs.at(op.res.id);
    auto setOne = BblNew(newName("one"));
    auto setZero = BblNew(newName("zero"));
    auto end = BblNew(newName("comm"));
    BblInsAdd(setOne, InsNew(OPC::MOV, res, ConstNewU(DK::U8, 1)));
    BblInsAdd(setOne, InsNew(OPC::BRA, end));
    BblInsAdd(setZero, InsNew(OPC::MOV, res, ConstNewU(DK::U8, 0)));
    BblInsAdd(setZero, InsNew(OPC::BRA, end));

    auto [regA, regB, _] = createUpcast(bbl, fun, op.a, op.b, regs);

    bool pos = true;
    switch (op.op) {
        case Opcode::Eq:
            BblInsAdd(bbl, InsNew(OPC::BEQ, regA, regB, setOne));
            break;
        case Opcode::Neq:
            BblInsAdd(bbl, InsNew(OPC::BNE, regA, regB, setOne));
            break;
        case Opcode::Gt:
            BblInsAdd(bbl, InsNew(OPC::BLE, regA, regB, setZero));
            pos = false;
            break;
        case Opcode::Gte:
            BblInsAdd(bbl, InsNew(OPC::BLT, regA, regB, setZero));
            pos = false;
            break;
        case Opcode::Lt:
            BblInsAdd(bbl, InsNew(OPC::BLT, regA, regB, setOne));
            break;
        case Opcode::Lte:
            BblInsAdd(bbl, InsNew(OPC::BLE, regA, regB, setOne));
            break;
        default:
            assert(false);
    }
    if (pos) {
        BblInsAdd(bbl, InsNew(OPC::BRA, setZero));
        FunBblAdd(fun, setZero);
        FunBblAdd(fun, setOne);
    }
    else {
        BblInsAdd(bbl, InsNew(OPC::BRA, setOne));
        FunBblAdd(fun, setOne);
        FunBblAdd(fun, setZero);
    }
    FunBblAdd(fun, end);

    bbl = end;
}

inline void createAdditive(cwerg::base::Bbl& bbl, cwerg::base::Fun fun, const Operation& op, const auto& regs) {
    using namespace cwerg::base;

    assert(op.op == Opcode::Add);

    auto regA = createConversion(bbl, fun, op.a, op.res.type, regs);
    auto regB = createConversion(bbl, fun, op.b, op.res.type, regs);
    auto res = regs.at(op.res.id);

    BblInsAdd(bbl, InsNew(OPC::ADD, res, regA, regB));
}

inline void createArithmetic(cwerg::base::Bbl& bbl, cwerg::base::Fun fun, const Operation& op, const auto& regs) {
    using namespace cwerg::base;
    assert(isNumeric(op.res.type));

    auto regA = createConversion(bbl, fun, op.a, op.res.type, regs);
    auto regB = createConversion(bbl, fun, op.b, op.res.type, regs);
    auto res = regs.at(op.res.id);

    switch (op.op) {
        case Opcode::Add:
            BblInsAdd(bbl, InsNew(OPC::ADD, res, regA, regB));
            break;
        case Opcode::Sub:
            BblInsAdd(bbl, InsNew(OPC::SUB, res, regA, regB));
            break;
        case Opcode::Mul:
            BblInsAdd(bbl, InsNew(OPC::MUL, res, regA, regB));
            break;
        case Opcode::Div:
            BblInsAdd(bbl, InsNew(OPC::DIV, res, regA, regB));
            break;
        case Opcode::Rem:
            BblInsAdd(bbl, InsNew(OPC::REM, res, regA, regB));
            break;
        case Opcode::Pow:
            throw std::runtime_error("Pow is not supported");
        default:
            assert(false);
    }
}

inline void createBitwise(cwerg::base::Bbl& bbl, cwerg::base::Fun fun, const Operation& op, const auto& regs) {
    using namespace cwerg::base;

    assert(op.res.type == ValueType::I32);
    auto regA = createConversion(bbl, fun, op.a, ValueType::I32, regs);
    auto regB = createConversion(bbl, fun, op.b, ValueType::I32, regs);
    auto res = regs.at(op.res.id);

    switch (op.op) {
        case Opcode::LShift:
            BblInsAdd(bbl, InsNew(OPC::SHL, res, regA, regB));
            break;
        case Opcode::RShift:
            BblInsAdd(bbl, InsNew(OPC::SHR, res, regA, regB));
            break;
        case Opcode::URShift:
            throw std::runtime_error("URShift is not supported");
        case Opcode::BitAnd:
            BblInsAdd(bbl, InsNew(OPC::AND, res, regA, regB));
            break;
        case Opcode::BitOr:
            BblInsAdd(bbl, InsNew(OPC::OR, res, regA, regB));
            break;
        case Opcode::BitXor:
            BblInsAdd(bbl, InsNew(OPC::XOR, res, regA, regB));
            break;
        default:
            assert(false);
    }
}


inline cwerg::base::Fun compile(cwerg::base::Unit& unit, Function& cfg) {
    using namespace cwerg::base;
    assert(cfg.entry == cfg.blocks.front().get());

    Fun fun = UnitFunAdd(unit, FunNew(StrNew(cfg.name()), FUN_KIND::NORMAL));

    std::map<TmpId, Reg> regs;
    std::map<BasicBlockPtr, Bbl> bbls;

    auto addReg = [&](TmpId id, ValueType type) {
        if (regs.contains(id)) {
            return regs.at(id);
        }
        Reg reg = FunRegAdd(fun, RegNew(getDK(type), StrNew("_" + std::to_string(id))));
        regs.insert({ id, reg });
        return reg;
    };


    FunNumOutputTypes(fun) = 1;
    FunOutputTypes(fun)[0] = getDK(cfg.ret);
    FunNumInputTypes(fun) = cfg.args.size();

    Bbl start;
    if (cfg.args.size() > 0) {
        start = FunBblAdd(fun, BblNew(StrNew("_start")));
        for (int i = 0; i < cfg.args.size(); ++i) {  // FIXME: maybe backwards?
            FunInputTypes(fun)[i] = getDK(cfg.args[i].type);

            auto& arg = cfg.args[i];
            Reg last = addReg(arg.id, arg.type);
            BblInsAdd(start, InsNew(OPC::POPARG, last));
        }
    }
    for (const auto& block : cfg.blocks) {
        std::stringstream ss;
        ss << "_" << block.get();
        Bbl bbl = BblNew(StrNew(ss.str()));
        bbls.insert({ block.get(), bbl });
    }
    if (cfg.args.size() > 0) {
        BblInsAdd(start, InsNew(OPC::BRA, bbls.at(cfg.blocks.front().get())));
    }

    for (const auto& block : cfg.blocks) {
        Bbl bbl = bbls.at(block.get());
        FunBblAdd(fun, bbl);
        for (auto statement : block->statements) {
            if (auto op = std::get_if<Operation>(&statement.op)) {
                if (op->res.type != ValueType::Void) {
                    addReg(op->res.id, op->res.type);
                }
                switch (op->op) {
                    // Binary
                    case Opcode::Add:
                        createAdditive(bbl, fun, *op, regs);
                        break;
                    case Opcode::Sub:  case Opcode::Mul:  case Opcode::Div:
                    case Opcode::Rem:  case Opcode::Pow:
                        createArithmetic(bbl, fun, *op, regs);
                        break;
                    case Opcode::LShift:  case Opcode::RShift:  case Opcode::URShift:
                    case Opcode::BitAnd:  case Opcode::BitOr:   case Opcode::BitXor:
                        createBitwise(bbl, fun, *op, regs);
                        break;
                    case Opcode::Eq:  case Opcode::Neq:
                    case Opcode::Gt:  case Opcode::Gte:
                    case Opcode::Lt:  case Opcode::Lte:
                        createRelational(bbl, fun, *op, regs);
                        break;
                    case Opcode::GetMember:
                        throw std::runtime_error("GetMember is not supported");

                    // Unary
                    case Opcode::Set: {
                        auto tmp = createConversion(bbl, fun, op->a, op->res.type, regs);
                        BblInsAdd(bbl, InsNew(OPC::MOV, regs.at(op->res.id), tmp));
                    } break;
                    case Opcode::BoolNot: {
                        auto res = regs.at(op->res.id);
                        auto setOne = FunBblAdd(fun, BblNew(newName("one")));
                        auto setZero = FunBblAdd(fun, BblNew(newName("zero")));
                        auto end = FunBblAdd(fun, BblNew(newName("comm")));
                        BblInsAdd(setOne, InsNew(OPC::MOV, res, ConstNewU(DK::U8, 1)));
                        BblInsAdd(setOne, InsNew(OPC::BRA, end));
                        BblInsAdd(setZero, InsNew(OPC::MOV, res, ConstNewU(DK::U8, 0)));
                        BblInsAdd(setZero, InsNew(OPC::BRA, end));
                        switch (op->a.type) {
                            case ValueType::I32:
                                BblInsAdd(bbl, InsNew(OPC::BEQ, regs.at(op->a.id), ConstNewU(DK::S32, 0), setOne));
                                break;
                            case ValueType::Double:
                                BblInsAdd(bbl, InsNew(OPC::BEQ, regs.at(op->a.id), ConstNewF(DK::R64, 0), setOne));
                                break;
                            case ValueType::Bool:
                                BblInsAdd(bbl, InsNew(OPC::BEQ, regs.at(op->a.id), ConstNewU(DK::U8, 0), setOne));
                                break;
                            default:
                                throw std::runtime_error("Invalid type");
                        }
                        BblInsAdd(bbl, InsNew(OPC::BRA, setZero));
                        bbl = end;
                    } break;
                    case Opcode::BitNot: {
                        auto reg = createConversion(bbl, fun, op->a, ValueType::I32, regs);
                        BblInsAdd(bbl, InsNew(OPC::XOR, regs.at(op->res.id), reg, ConstNewU(DK::S32, -1)));
                    } break;
                    case Opcode::UnPlus: {
                        auto tmp = createConversion(bbl, fun, op->a, op->res.type, regs);
                        BblInsAdd(bbl, InsNew(OPC::MOV, regs.at(op->res.id), tmp));
                    } break;
                    case Opcode::UnMinus: {
                        Const zero;
                        switch (op->a.type) {
                            case ValueType::I32:
                                zero = ConstNewU(DK::S32, 0);
                                break;
                            case ValueType::Double:
                                zero = ConstNewF(DK::R64, 0);
                                break;
                            default:
                                throw std::runtime_error("Invalid type");
                        }
                        BblInsAdd(bbl, InsNew(OPC::SUB, regs.at(op->res.id), zero, regs.at(op->a.id)));
                    } break;
                    case Opcode::Dup:
                    case Opcode::PushFree:
                        std::cout << "Opcode not implemented: " << static_cast<int>(op->op) << std::endl;
                    default:
                        std::cout << "Opcode not implemented: " << static_cast<int>(op->op) << std::endl;
                }
            }
            else if (auto init = std::get_if<ConstInit>(&statement.op)) {
                struct visitor {
                    Const& cst;
                    void operator()(int32_t value) {
                        cst = ConstNewU(DK::S32, value);
                    }
                    void operator()(double value) {
                        cst = ConstNewF(DK::R64, value);
                    }
                    void operator()(bool value) {
                        cst = ConstNewU(DK::U8, value);
                    }
                    void operator()(const std::string& value) {
                        throw std::runtime_error("Invalid type");
                    }
                };
                addReg(init->id, init->type());
                Const cst;
                std::visit(visitor{ cst }, init->value);
                BblInsAdd(bbl, InsNew(OPC::MOV, regs.at(init->id), cst));
            }
            else if (std::holds_alternative<Call>(statement.op)) {
                throw std::runtime_error("Calls are not supported");
            }
        }
        switch (block->jump.type) {
            case Terminal::None:
                throw std::runtime_error("Invalid jump");
            case Terminal::Branch: {
                assert(block->jump.value->type == ValueType::Bool);
                auto cond = regs.at(block->jump.value->id);
                BblInsAdd(bbl, InsNew(OPC::BEQ, cond, ConstNewU(DK::U8, 0), bbls.at(block->jump.other)));
                BblInsAdd(bbl, InsNew(OPC::BRA, bbls.at(block->jump.target)));
            } break;
            case Terminal::Jump:
                BblInsAdd(bbl, InsNew(OPC::BRA, bbls.at(block->jump.target)));
                break;
            case Terminal::Return:
                BblInsAdd(bbl, InsNew(OPC::RET));
                break;
            case Terminal::ReturnValue: {
                auto retReg = createConversion(bbl, fun, *block->jump.value, cfg.ret, regs);
                BblInsAdd(bbl, InsNew(OPC::PUSHARG, retReg));
                BblInsAdd(bbl, InsNew(OPC::RET));
            } break;
            case Terminal::Throw:
                throw std::runtime_error("Throw is not supported");
        }
    }

    return fun;
}


}  // namespace jac::cfg
