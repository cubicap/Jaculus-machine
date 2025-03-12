#pragma once

#include <cassert>
#include <cstdlib>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "opcode.h"


namespace jac::cfg {


using TmpId = int;
TmpId getTmpId();


using Identifier = std::string;
using MemberIdentifier = std::variant<Identifier, int32_t>;


/*
possible source:
    - literal
    - operator/function result
    - conversion from LVRef
*/
struct RValue {
    ValueType type;
    TmpId id;
};

/*
possible source:
    - variable
    - member access
*/
struct LVRef {  // FIXME: refactor
    ValueType type;  // type of the value itself
    TmpId id;        // id of target/parent
    std::optional<RValue> member;  // for member access
    ValueType parentType = ValueType::Void;
    bool _const = false;

    bool isMember() const {
        return member.has_value();
    }

    bool isConst() const {
        return _const;
    }

    static LVRef direct(ValueType type_, TmpId id_, bool isConst) {
        return { type_, id_, std::nullopt, ValueType::Void, isConst };
    }

    static LVRef mbr(ValueType type_, TmpId id_, RValue member_, ValueType parentType_, bool isConst) {
        return { type_, id_, member_, parentType_, isConst };
    }

    LVRef(): type(ValueType::Void), id(0) {}
private:
    LVRef(ValueType type_, TmpId id_, std::optional<RValue> member_, ValueType parentType_, bool isConst):
        type(type_), id(id_), member(member_), parentType(parentType_), _const(isConst)
    {}
};

struct Value {
    std::variant<RValue, LVRef> value;

    bool isRValue() const {
        return std::holds_alternative<RValue>(value);
    }
    LVRef& asLVRef() {
        return std::get<LVRef>(value);
    }
    RValue& asRValue() {
        return std::get<RValue>(value);
    }
};


struct Operation {  // FIXME: refactor
    Opcode op;

    RValue a;
    RValue b = { ValueType::Void, 0 };
    LVRef res;
};

struct ConstInit {
    TmpId id;
    std::variant<int32_t, double, bool, std::string> value;

    ValueType type() const {
        if (std::holds_alternative<int32_t>(value)) {
            return ValueType::I32;
        }
        else if (std::holds_alternative<double>(value)) {
            return ValueType::Double;
        }
        else if (std::holds_alternative<bool>(value)) {
            return ValueType::Bool;
        }
        else if (std::holds_alternative<std::string>(value)) {
            return ValueType::StringConst;
        }
        abort();
    }
};

struct Call {
    std::variant<Identifier, RValue> obj;  // native/object
    std::list<RValue> args;
    RValue res;

    bool isNative() const {
        return std::holds_alternative<Identifier>(obj);
    }
};

struct Statement {  // FIXME: rename, unify
    std::variant<Operation, ConstInit, Call> op;

    bool isOperation() const {
        return std::holds_alternative<Operation>(op);
    }

    Operation& asOperation() {
        return std::get<Operation>(op);
    }

    Call& asCall() {
        return std::get<Call>(op);
    }

    RValue res() {
        if (auto op_ = std::get_if<Operation>(&op)) {
            return { op_->res.type, op_->res.id };
        }
        else if (auto call = std::get_if<Call>(&op)) {
            return call->res;
        }
        else if (auto init = std::get_if<ConstInit>(&op)) {
            return { init->type(), init->id };
        }
        assert(false);
    }
};

struct BasicBlock;
using BasicBlockPtr = BasicBlock*;

struct Terminal {
    enum Type {
        Jump,
        Branch,
        Return,
        ReturnValue,
        Throw,
        None  // invalid jump type used for initialization
    };

    Type type;
    std::optional<RValue> value;
    BasicBlockPtr target;
    BasicBlockPtr other;

    static Terminal jump(BasicBlockPtr target) {
        return { Jump, {}, target, nullptr };
    }

    static Terminal branch(RValue condition, BasicBlockPtr target, BasicBlockPtr other) {
        return { Branch, condition, target, other };
    }

    static Terminal ret() {
        return { Return, {}, nullptr, nullptr };
    }

    static Terminal retVal(RValue retValue) {
        return { ReturnValue, retValue, nullptr, nullptr };
    }

    static Terminal throw_(RValue exception) {
        return { Throw, exception, nullptr, nullptr };
    }

    static Terminal none() {
        return { None, {}, nullptr, nullptr };
    }
};

struct BasicBlock {
    std::list<Statement> statements;
    Terminal jump = Terminal::none();
};


struct Scope {
    std::map<Identifier, std::tuple<TmpId, ValueType, bool>> locals;

    LVRef addLocal(Identifier name, ValueType type, bool isConst) {
        auto id = getTmpId();
        locals.emplace(name, std::forward_as_tuple(id, type, isConst));
        return LVRef::direct(type, id, isConst);
    }
    std::optional<LVRef> getLocal(Identifier name) {
        auto it = locals.find(name);
        if (it == locals.end()) {
            return std::nullopt;
        }
        auto& [ id, type, isConst ] = it->second;
        return LVRef::direct(type, id, isConst);
    }
    void insertLocal(Identifier name, LVRef ref) {
        assert(!ref.isMember());
        locals.emplace(name, std::forward_as_tuple(ref.id, ref.type, ref.isConst()));
    }
};

template<typename T>
struct ListPopper {
    std::forward_list<T>* list;

