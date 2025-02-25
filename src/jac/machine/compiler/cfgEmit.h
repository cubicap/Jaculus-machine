#pragma once

#include "ast.h"
#include "cfg.h"
#include <unordered_map>
#include <variant>
#include <vector>


// TODO: remove explicit conversions to bool/return type...


namespace jac::cfg {


enum class ShortCircuitKind {
    And,
    Or
};

const std::unordered_map<std::string_view, ShortCircuitKind> shortCircuitOps = {
    { "&&", ShortCircuitKind::And },
    { "||", ShortCircuitKind::Or }
};

const std::unordered_map<std::string_view, Opcode> binaryOps = {
    { "|", Opcode::BitOr },
    { "^", Opcode::BitXor },
    { "&", Opcode::BitAnd },
    { "==", Opcode::Eq },
    { "!=", Opcode::Neq },
    // { "===", ... },
    // { "!==", ... },
    { "<", Opcode::Lt },
    { "<=", Opcode::Lte },
    { ">", Opcode::Gt },
    { ">=", Opcode::Gte },
    // { "in", ... },
    // { "instanceof", ... },
    { "<<", Opcode::LShift },
    { ">>", Opcode::RShift },
    { ">>>", Opcode::URShift },
    { "+", Opcode::Add },
    { "-", Opcode::Sub },
    { "*", Opcode::Mul },
    { "/", Opcode::Div },
    { "%", Opcode::Rem },
    { "**", Opcode::Pow }
};

const std::unordered_map<std::string_view, Opcode> unaryOps = {
    { "!", Opcode::BoolNot },
    { "~", Opcode::BitNot },
    { "+", Opcode::UnPlus },
    { "-", Opcode::UnMinus },
    // "typeof",
    // "void",
    // "delete",
    // "await"
};

const std::unordered_map<std::string_view, ValueType> types = {
    { "Int32", ValueType::I32 },
    { "Float", ValueType::Double },
    { "Bool", ValueType::Bool },
    { "Object", ValueType::Object },
    { "any", ValueType::Any }
};

inline ValueType getType(std::string_view name) {
    auto it = types.find(name);
    if (it == types.end()) {
        throw std::runtime_error("Invalid type");
    }
    return it->second;
}


template<bool Yield, bool Await>
const ast::PrimaryExpression<Yield, Await>& newExpGetPrimary(const ast::NewExpression<Yield, Await>& newExp) {
    const auto member = std::get_if<ast::MemberExpression<Yield, Await>>(&(newExp.value));
    if (!member) {
        throw std::runtime_error("Only member expressions are supported");
    }
    const auto primary = std::get_if<ast::PrimaryExpression<Yield, Await>>(&(member->value));
    if (!primary) {
        throw std::runtime_error("Only primary expressions are supported");
    }

    return *primary;
}


template<bool Yield, bool Await>
const ast::PrimaryExpression<Yield, Await>& lhsGetPrimary(const ast::LeftHandSideExpression<Yield, Await>& lhs) {
    const auto newExp = std::get_if<ast::NewExpression<Yield, Await>>(&(lhs.value));
    if (newExp) {
        return newExpGetPrimary(*newExp);
    }

    throw std::runtime_error("Only new expressions are supported");
}

template<bool Yield, bool Await>
const std::optional<Identifier> memberGetIdentifier(const ast::MemberExpression<Yield, Await>& member) {
    auto primary = std::get_if<ast::PrimaryExpression<Yield, Await>>(&(member.value));
    if (!primary) {
        return std::nullopt;
    }
    auto identifier = std::get_if<ast::IdentifierReference<Yield, Await>>(&(primary->value));
    if (!identifier) {
        return std::nullopt;
    }

    return identifier->identifier.name.name;
}


[[nodiscard]] inline RValue emitConst(auto value, FunctionEmitter& func) {
    TmpId id = getTmpId();
    ConstInit init = ConstInit{
        .id = id,
        .value = value
    };
    func.emitStatement(init);
    return RValue{ init.type(), id };
}

[[nodiscard]] inline RValue emit(const ast::NumericLiteral& lit, FunctionEmitter &func) {
    return std::visit([&func](auto value) {
        return emitConst(value, func);
    }, lit.value);
}

inline void emitDup(RValue val, FunctionEmitter& func) {
    func.emitStatement({Operation{
        .op = Opcode::Dup,
        .a = val
    }});
}

inline void emitPushFree(RValue val, FunctionEmitter& func) {
    func.emitStatement({Operation{
        .op = Opcode::PushFree,
        .a = val
    }});
}

inline void emitPushFree(Value val, FunctionEmitter& func) {
    if (val.isRValue()) {
        emitPushFree(val.asRValue(), func);
    }
}


[[nodiscard]] inline RValue emit(const ast::Literal& lit, FunctionEmitter &func) {
    struct visitor {
        FunctionEmitter& func;

        RValue operator()(const ast::NullLiteral&) {
            throw std::runtime_error("Null literals are not supported");
        }
        RValue operator()(const ast::BooleanLiteral& lit) {
            return emitConst(lit.value, func);
        }
        RValue operator()(const ast::NumericLiteral& lit) {
            return emit(lit, func);
        }
        RValue operator()(const ast::StringLiteral&) {
            throw std::runtime_error("String literals are not supported");
        }
    };

    return std::visit(visitor{func}, lit.value);
}

inline RValue giveSimple(LVRef lv, FunctionEmitter&) {
    if (lv.isMember()) {
        throw std::runtime_error("Cannot move member");
    }
    return { lv.type, lv.id };
}

inline RValue materialize(LVRef lv, FunctionEmitter& func) {
    if (!lv.isMember()) {
        auto v = RValue{ lv.type, lv.id };
        emitDup(v, func);
        return v;
    }

    LVRef res = LVRef::direct(ValueType::Any, getTmpId(), false);
    func.emitStatement({Operation{
        .op = Opcode::GetMember,
        .a = { lv.parentType, lv.id },
        .b = *lv.member,
        .res = res
    }});

    return giveSimple(res, func);
}

inline RValue materialize(Value val, FunctionEmitter& func) {
    if (val.isRValue()) {
        return val.asRValue();
    }
    return materialize(val.asLVRef(), func);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::PrimaryExpression<Yield, Await>& primary, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::ThisExpr&) {
            throw std::runtime_error("This expressions are not supported");
        }
        Value operator()(const ast::IdentifierReference<Yield, Await>& identifier) {
            Identifier ident = identifier.identifier.name.name;
            auto local = func.getLocal(ident);
            if (!local) {
                throw std::runtime_error("Identifier referenced before declaration (" + identifier.identifier.name.name + ")");
            }
            return { *local };
        }
        Value operator()(const ast::Literal& literal) {
            return { emit(literal, func) };
        }
        Value operator()(const ast::ArrayLiteral<Yield, Await>&) {
            throw std::runtime_error("Array literals are not supported");
        }
        Value operator()(const ast::ObjectLiteral<Yield, Await>&) {
            throw std::runtime_error("Object literals are not supported");
        }
        Value operator()(const ast::FunctionExpression&) {
            throw std::runtime_error("Function expressions are not supported");
        }
        Value operator()(const ast::ClassExpression<Yield, Await>&) {
            throw std::runtime_error("Class expressions are not supported");
        }
        Value operator()(const ast::GeneratorExpression&) {
            throw std::runtime_error("Generator expressions are not supported");
        }
        Value operator()(const ast::AsyncFunctionExpression&) {
            throw std::runtime_error("Async function expressions are not supported");
        }
        Value operator()(const ast::AsyncGeneratorExpression&) {
            throw std::runtime_error("Async generator expressions are not supported");
        }
        Value operator()(const ast::RegularExpressionLiteral&) {
            throw std::runtime_error("Regular expression literals are not supported");
        }
        Value operator()(const ast::TemplateLiteral<Yield, Await, false>&) {
            throw std::runtime_error("Template literals are not supported");
        }
        Value operator()(const ast::ParenthesizedExpression<Yield, Await>& expr) {
            return { emit(expr.expression, func) };
        }
    };

    return std::visit(visitor{func}, primary.value);
}

