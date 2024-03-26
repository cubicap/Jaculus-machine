#pragma once

#include "ast.h"
#include "tacTree.h"
#include <unordered_map>
#include <variant>


namespace jac::tac {


inline tac::Identifier id(std::string name) {
    return "i_" + name;
}

inline tac::Identifier tmp() {
    static unsigned counter = 0;
    return "t_" + std::to_string(counter++);
}


const std::unordered_map<std::string_view, tac::Operation> binaryOps = {
    { "||", tac::Operation::BoolOr },
    { "&&", tac::Operation::BoolAnd },
    { "|", tac::Operation::BitOr },
    { "^", tac::Operation::BitXor },
    { "&", tac::Operation::BitAnd },
    { "==", tac::Operation::Eq },
    { "!=", tac::Operation::Neq },
    // { "===", ... },
    // { "!==", ... },
    { "<", tac::Operation::SignedLt },
    { "<=", tac::Operation::SignedLte },
    { ">", tac::Operation::SignedGt },
    { ">=", tac::Operation::SignedGte },
    // { "in", ... },
    // { "instanceof", ... },
    { "<<", tac::Operation::LShift },
    { ">>", tac::Operation::RShift },
    { ">>>", tac::Operation::URShift },
    { "+", tac::Operation::Add },
    { "-", tac::Operation::Sub },
    { "*", tac::Operation::Mul },
    { "/", tac::Operation::Div },
    { "%", tac::Operation::Mod },
    { "**", tac::Operation::Pow }
};

const std::unordered_map<std::string_view, tac::Operation> unaryOps = {
    { "!", tac::Operation::BoolNot },
    { "~", tac::Operation::BitNot },
    { "+", tac::Operation::UnPlus },
    { "-", tac::Operation::UnMinus },
    // "typeof",
    // "void",
    // "delete",
    // "await"
};


template<bool Yield, bool Await>
const ast::PrimaryExpression<Yield, Await>& lhsGetPrimary(const ast::LeftHandSideExpression<Yield, Await>& lhs) {
    const auto newExp = std::get_if<ast::NewExpression<Yield, Await>>(&(lhs.value));
    if (!newExp) {
        throw std::runtime_error("Only new expressions are supported");
    }
    const auto member = std::get_if<ast::MemberExpression<Yield, Await>>(&(newExp->value));
    if (!member) {
        throw std::runtime_error("Only member expressions are supported");
    }
    const auto primary = std::get_if<ast::PrimaryExpression<Yield, Await>>(&(member->value));
    if (!primary) {
        throw std::runtime_error("Only primary expressions are supported");
    }

    return *primary;
}


inline tac::Arg emit(ast::NumericLiteral lit) {
    struct visitor {
        tac::Arg operator()(int32_t lit) {
            return lit;
        }
        tac::Arg operator()(double lit) {
            return lit;
        }
    };

    return std::visit(visitor{}, lit.value);
}


inline tac::Arg emit(const ast::Literal& lit) {
    struct visitor {
        tac::Arg operator()(const ast::NullLiteral&) {
            throw std::runtime_error("Null literals are not supported");
        }
        tac::Arg operator()(const ast::BooleanLiteral& lit) {
            return tac::Arg(lit.value);
        }
        tac::Arg operator()(const ast::NumericLiteral& lit) {
            return emit(lit);
        }
        tac::Arg operator()(const ast::StringLiteral&) {
            throw std::runtime_error("String literals are not supported");
        }
    };

    return std::visit(visitor{}, lit.value);
}


template<bool Yield, bool Await>
tac::Arg emit(const ast::PrimaryExpression<Yield, Await>& primary, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const ast::ThisExpr&) {
            throw std::runtime_error("This expressions are not supported");
        }
        tac::Arg operator()(const ast::IdentifierReference<Yield, Await>& identifier) {
            return tac::Arg(id(identifier.identifier.name.name));
        }
        tac::Arg operator()(const ast::Literal& literal) {
            return emit(literal);
        }
        tac::Arg operator()(const ast::ArrayLiteral<Yield, Await>&) {
            throw std::runtime_error("Array literals are not supported");
        }
        tac::Arg operator()(const ast::ObjectLiteral<Yield, Await>&) {
            throw std::runtime_error("Object literals are not supported");
        }
        tac::Arg operator()(const ast::FunctionExpression&) {
            throw std::runtime_error("Function expressions are not supported");
        }
        tac::Arg operator()(const ast::ClassExpression<Yield, Await>&) {
            throw std::runtime_error("Class expressions are not supported");
        }
        tac::Arg operator()(const ast::GeneratorExpression&) {
            throw std::runtime_error("Generator expressions are not supported");
        }
        tac::Arg operator()(const ast::AsyncFunctionExpression&) {
            throw std::runtime_error("Async function expressions are not supported");
        }
        tac::Arg operator()(const ast::AsyncGeneratorExpression&) {
            throw std::runtime_error("Async generator expressions are not supported");
        }
        tac::Arg operator()(const ast::RegularExpressionLiteral&) {
            throw std::runtime_error("Regular expression literals are not supported");
        }
        tac::Arg operator()(const ast::TemplateLiteral<Yield, Await, false>&) {
            throw std::runtime_error("Template literals are not supported");
        }
        tac::Arg operator()(const ast::ParenthesizedExpression<Yield, Await>&) {
            throw std::runtime_error("Parenthesized expressions are not supported");
        }
    };

    return std::visit(visitor{block}, primary.value);
}


