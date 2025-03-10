#pragma once

#include "cfg.h"
#include "stackAllocator.h"
#include "mirUtil.h"
#include "mirBuiltins.h"
#include "opcode.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <jac/util.h>
#include <mir.h>
#include <quickjs.h>


namespace jac::cfg::mir_emit {


inline std::pair<MIR_type_t, size_t> getMIRArgType(ValueType type) {
    switch (type) {
        case ValueType::I32:
            return { MIR_T_I32, 0 };
        case ValueType::Double:
            return { MIR_T_D, 0 };
        case ValueType::Bool:
            return { MIR_T_I32, 0 };
        case ValueType::Object:
            return { MIR_T_P, 0 };
        case ValueType::String:
            return { MIR_T_P, 0 };
        case ValueType::Any:
            return { MIR_type_t(MIR_T_BLK + 1), sizeof(JSValue) };
        default:
            throw std::runtime_error("Invalid MIR type");
    }
}

inline MIR_type_t getMIRRetType(ValueType type) {
    auto [mirType, size] = getMIRArgType(type);
    assert(!MIR_all_blk_type_p(mirType));
    return mirType;
}

inline MIR_type_t getMIRRegType(ValueType type) {
    if (isIntegral(type)) {
        return MIR_T_I64;
    }
    if (type == ValueType::Double) {
        return MIR_T_D;
    }
    throw std::runtime_error("Invalid reg type");
}

inline std::string name(TmpId id) {
    return "_" + std::to_string(id);
}

inline MIR_insn_code_t chooseArithmetic(Opcode op, ValueType type) {
    switch (op) {
        case Opcode::Add:  switch (type) {  // TODO: support strings
            case ValueType::I32:    return MIR_ADDS;
            case ValueType::Double: return MIR_DADD;
            default: assert(false && "Invalid Add operand type");
        }
        case Opcode::Sub:  switch (type) {
            case ValueType::I32:    return MIR_SUBS;
            case ValueType::Double: return MIR_DSUB;
            default: assert(false && "Invalid Sub operand type");
        }
        case Opcode::Mul:  switch (type) {
            case ValueType::I32:    return MIR_MULS;
            case ValueType::Double: return MIR_DMUL;
            default: assert(false && "Invalid Mul operand type");
        }
        case Opcode::Div:  switch (type) {
            case ValueType::I32:    return MIR_DIVS;
            case ValueType::Double: return MIR_DDIV;
            default: assert(false && "Invalid Div operand type");
        }
        case Opcode::Rem:  switch (type) {
            case ValueType::I32:    return MIR_MODS;
            case ValueType::Double: throw std::runtime_error("fmod is not supported");
            default: assert(false && "Invalid Rem operand type");
        }
        case Opcode::Pow: throw std::runtime_error("Pow is not supported");
        case Opcode::UnMinus: switch (type) {
            case ValueType::I32:    return MIR_NEGS;
            case ValueType::Double: return MIR_DNEG;
            default: assert(false && "Invalid UnMinus operand type");
        }
        default:
            assert(false && "Invalid arithmetic operation");
    }
}

inline MIR_insn_code_t chooseBitwise(Opcode op, ValueType type) {
    assert(isIntegral(type));  // TODO: support doubles
    switch (op) {
        case Opcode::LShift: switch (type) {
            case ValueType::I32:    return MIR_LSHS;
            case ValueType::Bool:   return MIR_LSHS;
            default: assert(false && "Invalid LShift operand type");
        }
        case Opcode::RShift: switch (type) {
            case ValueType::I32:    return MIR_RSHS;
            case ValueType::Bool:   return MIR_RSHS;
            default: assert(false && "Invalid RShift operand type");
        }
        case Opcode::URShift: switch (type) {
            case ValueType::I32:    return MIR_URSHS;
            case ValueType::Bool:   return MIR_URSHS;
            default: assert(false && "Invalid URShift operand type");
        }
        case Opcode::BitAnd: switch (type) {
            case ValueType::I32:    return MIR_ANDS;
            case ValueType::Bool:   return MIR_ANDS;
            default: assert(false && "Invalid BitAnd operand type");
        }
        case Opcode::BitOr: switch (type) {
            case ValueType::I32:    return MIR_ORS;
            case ValueType::Bool:   return MIR_ORS;
            default: assert(false && "Invalid BitOr operand type");
        }
        case Opcode::BitXor:  switch (type) {
            case ValueType::I32:    return MIR_XORS;
            case ValueType::Bool:   return MIR_XORS;
            default: assert(false && "Invalid BitXor operand type");
        }
        default:
            assert(false && "Invalid bitwise operation");
    }
}

inline MIR_insn_code_t chooseComparison(Opcode op, ValueType type) {
    assert(type != ValueType::Void);
    switch (op) {
        case Opcode::Eq:  switch (type) {
            case ValueType::I32:    return MIR_EQS;
            case ValueType::Double: return MIR_DEQ;
            case ValueType::Bool:   return MIR_EQS;
            default: assert(false && "Invalid Eq operand type");
        }
        case Opcode::Neq:  switch (type) {
            case ValueType::I32:    return MIR_NES;
            case ValueType::Double: return MIR_DNE;
            case ValueType::Bool:   return MIR_NES;
            default: assert(false && "Invalid Neq operand type");
        }
        case Opcode::Gt:  switch (type) {
            case ValueType::I32:    return MIR_GTS;
            case ValueType::Double: return MIR_DGT;
            default: assert(false && "Invalid Gt operand type");
        }
        case Opcode::Gte:  switch (type) {
            case ValueType::I32:    return MIR_GES;
            case ValueType::Double: return MIR_DGE;
            default: assert(false && "Invalid Gte operand type");
        }
        case Opcode::Lt:  switch (type) {
            case ValueType::I32:    return MIR_LTS;
            case ValueType::Double: return MIR_DLT;
            default: assert(false && "Invalid Lt operand type");
        }
        case Opcode::Lte:  switch (type) {
            case ValueType::I32:    return MIR_LES;
            case ValueType::Double: return MIR_DLE;
            default: assert(false && "Invalid Lte operand type");
        }
        default:
            assert(false && "Invalid comparison operation");
    }
}

inline MIR_insn_code_t chooseConversion(ValueType from, ValueType to) {
    if (from == to) {
        switch (to) {
            case ValueType::I32:  case ValueType::Bool:
            return MIR_MOV;
            case ValueType::Double:
            return MIR_DMOV;
            default:
            throw std::runtime_error("Set type not implemented");
        }
    }
    switch (from) {
        case ValueType::I32:
            switch (to) {
                case ValueType::Double:    return MIR_I2D;
                default: throw std::runtime_error("Conversion not implemented");
            }
        case ValueType::Double:
            switch (to) {
                case ValueType::I32:    return MIR_D2I;
                default: throw std::runtime_error("Conversion not implemented");
            }
        case ValueType::Bool:
            switch (to) {
                case ValueType::I32:    return MIR_MOV;
                case ValueType::Double: return MIR_I2D;
                default: throw std::runtime_error("Conversion not implemented");
            }
        default:
            throw std::runtime_error("Conversion not implemented");
    }
}


inline bool hasRetArg(Function& cfg) {
    return cfg.ret == ValueType::Any;
}


// output arguments to prevent relocation of the strings
inline void getArgs(Function& cfg, std::vector<std::string>& arg_names, std::vector<MIR_var_t>& args) {
    bool retArg = hasRetArg(cfg);

    arg_names.reserve(cfg.args.size() + retArg);
    args.reserve(cfg.args.size() + retArg);
    for (const auto& arg : cfg.args) {
        arg_names.emplace_back(name(arg.id));
        auto [type, size] = getMIRArgType(arg.type);
        MIR_var_t var = {
            .type = type,
            .name = arg_names.back().c_str(),
            .size = size
        };
        args.emplace_back(var);
    }

    if (retArg) {
        arg_names.emplace_back("res");
        MIR_var_t var = {
            .type = MIR_T_P,
            .name = arg_names.back().c_str()
        };
        args.emplace_back(var);
    }
}


inline MIR_item_t getPrototype(MIR_context_t ctx, Function& cfg) {
    std::vector<std::string> arg_names;
    std::vector<MIR_var_t> args;
    getArgs(cfg, arg_names, args);

    std::string name = "_proto_" + cfg.name();

    if (cfg.ret == ValueType::Void || hasRetArg(cfg)) {
        return MIR_new_proto_arr(ctx, name.c_str(), 0, nullptr, args.size(), args.data());
    }
    auto retType = getMIRRetType(cfg.ret);
    return MIR_new_proto_arr(ctx, name.c_str(), 1, &retType, args.size(), args.data());
}


inline bool isRegType(ValueType type) {
    return type != ValueType::Any && type != ValueType::Void;
}


inline MIR_item_t compile(MIR_context_t ctx, const std::map<std::string, std::pair<MIR_item_t, MIR_item_t>>& prototypes, Function& cfg, Builtins& builtins) {
    std::map<TmpId, MIR_reg_t> regs;  // TODO: deduplicate information in MIR
    std::map<BasicBlockPtr, MIR_label_t> labels;
    std::map<TmpId, int> stackSlots = allocateStackSlots(cfg);

    MIR_item_t fun;
    MIR_func_t func;

    {
        std::vector<std::string> arg_names;
        std::vector<MIR_var_t> args;
        getArgs(cfg, arg_names, args);

        if (cfg.ret == ValueType::Void || hasRetArg(cfg)) {
            fun = MIR_new_func_arr(ctx, cfg.name().c_str(), 0, nullptr, args.size(), args.data());
        }
        else {
            auto retType = getMIRRetType(cfg.ret);
            fun = MIR_new_func_arr(ctx, cfg.name().c_str(), 1, &retType, args.size(), args.data());
        }

        func = MIR_get_item_func(ctx, fun);

        for (size_t i = 0; i < cfg.args.size(); ++i) {
            regs.emplace(cfg.args[i].id, MIR_reg(ctx, args[i].name, func));
        }
    }

    Defer d ([&] {
        MIR_finish_func(ctx);
    });

    MIR_reg_t allocaPtr = MIR_new_func_reg(ctx, func, MIR_T_I64, "allocaPtr");
    MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ALLOCA, MIR_new_reg_op(ctx, allocaPtr), MIR_new_int_op(ctx, sizeof(JSValue) * stackSlots.size())));

    auto label = [&](BasicBlockPtr block) {
        auto it = labels.find(block);
        if (it == labels.end()) {
            it = labels.emplace(block, MIR_new_label(ctx)).first;
        }
        return it->second;
    };

    auto labelOp = [&](BasicBlockPtr block) {
        return MIR_new_label_op(ctx, label(block));
    };

    auto regOp = [&](TmpId id, ValueType type = ValueType::I32) {  // TODO: support complex l-values
        auto it = regs.find(id);
        if (it == regs.end()) {
            std::string n = name(id);
            auto r = MIR_new_func_reg(ctx, func, getMIRRegType(type), n.c_str());
            it = regs.emplace_hint(it, id, r);
        }

        return MIR_new_reg_op(ctx, it->second);
    };

    auto calcOff = [&](MIR_reg_t base, int off, size_t size) {
        auto addr = MIR_new_func_reg(ctx, func, MIR_T_I64, ("_addr" + std::to_string(getId())).c_str());
        MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, addr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, off * size)));
        return addr;
    };

    auto jsValAddr = [&](TmpId id) -> MIR_reg_t{
        auto it = stackSlots.find(id);
        if (it == stackSlots.end()) {
            return regs.at(id);
        }
        return calcOff(allocaPtr, it->second, sizeof(JSValue));
    };

    auto insert = [&](MIR_insn_t insn) {
        MIR_append_insn(ctx, fun, insn);
    };

    auto copyJSVal = [&](MIR_reg_t srcAddr, MIR_reg_t dstAddr) {
        static_assert(sizeof(JSValue) == 2 * sizeof(int64_t));

        auto srcL = MIR_new_mem_op(ctx, MIR_T_I64, 0,               srcAddr, 0, 0);
        auto srcH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), srcAddr, 0, 0);
        auto dstL = MIR_new_mem_op(ctx, MIR_T_I64, 0,               dstAddr, 0, 0);
        auto dstH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), dstAddr, 0, 0);
        insert(MIR_new_insn(ctx, MIR_MOV, dstL, srcL));
        insert(MIR_new_insn(ctx, MIR_MOV, dstH, srcH));
    };

    for (auto& block : cfg.blocks) {
        MIR_append_insn(ctx, fun, label(block.get()));

        for (auto statement : block->statements) {
            if (auto op = std::get_if<Operation>(&statement.op)) {
                if (isRegType(op->res.type)) {
                    regOp(op->res.id, op->res.type);
                }
                switch (op->op) {
                    // Binary
                    case Opcode::Add:  case Opcode::Sub:  case Opcode::Mul:
                    case Opcode::Div:  case Opcode::Rem:  case Opcode::Pow:
                        assert(op->a.type == op->b.type && op->res.type == op->a.type);
                        insert(MIR_new_insn(ctx, chooseArithmetic(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id), regOp(op->b.id)));
                        break;
                    case Opcode::LShift:  case Opcode::RShift:  case Opcode::URShift:
                    case Opcode::BitAnd:  case Opcode::BitOr:   case Opcode::BitXor:
                        assert(isIntegral(op->b.type) && op->res.type == op->a.type);
                        insert(MIR_new_insn(ctx, chooseBitwise(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id), regOp(op->b.id)));
                        break;
                    case Opcode::Eq:  case Opcode::Neq:
                    case Opcode::Gt:  case Opcode::Gte:
                    case Opcode::Lt:  case Opcode::Lte:
                        assert(op->a.type == op->b.type && op->res.type == ValueType::Bool);
                        insert(MIR_new_insn(ctx, chooseComparison(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id), regOp(op->b.id)));
                        break;
                    case Opcode::GetMember:
                        throw std::runtime_error("GetMember is not supported");

                    // Unary
                    case Opcode::Set: {
                        if (op->a.type == ValueType::Any && op->res.type == ValueType::Any) {
                            auto baseSrc = jsValAddr(op->a.id);
                            auto baseDst = jsValAddr(op->res.id);
                            copyJSVal(baseSrc, baseDst);
                            break;
                        }
                        else if (op->a.type == ValueType::Any) {
                            throw std::runtime_error("Set Any to non-Any is not supported");
                        }
                        else if (op->res.type == ValueType::Any) {
                            throw std::runtime_error("Set non-Any to Any is not supported");
                        }
                        insert(MIR_new_insn(ctx, chooseConversion(op->a.type, op->res.type), regOp(op->res.id), regOp(op->a.id)));
                    } break;
                    case Opcode::UnPlus:
                        insert(MIR_new_insn(ctx, chooseConversion(op->a.type, op->res.type), regOp(op->res.id), regOp(op->a.id)));
                        break;
                    case Opcode::BoolNot: {
                        switch (op->a.type) {
                            case ValueType::I32:  case ValueType::Bool:
                                insert(MIR_new_insn(ctx, MIR_EQS,
                                    regOp(op->res.id), regOp(op->a.id), MIR_new_int_op(ctx, 0)));
                                break;
                            case ValueType::Double:
                                insert(MIR_new_insn(ctx, MIR_DNE,
                                    regOp(op->res.id), regOp(op->a.id), MIR_new_double_op(ctx, 0)));
                                break;
                            default:
                                throw std::runtime_error("BoolNot type not implemented");
                        }
                    } break;
                    case Opcode::BitNot: {
                        assert(op->a.type == op->res.type && op->a.type == ValueType::I32);
                        insert(MIR_new_insn(ctx, MIR_XORS,
                            regOp(op->res.id), regOp(op->a.id), MIR_new_int_op(ctx, -1)));
                    } break;
                    case Opcode::UnMinus: {
                        assert(op->a.type == op->res.type);
                        insert(MIR_new_insn(ctx, chooseArithmetic(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id)));
                    } break;
                    case Opcode::Dup:
                        if (op->a.type == ValueType::Any) {
                            insertCallJV(ctx, fun, builtins.jvProto, builtins.rtCtx, builtins.dup, jsValAddr(op->a.id), 0);
                        }
                        else if (!isNumeric(op->a.type)) {
                            throw std::runtime_error("Dup is not fully supported");
                        }
                        break;
                    case Opcode::PushFree:
                        if (op->a.type == ValueType::Any) {
                            insertCallJV(ctx, fun, builtins.jvProto, builtins.rtCtx,builtins.free, jsValAddr(op->a.id), 0);  // FIXME: not immediately - push to stack
                        }
                        else if (!isNumeric(op->a.type)) {
                            throw std::runtime_error("PushFree is not fully supported");
                        }
                        break;
                    default:
                        std::cout << "Opcode not implemented: " << static_cast<int>(op->op) << std::endl;
                }
            }
            else if (auto init = std::get_if<ConstInit>(&statement.op)) {
                auto reg = regOp(init->id, init->type());
                std::visit([&](auto val) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(val)>, int32_t>) {
                        auto op = MIR_new_int_op(ctx, val);
                        insert(MIR_new_insn(ctx, MIR_MOV, reg, op));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, double>) {
                        auto op = MIR_new_double_op(ctx, val);
                        insert(MIR_new_insn(ctx, MIR_DMOV, reg, op));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, bool>) {
                        auto op = MIR_new_int_op(ctx, val);
                        insert(MIR_new_insn(ctx, MIR_MOV, reg, op));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                        throw std::runtime_error("std::string constants are not supported");
                    }
                }, init->value);
            }
            else if (auto call = std::get_if<Call>(&statement.op)) {
                std::vector<MIR_op_t> args;
                if (!call->isNative()) {
                    throw std::runtime_error("Object calls are not supported");
                }
                auto [proto, forward] = prototypes.at(std::get<Identifier>(call->obj));
                args.push_back(MIR_new_ref_op(ctx, proto));
                args.push_back(MIR_new_ref_op(ctx, forward));
                if (call->res.type != ValueType::Void) {
                    args.push_back(regOp(call->res.id));
                }
                for (auto& arg : call->args) {
                    args.push_back(regOp(arg.id));
                }
                insert(MIR_new_insn_arr(ctx, MIR_CALL, args.size(), args.data()));
            }
        }

        switch (block->jump.type) {
            case Terminal::None:
                throw std::runtime_error("Invalid jump");
            case Terminal::Branch: {
                auto cond = block->jump.value;
                assert(cond->type == ValueType::Bool);
                insert(MIR_new_insn(ctx, MIR_BTS, labelOp(block->jump.target), regOp(cond->id)));
                insert(MIR_new_insn(ctx, MIR_JMP, labelOp(block->jump.other)));
            } break;
            case Terminal::Jump: {
                insert(MIR_new_insn(ctx, MIR_JMP, labelOp(block->jump.target)));
            } break;
            case Terminal::Return:
                insert(MIR_new_ret_insn(ctx, 0));
                break;
            case Terminal::ReturnValue:
                if (hasRetArg(cfg)) {
                    auto res = block->jump.value;
                    assert(res->type == cfg.ret);
                    auto addr = jsValAddr(res->id);
                    auto resReg = MIR_reg(ctx, "res", func);
                    copyJSVal(addr, resReg);
                    insert(MIR_new_ret_insn(ctx, 0));
                }
                else {
                    insert(MIR_new_ret_insn(ctx, 1, regOp(block->jump.value->id)));
                }
                break;
            case Terminal::Throw:
                throw std::runtime_error("Throw is not supported");
            default:
                std::cout << "Jump type not implemented: " << static_cast<int>(block->jump.type) << std::endl;
        }
    }

    return fun;
}