template<bool Yield, bool Await>
[[nodiscard]] RValue emitCallNative(Identifier ident, const ast::Arguments<Yield, Await>& args_, FunctionEmitter& func) {
    auto sig = func.getSignature(ident);
    if (!sig) {
        throw std::runtime_error("Function not found: " + ident);
    }

    RValue res = { sig->ret, getTmpId() };

    std::list<RValue> args;
    for (const auto& [ spread, expr ] : args_.arguments) {  // TODO: check arguments count
        if (spread) {
            throw std::runtime_error("Spread arguments are not supported");
        }
        Value arg = emit(*expr, func);
        args.push_back(materialize(arg, func));
        emitPushFree(arg, func);
    }

    func.emitStatement({Call{
        .obj = ident,
        .args = args,
        .res = res
    }});
    func.addRequiredFunction(ident);

    return res;
}

template<bool Yield, bool Await>
[[nodiscard]] RValue emit(const ast::CallExpression<Yield, Await>& call, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        RValue operator()(const ast::SuperCall<Yield, Await>&) {
            throw std::runtime_error("Super calls are not supported");
        }
        RValue operator()(const ast::ImportCall<Yield, Await>&) {
            throw std::runtime_error("Import calls are not supported");
        }
        RValue operator()(const std::pair<ast::MemberExpression<Yield, Await>, ast::Arguments<Yield, Await>>& call) {
            if (auto ident = memberGetIdentifier(call.first); ident) {
                if (!func.getLocal(*ident)) {
                    return emitCallNative(*ident, call.second, func);
                }
            }

            Value obj = emit(call.first, func);
            RValue objR = materialize(obj, func);
            emitPushFree(objR, func);

            RValue res = { ValueType::Any, getTmpId() };  // TODO: infer type from called function

            std::list<RValue> args;
            for (const auto& [ spread, expr ] : call.second.arguments) {
                if (spread) {
                    throw std::runtime_error("Spread arguments are not supported");
                }
                Value arg = emit(*expr, func);
                args.push_back(materialize(arg, func));
                emitPushFree(arg, func);
            }

            func.emitStatement({Call{
                .obj = objR,
                .args = args,
                .res = res
            }});

            return res;
        }
        RValue operator()(const std::pair<ast::CallExpressionPtr<Yield, Await>, ast::Arguments<Yield, Await>>&) { // call
            throw std::runtime_error("Call -> args are not supported");
        }
        RValue operator()(const std::pair<ast::CallExpressionPtr<Yield, Await>, ast::Expression<true, Yield, Await>>&) { // brackets
            throw std::runtime_error("Call -> brackets are not supported");
        }
        RValue operator()(const std::pair<ast::CallExpressionPtr<Yield, Await>, ast::IdentifierName>&) { // .
            throw std::runtime_error("Call -> dots are not supported");
        }
        RValue operator()(const std::pair<ast::CallExpressionPtr<Yield, Await>, ast::PrivateIdentifier>&) { // .private
            throw std::runtime_error("Call -> .private are not supported");
        }
        RValue operator()(const std::pair<ast::CallExpressionPtr<Yield, Await>, ast::TemplateLiteral<Yield, Await, true>>&) { // template
            throw std::runtime_error("Call -> template literals are not supported");
        }
    };

    return std::visit(visitor{func}, call.value);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::MemberExpression<Yield, Await>& member, FunctionEmitter& func);


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::IdentifierName>& memDot, FunctionEmitter& func) {
    const auto& [mem, ident] = memDot;
    Value obj = emit(*mem, func);
    RValue objR = materialize(obj, func);
    emitPushFree(objR, func);

    RValue identR = emitConst(ident.name, func);

    LVRef res = LVRef::mbr(ValueType::Any, objR.id, identR, objR.type, false);

    return { res };
}

