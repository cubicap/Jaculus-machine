#pragma once

#include <variant>
#include <vector>

#include "ast.h"


namespace jac::ast::traverse {


struct Functions {
    std::vector<const FunctionDeclaration*> functions;

    void add(const FunctionDeclaration& func) {
        functions.push_back(&func);
    }
};

inline void funcs(const Statement& statement, Functions& out) {
    // XXX: ignore for now (ignores nested functions)
}

inline void funcs(const HoistableDeclaration& hoistable, Functions& out) {
    if (!std::holds_alternative<FunctionDeclaration>(hoistable.value)) {
        return;
    }

    auto& func = std::get<FunctionDeclaration>(hoistable.value);
    out.add(func);
}

inline void funcs(const Declaration& declaration, Functions& out) {
    if (!std::holds_alternative<HoistableDeclaration>(declaration.value)) {
        return;
    }

    auto& hoistable = std::get<HoistableDeclaration>(declaration.value);
    funcs(hoistable, out);
}

inline void funcs(const StatementListItem& statement, Functions& out) {
    struct visitor {
        Functions& out;

        void operator()(const Statement& statement) {
            funcs(statement, out);
        }

        void operator()(const Declaration& declaration) {
            funcs(declaration, out);
        }
    };

    std::visit(visitor{out}, statement.value);
}


inline void funcs(const StatementList& statementList, Functions& out) {
    for (const auto& statement : statementList.items) {
        funcs(statement, out);
    }
}


inline void funcs(const Script& script, Functions& out) {
    if (!script.statementList) {
        return;
    }

    funcs(*script.statementList, out);
}


} // namespace jac::ast::traverse