inline MIR_item_t compileCaller(MIR_context_t ctx, Function& cfg, MIR_item_t fun, MIR_item_t proto) {
    std::string callerName = "_caller_" + cfg.name();

    std::vector<MIR_var_t> args {
        { .type = MIR_T_I64, .name = "argc" },
        { .type = MIR_T_P, .name = "argv" }
    };

    if (hasRetArg(cfg)) {
        args.push_back({ .type = MIR_T_P, .name = "res" });
    }

    MIR_item_t caller;
    if (cfg.ret == ValueType::Void || hasRetArg(cfg)) {
        caller = MIR_new_func_arr(ctx, callerName.c_str(), 0, nullptr, args.size(), args.data());
    }
    else {
        auto retType = getMIRRetType(cfg.ret);
        caller = MIR_new_func_arr(ctx, callerName.c_str(), 1, &retType, args.size(), args.data());
    }
    auto callerFunc = MIR_get_item_func(ctx, caller);

    auto insert = [&](MIR_insn_t insn) {
        MIR_append_insn(ctx, caller, insn);
    };

    MIR_label_t entry = MIR_new_label(ctx);
    insert(entry);
    MIR_label_t fail = MIR_new_label(ctx);

    std::vector<MIR_op_t> argOps;
    MIR_reg_t argcReg = MIR_reg(ctx, "argc", callerFunc);
    MIR_reg_t argvReg = MIR_reg(ctx, "argv", callerFunc);
    MIR_reg_t posReg;
    {
        std::string name = "pos";
        posReg = MIR_new_func_reg(ctx, callerFunc, MIR_T_I64, name.c_str());
    }

    insert(MIR_new_insn(ctx, MIR_BGT, MIR_new_label_op(ctx, fail), MIR_new_reg_op(ctx, argcReg), MIR_new_int_op(ctx, cfg.args.size())));

    argOps.reserve(cfg.args.size());
    for (size_t i = 0; i < cfg.args.size(); ++i) {
        auto& arg = cfg.args[i];

        std::string name = "_arg" + std::to_string(i);
        insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_reg_op(ctx, posReg), MIR_new_int_op(ctx, i)));

        static constexpr size_t argOffset = sizeof(JSValue);

        switch (arg.type) {
            case ValueType::I32:
            case ValueType::Bool: {
                auto reg = MIR_new_func_reg(ctx, callerFunc, getMIRRegType(arg.type), name.c_str());
                insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_reg_op(ctx, reg), MIR_new_mem_op(ctx, getMIRRegType(arg.type), 0, argvReg, posReg, argOffset)));
                argOps.push_back(MIR_new_reg_op(ctx, reg));
            } break;
            case ValueType::Double: {
                auto reg = MIR_new_func_reg(ctx, callerFunc, getMIRRegType(arg.type), name.c_str());
                insert(MIR_new_insn(ctx, MIR_DMOV, MIR_new_reg_op(ctx, reg), MIR_new_mem_op(ctx, getMIRRegType(arg.type), 0, argvReg, posReg, argOffset)));
                argOps.push_back(MIR_new_reg_op(ctx, reg));
            } break;
            case ValueType::Any: {
                auto addr = MIR_new_func_reg(ctx, callerFunc, MIR_T_I64, name.c_str());
                insert(MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, addr), MIR_new_reg_op(ctx, argvReg), MIR_new_int_op(ctx, argOffset * i)));
                auto [type, size] = getMIRArgType(ValueType::Any);
                auto op = MIR_new_mem_op(ctx, type, size, addr, 0, 0);
                argOps.push_back(op);
            } break;
            default:
                throw std::runtime_error("Invalid arg type");
        }
    }

    std::vector<MIR_op_t> callArgs;
    callArgs.push_back(MIR_new_ref_op(ctx, proto));
    callArgs.push_back(MIR_new_ref_op(ctx, fun));
    MIR_reg_t resReg;
    if (cfg.ret != ValueType::Void && !hasRetArg(cfg)) {
        std::string name = "res";
        resReg = MIR_new_func_reg(ctx, callerFunc, getMIRRegType(cfg.ret), name.c_str());
        callArgs.push_back(MIR_new_reg_op(ctx, resReg));
    }
    for (auto& op : argOps) {
        callArgs.push_back(op);
    }
    if (hasRetArg(cfg)) {
        callArgs.push_back(MIR_new_reg_op(ctx, MIR_reg(ctx, "res", callerFunc)));
    }
    insert(MIR_new_insn_arr(ctx, MIR_CALL, callArgs.size(), callArgs.data()));

    if (cfg.ret != ValueType::Void && !hasRetArg(cfg)) {
        insert(MIR_new_ret_insn(ctx, 1, MIR_new_reg_op(ctx, resReg)));
    }

    insert(fail);
    switch (cfg.ret) {
        case ValueType::I32:
        case ValueType::Bool:
            insert(MIR_new_ret_insn(ctx, 1, MIR_new_int_op(ctx, 0)));
            break;
        case ValueType::Double:
            insert(MIR_new_ret_insn(ctx, 1, MIR_new_double_op(ctx, 0)));
            break;
        case ValueType::Void:
        case ValueType::Any:
            insert(MIR_new_ret_insn(ctx, 0));
            break;
        default:
            throw std::runtime_error("Invalid return type");
    }

    MIR_finish_func(ctx);

    return caller;
}


}  // namespace jac::cfg::mir_emit