template<bool Yield, bool Await>
tac::Arg emit(const ast::UpdateExpression<Yield, Await>& expr, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const ast::UnaryExpressionPtr<Yield, Await>&) {
            throw std::runtime_error("Unary expressions are not supported");
        }

        tac::Arg operator()(const ast::LeftHandSideExpressionPtr<Yield, Await>& lhs) {
            return emit(lhsGetPrimary(*lhs), block);
        }
    };

    return std::visit(visitor{block}, expr.value);
}


template<bool Yield, bool Await>
tac::Arg emit(const ast::UnaryExpression<Yield, Await>& expr, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const ast::UnaryExpressionPtr<Yield, Await>& unary) {
            tac::Arg arg = emit(*unary, block);

            auto it = unaryOps.find(unary->op);
            if (it == unaryOps.end()) {
                throw std::runtime_error("Unsupported unary operator");
            }
            tac::Operation op = it->second;

            tac::Identifier res = tmp();

            block.statements.statements.push_back(tac::Statement{
                .op = op,
                .arg1 = arg,
                .resultType = tac::ValueType::Word,
                .result = res
            });

            return res;
        }

        tac::Arg operator()(const ast::UpdateExpression<Yield, Await>& update) {
            return emit(update, block);
        }
    };

    return std::visit(visitor{block}, expr.value);
}


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::BinaryExpression<In, Yield, Await>& expr, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const std::tuple<
                                ast::BinaryExpressionPtr<In, Yield, Await>,
                                ast::BinaryExpressionPtr<In, Yield, Await>,
                                std::string_view
                            >& binExpr) {
            tac::Arg lhs = emit(*std::get<0>(binExpr), block);
            tac::Arg rhs = emit(*std::get<1>(binExpr), block);

            auto it = binaryOps.find(std::get<2>(binExpr));
            if (it == binaryOps.end()) {
                throw std::runtime_error("Unsupported binary operator");
            }
            tac::Operation op = it->second;

            tac::Identifier res = tmp();

            block.statements.statements.push_back(tac::Statement{
                .op = op,
                .arg1 = lhs,
                .arg2 = rhs,
                .resultType = tac::ValueType::Word,
                .result = res
            });

            return res;
        }
        tac::Arg operator()(const ast::UnaryExpression<Yield, Await>& unary) {
            return emit(unary, block);
        }
    };

    return std::visit(visitor{block}, expr.value);
}


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::ConditionalExpression<In, Yield, Await>& expr, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const ast::BinaryExpression<In, Yield, Await>& xpr) {
            return emit(xpr, block);
        }
        tac::Arg operator()(const std::tuple<  // ternary conditional operator
                                ast::BinaryExpression<In, Yield, Await>,
                                ast::AssignmentExpressionPtr<In, Yield, Await>,
                                ast::AssignmentExpressionPtr<In, Yield, Await>
                            >&) {
            throw std::runtime_error("Ternary conditional operator is not supported");
        }
    };

    return std::visit(visitor{block}, expr.value);
}


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::Assignment<In, Yield, Await>& assign, tac::Block& block);


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::AssignmentExpression<In, Yield, Await>& expr, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        tac::Arg operator()(const ast::ConditionalExpression<In, Yield, Await>& cond) {
            return emit(cond, block);
        }
        tac::Arg operator()(const ast::YieldExpression<In, Yield, Await>&) {
            throw std::runtime_error("Yield expressions are not supported");
        }
        tac::Arg operator()(const ast::ArrowFunction<In, Yield, Await>&) {
            throw std::runtime_error("Arrow functions are not supported");
        }
        tac::Arg operator()(const ast::AsyncArrowFunction<In, Yield, Await>&) {
            throw std::runtime_error("Async arrow functions are not supported");
        }
        tac::Arg operator()(const ast::Assignment<In, Yield, Await>& assign) {
            return emit(assign, block);
        }
    };

    return std::visit(visitor{block}, expr.value);
}


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::Assignment<In, Yield, Await>& assign, tac::Block& block) {
    const auto& lhsPrimary = lhsGetPrimary(assign.lhs);
    if (!std::holds_alternative<ast::IdentifierReference<Yield, Await>>(lhsPrimary.value)) {
        throw std::runtime_error("Only identifier references are supported as assignment targets");
    }
    tac::Identifier target = id(std::get<ast::IdentifierReference<Yield, Await>>(lhsPrimary.value).identifier.name.name);

    tac::Arg rhs = emit(*assign.rhs, block);

    if (assign.op != "=") {
        throw std::runtime_error("Only simple assignments are supported");
    }

    block.statements.statements.push_back(tac::Statement{
        .op = tac::Operation::Copy,
        .arg1 = rhs,
        .resultType = tac::ValueType::Word,
        .result = target
    });

    return target;
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::Declaration<Yield, Await, Return>& declaration, tac::Block& block) {
    if (!std::holds_alternative<ast::LexicalDeclaration<true, Yield, Await>>(declaration.value)) {
        throw std::runtime_error("Only lexical declarations are supported");
    }

    struct visitor {
        tac::Block& block;
        void operator()(const ast::BindingIdentifier<Yield, Await>& decl) {
            // do nothing
        }
        void operator()(const std::pair<ast::BindingIdentifier<Yield, Await>, ast::InitializerPtr<true, Yield, Await>>& assign) {
            const auto& [binding, initializer] = assign;

            tac::Arg tmp = emit(*initializer, block);
            block.statements.statements.push_back(tac::Statement{
                .op = tac::Operation::Copy,
                .arg1 = tmp,
                .resultType = tac::ValueType::Word,
                .result = id(binding.identifier.name.name)
            });
        }
        void operator()(const std::pair<ast::BindingPattern<Yield, Await>, ast::InitializerPtr<true, Yield, Await>>&) {
            throw std::runtime_error("Binding patterns are not supported");
        }
    };

    const auto& lexical = std::get<ast::LexicalDeclaration<true, Yield, Await>>(declaration.value);
    for (const ast::LexicalBinding<true, Yield, Await>& binding : lexical.bindings) {
        std::visit(visitor{block}, binding.value);
    }
}