template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::MemberExpression<Yield, Await>& member, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::SuperProperty<Yield, Await>&) {
            throw std::runtime_error("Super properties are not supported");
        }

        Value operator()(const ast::MetaProperty&) {
            throw std::runtime_error("Meta properties are not supported");
        }

        Value operator()(const ast::PrimaryExpression<Yield, Await>& primary) {
            return emit(primary, func);
        }

        Value operator()(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::Expression<true, Yield, Await>>&) { // brackets
            throw std::runtime_error("Member -> brackets are not supported");
        }

        Value operator()(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::IdentifierName>& memDot) { // .
            return emit(memDot, func);
        }

        Value operator()(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::PrivateIdentifier>&) { // .private
            throw std::runtime_error("Member -> .private are not supported");
        }

        Value operator()(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::TemplateLiteral<Yield, Await, true>>&) { // template
            throw std::runtime_error("Member -> template literals are not supported");
        }

        Value operator()(const std::pair<ast::MemberExpressionPtr<Yield, Await>, ast::Arguments<Yield, Await>>&) { // new
            throw std::runtime_error("New expressions are not supported");
        }
    };

    return std::visit(visitor{func}, member.value);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::NewExpression<Yield, Await>& newExp, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::MemberExpression<Yield, Await>& member) {
            return emit(member, func);
        }
        Value operator()(const ast::NewExpressionPtr<Yield, Await>&) {
            throw std::runtime_error("New expressions are not supported");
        }
    };

    return std::visit(visitor{func}, newExp.value);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::LeftHandSideExpression<Yield, Await>& lhs, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::CallExpression<Yield, Await>& call) {
            return { emit(call, func) };
        }
        Value operator()(const ast::NewExpression<Yield, Await>& newExp) {
            return emit(newExp, func);
        }
    };

    return std::visit(visitor{func}, lhs.value);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::UpdateExpression<Yield, Await>& expr, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::UnaryExpressionPtr<Yield, Await>&) {
            throw std::runtime_error("Update unary expressions are not supported");
        }

        Value operator()(const ast::LeftHandSideExpressionPtr<Yield, Await>& lhs) {
            return emit(*lhs, func);
        }
    };

    return std::visit(visitor{func}, expr.value);
}


