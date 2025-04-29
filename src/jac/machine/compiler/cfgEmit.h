#pragma once

#include "ast.h"
#include "cfg.h"
#include "opcode.h"

#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>


namespace jac::cfg {


class IRGenError : public std::runtime_error {
public:
    explicit IRGenError(const std::string& message)
        : std::runtime_error(message) {}
};


SignaturePtr getSignature(const ast::FunctionDeclaration& decl);
FunctionEmitter emit(const ast::FunctionDeclaration& decl, SignaturePtr sig, const std::map<cfg::Identifier, cfg::SignaturePtr>& otherSignatures);


}  // namespace jac::cfg