    ListPopper(std::forward_list<T>& list_): list(&list_) {}
    ListPopper(const ListPopper&) = delete;
    ListPopper(ListPopper&& other): list(other.list) { other.list = nullptr; }
    ListPopper& operator=(const ListPopper&) = delete;
    ListPopper& operator=(ListPopper&& other) {
        list = other.list;
        other.list = nullptr;
        return *this;
    }

    ~ListPopper() { list->pop_front(); }
};

struct Signature {
    std::vector<std::pair<Identifier, ValueType>> args;
    ValueType ret;
};
using SignaturePtr = std::shared_ptr<Signature>;

struct Function {
    BasicBlockPtr entry;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::vector<LVRef> args;
    ValueType ret;
    std::string _name;
    std::set<Identifier> requiredFunctions;

    std::string name() const { return _name; }
};

struct FunctionEmitter {  // TODO: make members private, think about lists once again
    const std::map<cfg::Identifier, cfg::SignaturePtr>& _otherSignatures;

    SignaturePtr signature;
    std::forward_list<Scope> scopes;
    std::list<std::pair<Identifier, LVRef>> undefined;
    BasicBlockPtr activeBlock;
    BasicBlockPtr entry;
    std::forward_list<BasicBlockPtr> breakTargets;
    std::forward_list<BasicBlockPtr> continueTargets;
    std::list<std::unique_ptr<BasicBlock>> blocks;
    std::set<Identifier> requiredFunctions;
    std::vector<LVRef> args;

    std::string _name;

    FunctionEmitter(const std::map<cfg::Identifier, cfg::SignaturePtr>& otherSignatures):
        _otherSignatures(otherSignatures),
        scopes(1)
    {
        entry = createBlock();
        activeBlock = entry;
    }

    void reset() {
        scopes = { Scope{} };
        activeBlock = nullptr;
        entry = nullptr;
        breakTargets.clear();
        continueTargets.clear();
        blocks.clear();

        entry = createBlock();
        activeBlock = entry;
    }

    void setSignature(SignaturePtr sig) {
        if (signature) {
            throw std::runtime_error("Signature already set");
        }
        signature = sig;
        for (const auto& [name, type] : sig->args) {
            auto ref = addLexical(name, type, false);
            args.push_back(ref);
        }
    }

    auto pushScope() { scopes.emplace_front(); return ListPopper(scopes); }
    LVRef addLexical(Identifier name, ValueType type, bool isConst) { return scopes.front().addLocal(name, type, isConst); }
    LVRef addVar(Identifier name, ValueType type) {
        for (auto it = undefined.begin(); it != undefined.end(); ++it) {
            if (it->first == name) {
                it = undefined.erase(it);
                scopes.front().insertLocal(name, it->second);
                return it->second;
            }
        }
        return scopes.front().addLocal(name, type, false);
    }
    LVRef addUndefined(Identifier name, ValueType type) {
        auto id = getTmpId();
        undefined.emplace_back(name, LVRef::direct(type, id, false));
        return undefined.back().second;
    }
    std::optional<LVRef> getLocal(Identifier name) {  // TODO: update to allow for shadowing
        for (auto& scope : scopes) {
            if (auto ref = scope.getLocal(name); ref.has_value()) {
                return ref;
            }
        }
        return std::nullopt;
    }

    void emitStatement(Statement statement) {
        activeBlock->statements.push_back(statement);
    }
    void emitStatement(auto&& statement) {
        emitStatement(Statement{ std::forward<decltype(statement)>(statement) });
    }

    void addRequiredFunction(Identifier name) {
        requiredFunctions.emplace(name);
    }
    SignaturePtr getSignature(Identifier name) {
        auto it = _otherSignatures.find(name);
        if (it == _otherSignatures.end()) {
            return nullptr;
        }
        return it->second;
    }

    BasicBlockPtr createBlock() {
        auto block = std::make_unique<BasicBlock>();
        blocks.push_back(std::move(block));
        return blocks.back().get();
    }
    BasicBlockPtr getActiveBlock() { return activeBlock; }
    void setActiveBlock(BasicBlockPtr block) { activeBlock = block; }

    auto pushBreakTarget(BasicBlockPtr block) {
        breakTargets.push_front(block);
        return ListPopper(breakTargets);
    }
    BasicBlockPtr getBreakTarget() {
        if (breakTargets.empty()) {
            throw std::runtime_error("No break target");
        }
        return breakTargets.front();
    }

    auto pushContinueTarget(BasicBlockPtr block) {
        continueTargets.push_front(block);
        return ListPopper(continueTargets);
    }
    BasicBlockPtr getContinueTarget() {
        if (continueTargets.empty()) {
            throw std::runtime_error("No continue target");
        }
        return continueTargets.front();
    }

    auto getEntry() const { return entry; }
    auto getEntry() { return entry; }

    void setEntry(BasicBlockPtr block) { entry = block; }

    Function output() {
        std::vector<std::unique_ptr<BasicBlock>> blockPtrs;
        blockPtrs.reserve(blocks.size());
        for (auto& block : blocks) {
            blockPtrs.emplace_back(std::move(block));
        }

        args.shrink_to_fit();

        Function out = {  // FIXME: move to a single attribute
            .entry = entry,
            .blocks = std::move(blockPtrs),
            .args = std::move(args),
            .ret = signature->ret,
            ._name = std::move(_name),
            .requiredFunctions = std::move(requiredFunctions)
        };

        reset();

        return out;
    }

    void setFunctionName(std::string name) {
        _name = name;
    }
};


}  // namespace jac::cfg