template<bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::UnaryExpression<Yield, Await>& expr, FunctionEmitter& func) {
    if (std::holds_alternative<ast::UpdateExpression<Yield, Await>>(expr.value)) {
        return emit(std::get<ast::UpdateExpression<Yield, Await>>(expr.value), func);
    }

    const auto& un = std::get<std::pair<ast::UnaryExpressionPtr<Yield, Await>, std::string_view>>(expr.value);
    Value arg = emit(*(un.first), func);

    auto it = unaryOps.find(un.second);
    if (it == unaryOps.end()) {
        throw std::runtime_error("Unsupported unary operator '" + std::string(un.second) + "'");
    }
    Opcode op = it->second;

    RValue argR = materialize(arg, func);
    LVRef res = LVRef::direct(argR.type, getTmpId(), false);

    func.emitStatement({Operation{
        .op = op,
        .a = argR,
        .res = res
    }});

    return { giveSimple(res, func) };
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] Value emitShortCircuit(const ast::BinaryExpression<In, Yield, Await>& lhs, const ast::BinaryExpression<In, Yield, Await>& rhs, ShortCircuitKind kind) {
    (void)lhs;
    (void)rhs;
    (void)kind;
    throw std::runtime_error("Short circuit expressions are not supported");
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::BinaryExpression<In, Yield, Await>& expr, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const std::tuple<
                                ast::BinaryExpressionPtr<In, Yield, Await>,
                                ast::BinaryExpressionPtr<In, Yield, Await>,
                                std::string_view
                            >& binExpr) {
            if (auto it = shortCircuitOps.find(std::get<2>(binExpr)); it != shortCircuitOps.end()) {
                auto& [lhs, rhs, _] = binExpr;
                return emitShortCircuit(*lhs, *rhs, it->second);
            }

            auto it = binaryOps.find(std::get<2>(binExpr));
            if (it == binaryOps.end()) {
                throw std::runtime_error("Unsupported binary operator");
            }

            Value lop = emit(*std::get<0>(binExpr), func);
            Value rop = emit(*std::get<1>(binExpr), func);
            Opcode op = it->second;

            RValue lopR = materialize(lop, func);
            emitPushFree(lopR, func);
            RValue ropR = materialize(rop, func);
            emitPushFree(ropR, func);

            LVRef res = LVRef::direct(resultType(op, lopR.type, ropR.type), getTmpId(), false);

            func.emitStatement({Operation{
                .op = op,
                .a = lopR,
                .b = ropR,
                .res = res
            }});

            return { giveSimple(res, func) };
        }
        Value operator()(const ast::UnaryExpression<Yield, Await>& unary) {
            return emit(unary, func);
        }
    };

    return std::visit(visitor{func}, expr.value);
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::ConditionalExpression<In, Yield, Await>& expr, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::BinaryExpression<In, Yield, Await>& xpr) {
            return emit(xpr, func);
        }
        Value operator()(const std::tuple<  // ternary conditional operator
                                ast::BinaryExpression<In, Yield, Await>,
                                ast::AssignmentExpressionPtr<In, Yield, Await>,
                                ast::AssignmentExpressionPtr<In, Yield, Await>
                            >&) {
            throw std::runtime_error("Ternary conditional operator is not supported");
        }
    };

    return std::visit(visitor{func}, expr.value);
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] RValue emit(const ast::Assignment<In, Yield, Await>& assign, FunctionEmitter& func);


