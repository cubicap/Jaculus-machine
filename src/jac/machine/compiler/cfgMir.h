#pragma once

#include "cfg.h"
#include "mirBuiltins.h"
#include "mirUtil.h"
#include "opcode.h"
#include "stackAllocator.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <jac/util.h>
#include <memory>
#include <mir.h>
#include <quickjs.h>
#include <sys/types.h>


namespace jac::cfg::mir_emit {


inline std::string name(TmpId id) {
    return "_" + std::to_string(id);
}


inline auto getJSTag(ValueType type) {
    switch (type) {
        case ValueType::I32:    return JS_TAG_INT;
        case ValueType::Double: return JS_TAG_FLOAT64;
        case ValueType::Bool:   return JS_TAG_BOOL;
        case ValueType::Object: return JS_TAG_OBJECT;
        default:
            throw std::runtime_error("Invalid type");
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

inline MIR_insn_code_t chooseMove(ValueType type) {
    switch (type) {
        case ValueType::I32:  case ValueType::Bool:  case ValueType::Object:
            return MIR_MOV;
        case ValueType::Double:
            return MIR_DMOV;
        default:
            throw std::runtime_error("Set type not implemented");
    }
}

inline MIR_insn_code_t chooseConversion(ValueType from, ValueType to) {
    if (from == to) {
        return chooseMove(from);
    }
    switch (from) {
        case ValueType::I32:
            switch (to) {
                case ValueType::Double:    return MIR_I2D;
                case ValueType::Bool:
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


struct CompileContext {
    MIR_context_t ctx;
    MIR_module_t mod;
    MIR_item_t fun;
    MIR_func_t func;

    std::map<TmpId, MIR_reg_t> regs;  // TODO: deduplicate information in MIR
    std::map<BasicBlockPtr, MIR_label_t> labels;
    std::map<TmpId, int> stackSlots;

    MIR_reg_t allocaPtr;

    CompileContext(MIR_context_t ctx_) : ctx(ctx_) {}

    auto label(BasicBlockPtr block) {
        auto it = labels.find(block);
        if (it == labels.end()) {
            it = labels.emplace(block, MIR_new_label(ctx)).first;
        }
        return it->second;
    }

    auto labelOp(BasicBlockPtr block) {
        return MIR_new_label_op(ctx, label(block));
    }

    auto reg(TmpId id, ValueType type) {
        auto it = regs.find(id);
        if (it == regs.end()) {
            std::string n = name(id);
            auto r = MIR_new_func_reg(ctx, func, getMIRRegType(type), n.c_str());
            it = regs.emplace_hint(it, id, r);
        }

        return it->second;
    }

    auto regOp(TmpId id) {
        return MIR_new_reg_op(ctx, regs.at(id));
    }

    auto scratch(auto prefix, MIR_type_t type) {
        auto r = MIR_new_func_reg(ctx, func, type, (prefix + std::to_string(getId())).c_str());
        return r;
    }

    auto calcOff(MIR_reg_t base, int off, size_t size) {
        auto addr = scratch("_addr", MIR_T_I64);
        MIR_append_insn(ctx, fun, MIR_new_insn(ctx, MIR_ADD, MIR_new_reg_op(ctx, addr), MIR_new_reg_op(ctx, base), MIR_new_int_op(ctx, off * size)));
        return addr;
    }

    auto jsValAddr(TmpId id) -> MIR_reg_t{
        auto it = stackSlots.find(id);
        if (it == stackSlots.end()) {
            return regs.at(id);
        }
        return calcOff(allocaPtr, it->second, sizeof(JSValue));
    };

    auto insert(MIR_insn_t insn) {
        MIR_append_insn(ctx, fun, insn);
    }

    auto copyJSVal(MIR_reg_t srcAddr, MIR_reg_t dstAddr) {
        static_assert(sizeof(JSValue) == 2 * sizeof(int64_t));

        auto srcL = MIR_new_mem_op(ctx, MIR_T_I64, 0,               srcAddr, 0, 0);
        auto srcH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), srcAddr, 0, 0);
        auto dstL = MIR_new_mem_op(ctx, MIR_T_I64, 0,               dstAddr, 0, 0);
        auto dstH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), dstAddr, 0, 0);
        insert(MIR_new_insn(ctx, MIR_MOV, dstL, srcL));
        insert(MIR_new_insn(ctx, MIR_MOV, dstH, srcH));
    }

    auto toJSVal(Temp a, MIR_reg_t dstAddr) {
        auto dstL = MIR_new_mem_op(ctx, getMIRRegType(a.type),   0, dstAddr, 0, 0);
        auto dstH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), dstAddr, 0, 0);
        auto src = MIR_new_reg_op(ctx, reg(a.id, a.type));

        insert(MIR_new_insn(ctx, chooseMove(a.type), dstL, src));
        insert(MIR_new_insn(ctx, MIR_MOV, dstH, MIR_new_int_op(ctx, getJSTag(a.type))));
    }

