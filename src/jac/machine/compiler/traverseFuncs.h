#pragma once

#include <variant>
#include <vector>

#include "ast.h"


namespace jac::ast::traverse {


struct Functions {
    std::vector<const FunctionDeclaration<false, false, false>*> functions;

    template<bool Yield, bool Await, bool Default>
    void add(const FunctionDeclaration<Yield, Await, Default>& func) {
        if constexpr (!Yield && !Await && !Default) {
            functions.push_back(&func);
        }
    }
};

template<bool Yield, bool Await, bool Return>
void funcs(const Statement<Yield, Await, Return>& statement, Functions& out) {
    // TODO
}

template<bool Yield, bool Await, bool Default>
void funcs(const HoistableDeclaration<Yield, Await, Default>& hoistable, Functions& out) {
    if (!std::holds_alternative<FunctionDeclaration<Yield, Await, Default>>(hoistable.value)) {
        return;
    }

    auto& func = std::get<FunctionDeclaration<Yield, Await, Default>>(hoistable.value);
    out.add(func);
}

template<bool Yield, bool Await, bool Return>
void funcs(const Declaration<Yield, Await, Return>& declaration, Functions& out) {
    if (!std::holds_alternative<HoistableDeclaration<Yield, Await, false>>(declaration.value)) {
        return;
    }

    auto& hoistable = std::get<HoistableDeclaration<Yield, Await, false>>(declaration.value);
    funcs(hoistable, out);
}

template<bool Yield, bool Await, bool Return>
void funcs(const StatementListItem<Yield, Await, Return>& statement, Functions& out) {
    struct visitor {
        Functions& out;

        void operator()(const Statement<Yield, Await, Return>& statement) {
            funcs(statement, out);
        }

        void operator()(const Declaration<Yield, Await, Return>& declaration) {
            funcs(declaration, out);
        }
    };

    std::visit(visitor{out}, statement.value);
}


template<bool Yield, bool Await, bool Return>
void funcs(const StatementList<Yield, Await, Return>& statementList, Functions& out) {
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