template<bool In, bool Yield, bool Await>
[[nodiscard]] Value emit(const ast::AssignmentExpression<In, Yield, Await>& expr, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::ConditionalExpression<In, Yield, Await>& cond) {
            return emit(cond, func);
        }
        Value operator()(const ast::YieldExpression<In, Yield, Await>&) {
            throw std::runtime_error("Yield expressions are not supported");
        }
        Value operator()(const ast::ArrowFunction<In, Yield, Await>&) {
            throw std::runtime_error("Arrow functions are not supported");
        }
        Value operator()(const ast::AsyncArrowFunction<In, Yield, Await>&) {
            throw std::runtime_error("Async arrow functions are not supported");
        }
        Value operator()(const ast::Assignment<In, Yield, Await>& assign) {
            return { emit(assign, func) };
        }
    };

    return std::visit(visitor{func}, expr.value);
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] RValue emit(const ast::Assignment<In, Yield, Await>& assign, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        Value operator()(const ast::MemberExpression<Yield, Await>& expr) {
            return emit(expr, func);
        }
        Value operator()(const ast::NewExpressionPtr<Yield, Await>&) {
            throw std::runtime_error("Assignment patterns are not supported");
        }
    };
    if (assign.op != "=") {
        throw std::runtime_error("Only simple assignments are supported");
    }

    if (!std::holds_alternative<ast::NewExpression<Yield, Await>>(assign.lhs.value)) {
        throw std::runtime_error("Only new expressions are supported in assignments");
    }
    Value target = std::visit(visitor{func}, std::get<ast::NewExpression<Yield, Await>>(assign.lhs.value).value);
    if (target.isRValue()) {
        throw std::runtime_error("Invalid assignment target");
    }

    Value rhs = emit(*assign.rhs, func);
    RValue rhsR = materialize(rhs, func);

    func.emitStatement({Operation{
        .op = Opcode::Set,
        .a = rhsR,
        .res = target.asLVRef()
    }});

    return rhsR;
}


template<bool In, bool Yield, bool Await>
void emit(const ast::LexicalDeclaration<In, Yield, Await>& lexical, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;
        ValueType type;

        void operator()(const ast::BindingIdentifier<Yield, Await>& decl) {
            func.addLexical(decl.identifier.name.name, type, false);
            // do nothing
        }
        void operator()(const std::pair<ast::BindingIdentifier<Yield, Await>, ast::InitializerPtr<In, Yield, Await>>& assign) {
            const auto& [binding, initializer] = assign;

            Value rhs = emit(*initializer, func);
            LVRef target = func.addLexical(binding.identifier.name.name, type, false);

            RValue rhsR = materialize(rhs, func);
            emitPushFree(rhsR, func);

            func.emitStatement({Operation{
                .op = Opcode::Set,
                .a = rhsR,
                .res = target
            }});
        }
        void operator()(const std::pair<ast::BindingPattern<Yield, Await>, ast::InitializerPtr<In, Yield, Await>>&) {
            throw std::runtime_error("Binding patterns are not supported");
        }
    };

    for (const ast::LexicalBinding<In, Yield, Await>& binding : lexical.bindings) {
        if (!binding.type) {
            throw std::runtime_error("Lexical bindings must have a type");
        }
        std::visit(visitor{ func, getType(binding.type->type.name.name) }, binding.value);
    }
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::Declaration<Yield, Await, Return>& declaration, FunctionEmitter& func) {
    if (!std::holds_alternative<ast::LexicalDeclaration<true, Yield, Await>>(declaration.value)) {
        throw std::runtime_error("Only lexical declarations are supported");
    }

    emit(std::get<ast::LexicalDeclaration<true, Yield, Await>>(declaration.value), func);
}