    auto fromJSVal(MIR_reg_t srcAddr, Temp res) {
        auto srcL = MIR_new_mem_op(ctx, getMIRRegType(res.type), 0, srcAddr, 0, 0);
        auto srcH = MIR_new_mem_op(ctx, MIR_T_I64, sizeof(int64_t), srcAddr, 0, 0);
        auto dst = MIR_new_reg_op(ctx, reg(res.id, res.type));

        insert(MIR_new_insn(ctx, chooseMove(res.type), dst, srcL));
        (void) srcH;  // TODO: check tag value
    }
};

inline MIR_insn_code_t chooseSimpleArithmetic(Opcode op, ValueType type) {
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
        case Opcode::UnMinus: switch (type) {
            case ValueType::I32:    return MIR_NEGS;
            case ValueType::Double: return MIR_DNEG;
            default: assert(false && "Invalid UnMinus operand type");
        }
        default:
            assert(false && "Invalid arithmetic operation");
    }
}

inline const char* chooseArithmeticBuiltin(Opcode op) {
    switch (op) {
        case Opcode::Add:  return "__add";
        case Opcode::Sub:  return "__sub";
        case Opcode::Mul:  return "__mul";
        case Opcode::Div:  return "__div";
        case Opcode::Rem:  return "__rem";
        default:
            assert(false && "Invalid arithmetic operation");
    }
}

inline void generateArithmetic(CompileContext& cc, Opcode op, Temp a, Temp b, Temp res, Builtins& builtins) {
    assert(a.type == b.type && a.type == res.type);
    if (a.type == ValueType::Any) {
        insertCall(cc.ctx, cc.fun, builtins, chooseArithmeticBuiltin(op), { cc.jsValAddr(a.id), cc.jsValAddr(b.id), cc.jsValAddr(res.id) });
    }
    else {
        cc.insert(MIR_new_insn(cc.ctx, chooseSimpleArithmetic(op, a.type), cc.regOp(res.id), cc.regOp(a.id), cc.regOp(b.id)));
    }
}

inline MIR_insn_code_t chooseBitwise(Opcode op, ValueType type) {
    assert(type == ValueType::I32);  // TODO: support doubles
    switch (op) {
        case Opcode::LShift:
            return MIR_LSHS;
        case Opcode::RShift:
            return MIR_RSHS;
        case Opcode::URShift:
            return MIR_URSHS;
        case Opcode::BitAnd:
            return MIR_ANDS;
        case Opcode::BitOr:
            return MIR_ORS;
        case Opcode::BitXor:
            return MIR_XORS;
        default:
            assert(false && "Invalid bitwise operation");
    }
}

inline void createBoolConv(CompileContext cc, Temp a, Temp res, bool inverted) {
    switch (a.type) {
        case ValueType::I32:  case ValueType::Bool:
            cc.insert(MIR_new_insn(cc.ctx, MIR_EQS,
                cc.regOp(res.id), cc.regOp(a.id), MIR_new_int_op(cc.ctx, !inverted)));
            break;
        case ValueType::Double:
            cc.insert(MIR_new_insn(cc.ctx, MIR_DNE,
                cc.regOp(res.id), cc.regOp(a.id), MIR_new_double_op(cc.ctx, !inverted)));
            break;
        default:
            throw std::runtime_error("BoolNot type not implemented");
    }
}