template<bool In, bool Yield, bool Await>
tac::Arg emit(const ast::Expression<In, Yield, Await>& expr, tac::Block& block) {
    for (size_t i = 0; i + 1 < expr.items.size(); i++) {
        emit(*expr.items[i], block);
    }
    if (expr.items.size() != 0) {
        return emit(*expr.items.back(), block);
    }
    return tac::Arg(0);
}


template<bool Yield, bool Await>
void emit(const ast::ReturnStatement<Yield, Await>& stmt, tac::Block& block) {
    if (!stmt.expression) {
        block.jump = tac::Jump::retVal(tac::Arg(0));
        return;
    }

    tac::Arg arg = emit(*stmt.expression, block);
    block.jump = tac::Jump::retVal(arg);
}


template<bool Yield, bool Await, bool Return>
void emit(const ast::Statement<Yield, Await, Return>& statement, tac::Block& block) {
    struct visitor {
        tac::Block& block;
        void operator()(const ast::BlockStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Block statements are not supported");
        }
        void operator()(const ast::VariableStatement<Yield, Await>&) {
            throw std::runtime_error("Variable statements are not supported");
        }
        void operator()(const ast::EmptyStatement&) {
            throw std::runtime_error("Empty statements are not supported");
        }
        void operator()(const ast::ExpressionStatement<Yield, Await>& statement) {
            emit(statement.expression, block);
        }
        void operator()(const ast::IfStatement<Yield, Await, Return>&) {
            throw std::runtime_error("If statements are not supported");
        }
        void operator()(const ast::BreakableStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Breakable statements are not supported");
        }
        void operator()(const ast::ContinueStatement<Yield, Await>&) {
            throw std::runtime_error("Continue statements are not supported");
        }
        void operator()(const ast::BreakStatement<Yield, Await>&) {
            throw std::runtime_error("Break statements are not supported");
        }
        void operator()(const ast::ReturnStatement<Yield, Await>& stmt) {
            emit(stmt, block);
        }
        void operator()(const ast::WithStatement<Yield, Await, Return>&) {
            throw std::runtime_error("With statements are not supported");
        }
        void operator()(const ast::LabeledStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Labeled statements are not supported");
        }
        void operator()(const ast::ThrowStatement<Yield, Await>&) {
            throw std::runtime_error("Throw statements are not supported");
        }
        void operator()(const ast::TryStatement<Yield, Await, Return>&) {
            throw std::runtime_error("Try statements are not supported");
        }
        void operator()(const ast::DebuggerStatement&) {
            throw std::runtime_error("Debugger statements are not supported");
        }
    };

    std::visit(visitor{block}, statement.value);
}