template<bool In, bool Yield, bool Await>
[[nodiscard]] RValue emit(const ast::Expression<In, Yield, Await>& expr, FunctionEmitter& func) {
    for (size_t i = 0; i + 1 < expr.items.size(); i++) {
        Value v = emit(*expr.items[i], func);
        emitPushFree(v, func);
    }
    if (expr.items.size() != 0) {
        Value last = emit(*expr.items.back(), func);
        return materialize(last, func);
    }
    throw std::runtime_error("Empty expression");
}


template<bool Yield, bool Await, bool Return>
bool emit(const ast::Statement<Yield, Await, Return>& statement, FunctionEmitter& func);


template<bool Yield, bool Await, bool Return>
void emit(const ast::IfStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    auto preBlock = func.getActiveBlock();
    auto condBlock = func.createBlock();
    auto ifBlock = func.createBlock();
    auto elseBlock = func.createBlock();  // XXX: set to null if no else block
    auto postBlock = func.createBlock();

    postBlock->jump = preBlock->jump;
    preBlock->jump = Terminal::jump(condBlock);
    ifBlock->jump = Terminal::jump(postBlock);
    elseBlock->jump = Terminal::jump(postBlock);

    // condition block
    func.setActiveBlock(condBlock);
    RValue cond = emit(stmt.expression, func);
    emitPushFree(cond, func);
    LVRef res = LVRef::direct(ValueType::Bool, getTmpId(), false);
    func.emitStatement({Operation{
        .op = Opcode::Set,
        .a = cond,
        .res = res
    }});

    RValue resR = giveSimple(res, func);
    emitPushFree(resR, func);
    condBlock->jump = Terminal::branch(resR, ifBlock, elseBlock);

    // if block
    func.setActiveBlock(ifBlock);
    emit(*stmt.consequent, func);

    // else block
    if (stmt.alternate) {
        func.setActiveBlock(elseBlock);
        emit(**stmt.alternate, func);
    }

    func.setActiveBlock(postBlock);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::DoWhileStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    auto preBlock = func.getActiveBlock();
    auto condBlock = func.createBlock();
    auto loopBlock = func.createBlock();
    auto postBlock = func.createBlock();

    postBlock->jump = preBlock->jump;
    preBlock->jump = Terminal::jump(loopBlock);
    loopBlock->jump = Terminal::jump(condBlock);

    // condition block
    func.setActiveBlock(condBlock);
    RValue cond = emit(stmt.expression, func);
    emitPushFree(cond, func);
    LVRef res = LVRef::direct(ValueType::Bool, getTmpId(), false);
    func.emitStatement({Operation{
        .op = Opcode::Set,
        .a = cond,
        .res = res
    }});

    RValue resR = giveSimple(res, func);
    emitPushFree(resR, func);
    condBlock->jump = Terminal::branch(resR, loopBlock, postBlock);

    // loop block
    func.setActiveBlock(loopBlock);
    {
        auto _ = func.pushBreakTarget(postBlock);
        auto __ = func.pushContinueTarget(condBlock);
        emit(*stmt.statement, func);
    }

    func.setActiveBlock(postBlock);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::WhileStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    auto preBlock = func.getActiveBlock();
    auto condBlock = func.createBlock();
    auto loopBlock = func.createBlock();
    auto postBlock = func.createBlock();

    postBlock->jump = preBlock->jump;
    preBlock->jump = Terminal::jump(condBlock);
    loopBlock->jump = Terminal::jump(condBlock);

    // condition block
    func.setActiveBlock(condBlock);
    RValue cond = emit(stmt.expression, func);
    emitPushFree(cond, func);
    LVRef res = LVRef::direct(ValueType::Bool, getTmpId(), false);
    func.emitStatement({Operation{
        .op = Opcode::Set,
        .a = cond,
        .res = res
    }});

    RValue resR = giveSimple(res, func);
    emitPushFree(resR, func);
    condBlock->jump = Terminal::branch(resR, loopBlock, postBlock);

    // loop block
    func.setActiveBlock(loopBlock);
    {
        auto _ = func.pushBreakTarget(postBlock);
        auto __ = func.pushContinueTarget(condBlock);
        emit(*stmt.statement, func);
    }

    func.setActiveBlock(postBlock);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::ForStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    struct InitVisitor {
        FunctionEmitter& func;

        void operator()(std::monostate) {
            // do nothing
        }
        void operator()(const ast::Expression<false, Yield, Await>& expr) {
            RValue v = emit(expr, func);
            emitPushFree(v, func);
        }
        void operator()(const ast::VariableDeclarationList<false, Yield, Await>&) {
            throw std::runtime_error("Variable declarations are not supported");
        }
        void operator()(const ast::LexicalDeclaration<false, Yield, Await>& decl) {
            emit(decl, func);
        }
    };

    BasicBlockPtr preBlock = func.getActiveBlock();
    BasicBlockPtr initBlock = func.createBlock();
    BasicBlockPtr condBlock = func.createBlock();
    BasicBlockPtr loopBlock = func.createBlock();
    BasicBlockPtr updateBlock = func.createBlock();
    BasicBlockPtr postBlock = func.createBlock();

    postBlock->jump = preBlock->jump;
    preBlock->jump = Terminal::jump(initBlock);
    initBlock->jump = Terminal::jump(condBlock);
    loopBlock->jump = Terminal::jump(updateBlock);
    updateBlock->jump = Terminal::jump(condBlock);

    auto _ = func.pushScope();

    // init block
    func.setActiveBlock(initBlock);
    std::visit(InitVisitor{func}, stmt.init);

    // condition block
    if (stmt.condition) {
        func.setActiveBlock(condBlock);
        RValue cond = emit(*stmt.condition, func);
        emitPushFree(cond, func);
        LVRef res = LVRef::direct(ValueType::Bool, getTmpId(), false);
        func.emitStatement({Operation{
            .op = Opcode::Set,
            .a = cond,
            .res = res
        }});

        RValue resR = giveSimple(res, func);
        emitPushFree(resR, func);
        condBlock->jump = Terminal::branch(resR, loopBlock, postBlock);
    }
    else {
        condBlock->jump = Terminal::jump(loopBlock);
    }

    // update block
    if (stmt.update) {
        func.setActiveBlock(updateBlock);
        RValue v = emit(*stmt.update, func);
        emitPushFree(v, func);
    }

    // loop block
    func.setActiveBlock(loopBlock);
    {
        auto __ = func.pushBreakTarget(postBlock);
        auto ___ = func.pushContinueTarget(updateBlock);
        emit(*stmt.statement, func);
    }

    func.setActiveBlock(postBlock);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::IterationStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        void operator()(const ast::DoWhileStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
        }
        void operator()(const ast::WhileStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
        }
        void operator()(const ast::ForStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
        }
        void operator()(const ast::ForInOfStatement<Yield, Await, Return>&) {
            throw std::runtime_error("For-in/of statements are not supported");
        }
    };

    // XXX: move continue target here?
    std::visit(visitor{func}, stmt.value);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::BreakableStatement<Yield, Await, Return>& stmt, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        void operator()(const ast::IterationStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
        }
        void operator()(const ast::SwitchStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Switch statements are not supported");
        }
    };

    // XXX: move break target here?
    std::visit(visitor{func}, stmt.value);
}