inline bool hasRetArg(Function& cfg) {
    return cfg.ret == ValueType::Any;
}


// output arguments to prevent relocation of the strings
inline void getArgs(Function& cfg, std::vector<std::string>& argNames, std::vector<MIR_var_t>& args) {
    bool retArg = hasRetArg(cfg);

    argNames.reserve(cfg.args.size() + retArg);
    args.reserve(cfg.args.size() + retArg);
    for (const auto& arg : cfg.args) {
        argNames.emplace_back(name(arg.id));
        auto [type, size] = getMIRArgType(arg.type);
        MIR_var_t var = {
            .type = type,
            .name = argNames.back().c_str(),
            .size = size
        };
        args.emplace_back(var);
    }

    if (retArg) {
        argNames.emplace_back("res");
        MIR_var_t var = {
            .type = MIR_T_P,
            .name = argNames.back().c_str()
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
    CompileContext cc(ctx);
    cc.stackSlots = allocateStackSlots(cfg);

    {
        std::vector<std::string> arg_names;
        std::vector<MIR_var_t> args;
        getArgs(cfg, arg_names, args);

        if (cfg.ret == ValueType::Void || hasRetArg(cfg)) {
            cc.fun = MIR_new_func_arr(ctx, cfg.name().c_str(), 0, nullptr, args.size(), args.data());
        }
        else {
            auto retType = getMIRRetType(cfg.ret);
            cc.fun = MIR_new_func_arr(ctx, cfg.name().c_str(), 1, &retType, args.size(), args.data());
        }

        cc.func = MIR_get_item_func(ctx, cc.fun);

        for (size_t i = 0; i < cfg.args.size(); ++i) {
            cc.regs.emplace(cfg.args[i].id, MIR_reg(ctx, args[i].name, cc.func));
        }
    }

    Defer d ([&] {
        MIR_finish_func(ctx);
    });

    cc.allocaPtr = MIR_new_func_reg(ctx, cc.func, MIR_T_I64, "allocaPtr");
    MIR_append_insn(ctx, cc.fun, MIR_new_insn(ctx, MIR_ALLOCA, MIR_new_reg_op(ctx, cc.allocaPtr), MIR_new_int_op(ctx, sizeof(JSValue) * cc.stackSlots.size())));

    insertCall(ctx, cc.fun, builtins, "__enterStackFrame", {});

    auto regOp = [&cc](TmpId id) {
        return cc.regOp(id);
    };

    for (auto& block : cfg.blocks) {
        MIR_append_insn(ctx, cc.fun, cc.label(block.get()));

        for (auto statement : block->statements) {
            if (auto op = std::get_if<Operation>(&statement.op)) {
                if (isRegType(op->res.type)) {
                    cc.reg(op->res.id, op->res.type);
                }
                switch (op->op) {
                    // Binary
                    case Opcode::Add:  case Opcode::Sub:  case Opcode::Mul:
                    case Opcode::Div:  case Opcode::Rem:
                        generateArithmetic(cc, op->op, op->a, op->b, op->res, builtins);
                        break;
                    case Opcode::Pow:
                        throw std::runtime_error("Pow is not supported");
                    case Opcode::LShift:  case Opcode::RShift:  case Opcode::URShift:
                    case Opcode::BitAnd:  case Opcode::BitOr:   case Opcode::BitXor:
                        assert(isIntegral(op->b.type) && op->res.type == op->a.type);
                        cc.insert(MIR_new_insn(ctx, chooseBitwise(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id), regOp(op->b.id)));
                        break;
                    case Opcode::Eq:  case Opcode::Neq:
                    case Opcode::Gt:  case Opcode::Gte:
                    case Opcode::Lt:  case Opcode::Lte:
                        assert(op->a.type == op->b.type && op->res.type == ValueType::Bool);
                        cc.insert(MIR_new_insn(ctx, chooseComparison(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id), regOp(op->b.id)));
                        break;
                    case Opcode::GetMember:
                        assert(op->res.type == ValueType::Any);
                        if (op->a.type == ValueType::Object && op->b.type == ValueType::StringConst) {
                            insertCall(ctx, cc.fun, builtins, "__getMemberObjCStr", { cc.regs.at(op->a.id), cc.regs.at(op->b.id), cc.jsValAddr(op->res.id) });
                        }
                        else if (op->a.type == ValueType::Object && op->b.type == ValueType::I32) {
                            insertCall(ctx, cc.fun, builtins, "__getMemberObjI32", { cc.regs.at(op->a.id), cc.regs.at(op->b.id), cc.jsValAddr(op->res.id) });
                        }
                        else {
                            throw std::runtime_error("GetMember is not fully supported");
                        }
                        break;
                    case Opcode::SetMember: {
                        MIR_reg_t srcAddr;
                        std::optional<MIR_reg_t> startBlock;
                        if (op->b.type == ValueType::Any) {
                            srcAddr = cc.jsValAddr(op->b.id);
                        }
                        else {
                            srcAddr = cc.scratch("_addr", MIR_T_I64);
                            startBlock = cc.scratch("_bstart", MIR_T_I64);
                            cc.insert(MIR_new_insn(ctx, MIR_BSTART, MIR_new_reg_op(ctx, *startBlock)));
                            cc.insert(MIR_new_insn(ctx, MIR_ALLOCA,
                                MIR_new_reg_op(ctx, srcAddr),
                                MIR_new_int_op(ctx, sizeof(JSValue)))
                            );
                            cc.toJSVal(op->b, srcAddr);
                        }
                        if (op->a.type == ValueType::StringConst && op->res.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__setMemberObjCStr", { cc.regs.at(op->res.id), cc.regs.at(op->a.id), srcAddr });
                        }
                        else if (op->a.type == ValueType::I32 && op->res.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__setMemberObjI32", { cc.regs.at(op->res.id), cc.regs.at(op->a.id), srcAddr });
                        }
                        else {
                            throw std::runtime_error("SetMember is not fully supported");
                        }

                        if (startBlock) {
                            cc.insert(MIR_new_insn(ctx, MIR_BEND, MIR_new_reg_op(ctx, *startBlock)));
                        }
                    } break;
                    // Unary
                    case Opcode::Set: {
                        if (op->a.type == ValueType::Any && op->res.type == ValueType::Any) {
                            auto baseSrc = cc.jsValAddr(op->a.id);
                            auto baseDst = cc.jsValAddr(op->res.id);
                            cc.copyJSVal(baseSrc, baseDst);
                            break;
                        }
                        else if (op->a.type == ValueType::Any) {
                            cc.fromJSVal(cc.jsValAddr(op->a.id), op->res);
                            break;
                        }
                        else if (op->res.type == ValueType::Any) {
                            cc.toJSVal(op->a, cc.jsValAddr(op->res.id));
                            break;
                        }
                        else if (op->res.type == ValueType::Bool) {
                            createBoolConv(cc, op->a, op->res, false);
                            break;
                        }
                        cc.insert(MIR_new_insn(ctx, chooseConversion(op->a.type, op->res.type), regOp(op->res.id), regOp(op->a.id)));
                    } break;
                    case Opcode::UnPlus:
                        cc.insert(MIR_new_insn(ctx, chooseConversion(op->a.type, op->res.type), regOp(op->res.id), regOp(op->a.id)));
                        break;
                    case Opcode::BoolNot:
                        createBoolConv(cc, op->a, op->res, true);
                        break;
                    case Opcode::BitNot: {
                        assert(op->a.type == op->res.type && op->a.type == ValueType::I32);
                        cc.insert(MIR_new_insn(ctx, MIR_XORS,
                            regOp(op->res.id), regOp(op->a.id), MIR_new_int_op(ctx, -1)));
                    } break;
                    case Opcode::UnMinus: {
                        assert(op->a.type == op->res.type);
                        cc.insert(MIR_new_insn(ctx, chooseSimpleArithmetic(op->op, op->a.type),
                            regOp(op->res.id), regOp(op->a.id)));
                    } break;
                    case Opcode::Dup:
                        if (op->a.type == ValueType::Any) {
                            insertCall(ctx, cc.fun, builtins, "__dupVal", { cc.jsValAddr(op->a.id) });
                        }
                        else if (op->a.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__dupObj", { cc.regs.at(op->a.id) });
                        }
                        else if (!isNumeric(op->a.type)) {
                            throw std::runtime_error("Dup is not fully supported");
                        }
                        break;
                    case Opcode::PushFree:
                        if (op->a.type == ValueType::Any) {
                            insertCall(ctx, cc.fun, builtins, "__pushFreeVal", { cc.jsValAddr(op->a.id) });
                        }
                        else if (op->a.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__pushFreeObj", { cc.regs.at(op->a.id) });
                        }
                        else if (!isNumeric(op->a.type) && op->a.type != ValueType::StringConst) {
                            throw std::runtime_error("PushFree is not fully supported");
                        }
                        break;
                    default:
                        std::cout << "Opcode not implemented: " << static_cast<int>(op->op) << std::endl;
                }
            }
            else if (auto init = std::get_if<ConstInit>(&statement.op)) {
                auto reg = MIR_new_reg_op(ctx, cc.reg(init->id, init->type()));
                std::visit([&](auto val) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(val)>, int32_t>) {
                        auto vOp = MIR_new_int_op(ctx, val);
                        cc.insert(MIR_new_insn(ctx, MIR_MOV, reg, vOp));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, double>) {
                        auto vOp = MIR_new_double_op(ctx, val);
                        cc.insert(MIR_new_insn(ctx, MIR_DMOV, reg, vOp));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, bool>) {
                        auto vOp = MIR_new_int_op(ctx, val);
                        cc.insert(MIR_new_insn(ctx, MIR_MOV, reg, vOp));
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                        auto strDup = std::make_unique<char[]>(val.size() + 1);
                        std::copy(val.begin(), val.end(), strDup.get());
                        strDup[val.size()] = '\0';
                        auto vOp = MIR_new_int_op(ctx, reinterpret_cast<uint64_t>(strDup.get()));  // NOLINT
                        cc.insert(MIR_new_insn(ctx, MIR_MOV, reg, vOp));
                        builtins.rtCtx->stringConsts.emplace_back(std::move(strDup));
                    }
                }, init->value);
            }
            else if (auto call = std::get_if<Call>(&statement.op)) {
                std::vector<MIR_op_t> args;
                if (call->isNative()) {
                    if (call->res.type != ValueType::Void && call->res.type != ValueType::Any) {
                        cc.reg(call->res.id, call->res.type);
                    }
                    auto [proto, forward] = prototypes.at(std::get<Identifier>(call->obj));
                    args.push_back(MIR_new_ref_op(ctx, proto));
                    args.push_back(MIR_new_ref_op(ctx, forward));
                    if (call->res.type != ValueType::Void && call->res.type != ValueType::Any) {
                        args.push_back(regOp(call->res.id));
                    }
                    for (auto& arg : call->args) {
                        auto [type, size] = getMIRArgType(arg.type);
                        if (MIR_blk_type_p(type)) {
                            args.push_back(MIR_new_mem_op(ctx, type, size, cc.jsValAddr(arg.id), 0, 0));
                        }
                        else {
                            args.push_back(regOp(arg.id));
                    }
                    }
                    if (call->res.type == ValueType::Any) {
                        args.push_back(MIR_new_reg_op(ctx, cc.jsValAddr(call->res.id)));
                    }
                    cc.insert(MIR_new_insn_arr(ctx, MIR_CALL, args.size(), args.data()));
                }
                else {
                    assert(call->args.size() >= 1);

                    auto startBlock = cc.scratch("_bstart", MIR_T_I64);
                    cc.insert(MIR_new_insn(ctx, MIR_BSTART, MIR_new_reg_op(ctx, startBlock)));

                    auto argsStart = cc.scratch("_args", MIR_T_I64);
                    auto argCount = cc.scratch("_argCount", MIR_T_I64);
                    cc.insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_reg_op(ctx, argCount), MIR_new_int_op(ctx, call->args.size() - 1)));
                    cc.insert(MIR_new_insn(ctx, MIR_ALLOCA,
                        MIR_new_reg_op(ctx, argsStart),
                        MIR_new_int_op(ctx, sizeof(JSValue) * std::min(call->args.size() - 1, static_cast<size_t>(1))))
                    );
                    for (size_t i = 1; i < call->args.size(); ++i) {
                        auto arg = call->args[i];
                        auto addr = cc.calcOff(argsStart, i - 1, sizeof(JSValue));
                        cc.toJSVal(arg, addr);
                    }

                    auto funObj = std::get<Temp>(call->obj);
                    auto thisArg = call->args[0];
                    if (funObj.type == ValueType::Any) {
                        auto funReg = cc.jsValAddr(funObj.id);
                        if (thisArg.type == ValueType::Any) {
                            insertCall(ctx, cc.fun, builtins, "__callAnyAny", { funReg, cc.jsValAddr(thisArg.id), argCount, argsStart });
                        }
                        else if (thisArg.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__callAnyObj", { funReg, cc.regs.at(thisArg.id), argCount, argsStart });
                        }
                        else if (thisArg.type == ValueType::Void) {
                            insertCall(ctx, cc.fun, builtins, "__callAnyUndefined", { funReg, argCount, argsStart });
                        }
                        else {
                            throw std::runtime_error("Invalid call operand type");
                        }
                    }
                    else if (funObj.type == ValueType::Object) {
                        auto funReg = cc.regs.at(funObj.id);
                        if (thisArg.type == ValueType::Any) {
                            insertCall(ctx, cc.fun, builtins, "__callObjAny", { funReg, cc.jsValAddr(thisArg.id), argCount, argsStart });
                        }
                        else if (thisArg.type == ValueType::Object) {
                            insertCall(ctx, cc.fun, builtins, "__callObjObj", { funReg, cc.regs.at(thisArg.id), argCount, argsStart });
                        }
                        else if (thisArg.type == ValueType::Void) {
                            insertCall(ctx, cc.fun, builtins, "__callObjUndefined", { funReg, argCount, argsStart });
                        }
                        else {
                            throw std::runtime_error("Invalid call operand type");
                        }
                    }
                    else {
                        throw std::runtime_error("Invalid call operand type");
                    }

                    cc.copyJSVal(argsStart, cc.jsValAddr(call->res.id));

                    cc.insert(MIR_new_insn(ctx, MIR_BEND, MIR_new_reg_op(ctx, startBlock)));
                }
            }
        }

        switch (block->jump.type) {
            case Terminal::None:
                throw std::runtime_error("Invalid jump");
            case Terminal::Branch: {
                auto cond = block->jump.value;
                assert(cond->type == ValueType::Bool);
                cc.insert(MIR_new_insn(ctx, MIR_BTS, cc.labelOp(block->jump.target), regOp(cond->id)));
                cc.insert(MIR_new_insn(ctx, MIR_JMP, cc.labelOp(block->jump.other)));
            } break;
            case Terminal::Jump: {
                cc.insert(MIR_new_insn(ctx, MIR_JMP, cc.labelOp(block->jump.target)));
            } break;
            case Terminal::Return:
                insertCall(ctx, cc.fun, builtins, "__exitStackFrame", {});
                cc.insert(MIR_new_ret_insn(ctx, 0));
                break;
            case Terminal::ReturnValue:
                insertCall(ctx, cc.fun, builtins, "__exitStackFrame", {});
                if (hasRetArg(cfg)) {
                    auto res = block->jump.value;
                    assert(res->type == cfg.ret);
                    auto addr = cc.jsValAddr(res->id);
                    auto resReg = MIR_reg(ctx, "res", cc.func);
                    cc.copyJSVal(addr, resReg);
                    cc.insert(MIR_new_ret_insn(ctx, 0));
                }
                else {
                    cc.insert(MIR_new_ret_insn(ctx, 1, regOp(block->jump.value->id)));
                }
                break;
            case Terminal::Throw:
                throw std::runtime_error("Throw is not supported");
            default:
                std::cout << "Jump type not implemented: " << static_cast<int>(block->jump.type) << std::endl;
        }
    }

    return cc.fun;
}