template<bool Yield, bool Await, bool Return>
tac::Block emit(const ast::StatementList<Yield, Await, Return>& list) {
    struct visitor {
        tac::Block& block;
        void operator()(const ast::Statement<Yield, Await, Return>& statement) {
            emit(statement, block);
        }
        void operator()(const ast::Declaration<Yield, Await, Return>& declaration) {
            emit(declaration, block);
        }
    };

    tac::Block block;
    for (const ast::StatementListItem<Yield, Await, Return>& statement : list.items) {
        std::visit(visitor{block}, statement.value);
    }
    return block;
}


template<bool Yield, bool Await, bool Default>
tac::Function emit(const ast::FunctionDeclaration<Yield, Await, Default>& decl) {
    tac::Function out;
    if (!decl.name) {
        throw std::runtime_error("Function declarations must have a name");
    }
    out.name = id(decl.name->identifier.name.name);

    out.returnType = tac::ValueType::Word;
    for (const ast::FormalParameter<Yield, Await>& arg : decl.parameters.parameterList) {
        if (!std::holds_alternative<ast::BindingIdentifier<Yield, Await>>(arg.value)) {
            throw std::runtime_error("Only binding identifiers are supported as function parameters");
        }
        const auto& binding = std::get<ast::BindingIdentifier<Yield, Await>>(arg.value);
        out.args.emplace_back(id(binding.identifier.name.name), tac::ValueType::Word);
    }

    out.body.blocks.push_back(tac::Block{
        .name = "start",
        .statements = {},
        .jump = tac::Jump::next()
    });

    auto block = emit(*decl.body);
    block.name = "f";
    out.body.blocks.push_back(block);

    out.body.blocks.push_back(tac::Block{
        .name = "end",
        .statements = {},
        .jump = tac::Jump::retVal(tac::Arg(0))
    });

    return out;
}


}  // namespace jac::tac