template<bool Yield, bool Await>
void emit(const ast::ReturnStatement<Yield, Await>& stmt, FunctionEmitter& func) {
    if (!stmt.expression) {
        func.getActiveBlock()->jump = Terminal::ret();
        return;
    }

    Value arg = { emit(*stmt.expression, func) };
    RValue argR = materialize(arg, func);

    func.getActiveBlock()->jump = Terminal::retVal(argR);
}


template<bool Yield, bool Await, bool Return>
bool emit(const ast::Block<Yield, Await, Return>& block, FunctionEmitter& func);


template<bool Yield, bool Await, bool Return>
bool emit(const ast::Statement<Yield, Await, Return>& statement, FunctionEmitter& func) {
    struct visitor {
        FunctionEmitter& func;

        bool operator()(const ast::BlockStatement<Yield, Await, Return>& stmt) {
            return emit(stmt, func);
        }
        bool operator()(const ast::VariableStatement<Yield, Await>&) {
            throw std::runtime_error("Variable statements are not supported");
        }
        bool operator()(const ast::EmptyStatement&) {
            throw std::runtime_error("Empty statements are not supported");
        }
        bool operator()(const ast::ExpressionStatement<Yield, Await>& stmt) {
            RValue v = emit(stmt.expression, func);
            emitPushFree(v, func);
            return false;
        }
        bool operator()(const ast::IfStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
            return false;
        }
        bool operator()(const ast::BreakableStatement<Yield, Await, Return>& stmt) {
            emit(stmt, func);
            return false;
        }
        bool operator()(const ast::ContinueStatement<Yield, Await>&) {
            throw std::runtime_error("Continue statements are not supported");
        }
        bool operator()(const ast::BreakStatement<Yield, Await>&) {
            throw std::runtime_error("Break statements are not supported");
        }
        bool operator()(const ast::ReturnStatement<Yield, Await>& stmt) {
            emit(stmt, func);
            return true;
        }
        bool operator()(const ast::WithStatement<Yield, Await, Return>&) {
            throw std::runtime_error("With statements are not supported");
        }
        bool operator()(const ast::LabeledStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Labeled statements are not supported");
        }
        bool operator()(const ast::ThrowStatement<Yield, Await>&) {
            throw std::runtime_error("Throw statements are not supported");
        }
        bool operator()(const ast::TryStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Try statements are not supported");
        }
        bool operator()(const ast::DebuggerStatement&) {
            throw std::runtime_error("Debugger statements are not supported");
        }
    };

    return std::visit(visitor{func}, statement.value);
}