inline MIR_item_t compileCaller(MIR_context_t ctx, Function& cfg, MIR_item_t fun, MIR_item_t proto) {
    std::string callerName = "_caller_" + cfg.name();

    std::vector<MIR_var_t> args {
        { .type = MIR_T_I64, .name = "argc" },
        { .type = MIR_T_P, .name = "argv" }
    };

    args.push_back({ .type = MIR_T_P, .name = "res" });

    MIR_item_t caller;
    caller = MIR_new_func_arr(ctx, callerName.c_str(), 0, nullptr, args.size(), args.data());
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
            case ValueType::I32:  case ValueType::Bool:  case ValueType::Object: {
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

    MIR_reg_t resReg = MIR_reg(ctx, "res", callerFunc);
    MIR_reg_t calleeRes;

    std::vector<MIR_op_t> callArgs;
    callArgs.push_back(MIR_new_ref_op(ctx, proto));
    callArgs.push_back(MIR_new_ref_op(ctx, fun));
    if (cfg.ret != ValueType::Void && !hasRetArg(cfg)) {
        std::string name = "calleeRes";
        calleeRes = MIR_new_func_reg(ctx, callerFunc, getMIRRegType(cfg.ret), name.c_str());
        callArgs.push_back(MIR_new_reg_op(ctx, calleeRes));
    }
    for (auto& op : argOps) {
        callArgs.push_back(op);
    }
    if (hasRetArg(cfg)) {
        callArgs.push_back(MIR_new_reg_op(ctx, resReg));
    }
    insert(MIR_new_insn_arr(ctx, MIR_CALL, callArgs.size(), callArgs.data()));

    // perform conversion from resReg to JSValue in *res

    static constexpr size_t TAG_DISP = offsetof(JSValue, tag);
    static constexpr size_t VAL_DISP = offsetof(JSValue, u);
    auto tagOp = MIR_new_mem_op(ctx, MIR_T_I64, TAG_DISP, resReg, 0, 0);
    switch (cfg.ret) {
        case ValueType::I32:
            insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_INT)));
            insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_mem_op(ctx, MIR_T_I64, VAL_DISP, resReg, 0, 0), MIR_new_reg_op(ctx, calleeRes)));
            break;
        case ValueType::Bool:
            insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_BOOL)));
            insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_mem_op(ctx, MIR_T_I64, VAL_DISP, resReg, 0, 0), MIR_new_reg_op(ctx, calleeRes)));
            break;
        case ValueType::Object:
            insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_OBJECT)));
            insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_mem_op(ctx, MIR_T_I64, VAL_DISP, resReg, 0, 0), MIR_new_reg_op(ctx, calleeRes)));
            break;
        case ValueType::Double:
            insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_FLOAT64)));
            insert(MIR_new_insn(ctx, MIR_DMOV, MIR_new_mem_op(ctx, MIR_T_D, VAL_DISP, resReg, 0, 0), MIR_new_reg_op(ctx, calleeRes)));
            break;
        case ValueType::Void:
            insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_UNDEFINED)));
            insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_mem_op(ctx, MIR_T_I64, VAL_DISP, resReg, 0, 0), MIR_new_int_op(ctx, 0)));
            break;
        case ValueType::Any:
            break;  // return is performed by callee
        default:
            throw std::runtime_error("Invalid return type");
    }
    insert(MIR_new_ret_insn(ctx, 0));

    insert(fail);

    insert(MIR_new_insn(ctx, MIR_MOV, tagOp, MIR_new_int_op(ctx, JS_TAG_UNDEFINED)));
    insert(MIR_new_insn(ctx, MIR_MOV, MIR_new_mem_op(ctx, MIR_T_I64, VAL_DISP, resReg, 0, 0), MIR_new_int_op(ctx, 0)));
    insert(MIR_new_ret_insn(ctx, 0));

    MIR_finish_func(ctx);

    return caller;
}


}  // namespace jac::cfg::mir_emit