template<bool Yield, bool Await, bool Return>
bool emit(const ast::StatementList<Yield, Await, Return>& list, FunctionEmitter& func) {
    // FIXME: ignore dead code
    struct visitor {
        FunctionEmitter& func;
        bool operator()(const ast::Statement<Yield, Await, Return>& statement) {
            return emit(statement, func);
        }
        bool operator()(const ast::Declaration<Yield, Await, Return>& declaration) {
            emit(declaration, func);
            return false;
        }
    };

    for (const ast::StatementListItem<Yield, Await, Return>& statement : list.items) {
        if (std::visit(visitor{func}, statement.value)) {
            return true;
        }
    }

    return false;
}


template<bool Yield, bool Await, bool Return>
bool emit(const ast::Block<Yield, Await, Return>& block, FunctionEmitter& func) {
    if (block.statementList) {
        auto _ = func.pushScope();
        return emit(*block.statementList, func);
    }
    return false;
}


template<bool Yield, bool Await, bool Default>
SignaturePtr getSignature(const ast::FunctionDeclaration<Yield, Await, Default>& decl) {
    auto sig = std::make_shared<Signature>();
    if (!decl.returnType) {
        return nullptr;
    }
    sig->ret = getType(decl.returnType->type.name.name);

    for (const ast::FormalParameter<Yield, Await>& arg : decl.parameters.parameterList) {
        if (!std::holds_alternative<ast::BindingIdentifier<Yield, Await>>(arg.value)) {
            return nullptr;
        }
        const auto& binding = std::get<ast::BindingIdentifier<Yield, Await>>(arg.value);

        if (!arg.type) {
            return nullptr;
        }

        sig->args.emplace_back(binding.identifier.name.name, getType(arg.type->type.name.name));
    }

    return sig;
}

template<bool Yield, bool Await, bool Default>
FunctionEmitter emit(const ast::FunctionDeclaration<Yield, Await, Default>& decl, SignaturePtr sig, const std::map<cfg::Identifier, cfg::SignaturePtr>& otherSignatures) {
    if (!decl.name) {
        throw std::runtime_error("Function declarations must have a name");
    }

    FunctionEmitter out(otherSignatures);
    out.setSignature(sig);
    out.setFunctionName(decl.name->identifier.name.name);

    emit(*decl.body, out);

    if (out.getActiveBlock()->jump.type == Terminal::None) {
        out.getActiveBlock()->jump = Terminal::ret();
    }

    return out;
}


}  // namespace jac::cfg
