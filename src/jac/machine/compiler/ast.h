#pragma once

#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

#include "scanner.h"


namespace jac::ast {


struct Yield {
    const bool value = true;
};
struct Await {
    const bool value = true;
};
struct In {
    const bool value = true;
};
struct Return {
    const bool value = true;
};


template<typename... Stacks>
struct PushPopper {
    std::tuple<Stacks...> stacks;

    PushPopper(auto... stacks_):
        stacks(stacks_.first...)
    {
        (stacks_.first.push_back(stacks_.second), ...);
    }

    ~PushPopper() {
        std::apply([](auto&... stacks_) {
            (stacks_.pop_back(), ...);
        }, stacks);
    }
};
template<typename... Args>
PushPopper(Args... args) -> PushPopper<decltype(args.first)...>;


class ParserState {
    std::span<lex::Token> _tokens;
    std::span<lex::Token>::iterator _pos;

    lex::Token _errorToken = lex::Token(0, 0, "", lex::Token::NoToken);
    std::string_view _errorMessage;

    std::vector<bool> yieldStack;
    std::vector<bool> awaitStack;
    std::vector<bool> inStack;
    std::vector<bool> returnStack;

    template<typename Arg>
    auto& getStack() {
        if constexpr (std::is_same_v<Arg, Yield>) {
            return yieldStack;
        }
        else if constexpr (std::is_same_v<Arg, Await>) {
            return awaitStack;
        }
        else if constexpr (std::is_same_v<Arg, In>) {
            return inStack;
        }
        else if constexpr (std::is_same_v<Arg, Return>) {
            return returnStack;
        }
    }
public:
    ParserState(std::span<lex::Token> tokens):
        _tokens(tokens),
        _pos(_tokens.begin())
    {}

    void error(std::string_view message) {
        if (_tokens.empty()) {
            _errorMessage = message;
            return;
        }
        if (_pos == _tokens.end()) { // TODO: fix token position
            _errorToken = _tokens.back();
            _errorMessage = message;
            return;
        }
        if (_errorToken.text.begin() > _pos->text.begin()) {
            return;
        }
        _errorToken = current();
        _errorMessage = message;
    }

    lex::Token current() {
        if (_tokens.empty()) {
            return lex::Token(0, 0, "", lex::Token::NoToken);
        }
        if (_pos == _tokens.end()) {
            auto pos = _tokens.back().text.end();
            return lex::Token(_tokens.back().line + 1, 0, std::string_view(pos, pos), lex::Token::NoToken);
        }
        return *_pos;
    }

    void advance() {
        if (_pos == _tokens.end()) {
            error("Unexpected end of input");
            return;
        }
        ++_pos;
    }

    void backtrack() {
        assert(_pos != _tokens.begin());
        --_pos;
    }

    bool isEnd() {
        return _pos == _tokens.end();
    }

    auto getPosition() const {
        return _pos;
    }

    void restorePosition(auto position) {
        _pos = position;
    }

    std::string_view getErrorMessage() const {
        return _errorMessage;
    }

    lex::Token getErrorToken() const {
        return _errorToken;
    }

    bool getYield() const {
        if (yieldStack.empty()) {
            return false;
        }
        return yieldStack.back();
    }
    bool getAwait() const {
        if (awaitStack.empty()) {
            return false;
        }
        return awaitStack.back();
    }
    bool getIn() const {
        if (inStack.empty()) {
            return false;
        }
        return inStack.back();
    }
    bool getReturn() const {
        if (returnStack.empty()) {
            return false;
        }
        return returnStack.back();
    }

    template<auto... Args>
    auto pushTemplate() {
        auto make = [&](auto arg) {
            auto& stack = getStack<std::decay_t<decltype(arg)>>();
            return std::pair<decltype(stack), bool>(stack, arg.value);
        };
        return PushPopper(make(Args)...);
    }
};


using IdentifierName = std::string;


struct Identifier {
    IdentifierName name;

    static std::optional<Identifier> parse(ParserState& state);
};

struct IdentifierReference {
    Identifier identifier;

    static std::optional<IdentifierReference> parse(ParserState& state);
};

struct BindingIdentifier {
    Identifier identifier;

    static std::optional<BindingIdentifier> parse(ParserState& state);
};

struct LabelIdentifier {
    Identifier identifier;

    static std::optional<LabelIdentifier> parse(ParserState& state);
};

struct PrivateIdentifier {
    IdentifierName name;

    static std::optional<PrivateIdentifier> parse(ParserState& state);
};


struct ThisExpr {
    static std::optional<ThisExpr> parse(ParserState& state);
};

struct NullLiteral {
    static std::optional<NullLiteral> parse(ParserState& state);
};

struct BooleanLiteral {
    bool value;

    static std::optional<BooleanLiteral> parse(ParserState& state);
};

struct NumericLiteral {
    std::variant<std::int32_t, double> value;  // TODO: bigint

    static std::optional<NumericLiteral> parse(ParserState& state);
};

struct StringLiteral {
    std::string value;

    static std::optional<StringLiteral> parse(ParserState& state);
};

struct Literal {
    std::variant<NullLiteral, BooleanLiteral, NumericLiteral, StringLiteral> value;

    static std::optional<Literal> parse(ParserState& state);
};

struct AssignmentExpression;

using AssignmentExpressionPtr = std::unique_ptr<AssignmentExpression>;

using InitializerPtr = AssignmentExpressionPtr;

struct LeftHandSideExpression;
using LeftHandSideExpressionPtr = std::unique_ptr<LeftHandSideExpression>;

struct UnaryExpression;

using UnaryExpressionPtr = std::unique_ptr<UnaryExpression>;


enum class UpdateKind {
    None,
    PreInc,
    PreDec,
    PostInc,
    PostDec
};

struct UpdateExpression {
    std::variant<
        UnaryExpressionPtr,
        LeftHandSideExpressionPtr
    > value;

    UpdateKind kind;

    static std::optional<UpdateExpression> parse(ParserState& state);
};

struct UnaryExpression {
    std::variant<
        std::pair<UnaryExpressionPtr, std::string_view>,
        UpdateExpression
    > value;

    static std::optional<UnaryExpression> parse(ParserState& state);
};

struct BinaryExpression {
    using BinaryExpressionPtr = std::unique_ptr<BinaryExpression>;

    std::variant<
        std::tuple<BinaryExpressionPtr, BinaryExpressionPtr, std::string_view>,
        UnaryExpressionPtr
    > value;

    static std::optional<BinaryExpression> parse(ParserState& state);
};

using BinaryExpressionPtr = std::unique_ptr<BinaryExpression>;

struct ConditionalExpression {
    std::variant<
        BinaryExpression,
        std::tuple<  // ternary conditional operator
            BinaryExpression,
            AssignmentExpressionPtr,
            AssignmentExpressionPtr
        >
    > value;

    static std::optional<ConditionalExpression> parse(ParserState& state);
};

struct Elision {};

struct ArrayLiteral {
    // AssignmentExpression[+In, Yield, Await]
    std::vector<std::variant<Elision, AssignmentExpressionPtr>> elementList;
    std::optional<AssignmentExpressionPtr> spreadElement;
};

struct ComputedPropertyName {
    // AssignmentExpression[+In, Yield, Await]
    AssignmentExpressionPtr expression;
};

struct PropertyName {
    std::variant<
        IdentifierName,
        StringLiteral,
        NumericLiteral,
        ComputedPropertyName
    > value;
};

struct ClassElementName {
    std::variant<
        PropertyName,
        PrivateIdentifier
    > value;
};

struct BindingProperty;

struct BindingElement;

struct ObjectBindingPattern {
    std::vector<BindingProperty> properties;
    std::optional<BindingIdentifier> rest;
};

struct ArrayBindingPattern {
    std::vector<std::variant<
        Elision,
        BindingElement
    >> elements;
    std::optional<BindingIdentifier> rest;
};

struct BindingPattern {
    std::variant<
        ObjectBindingPattern,
        ArrayBindingPattern
    > value;

    static std::optional<BindingPattern> parse(ParserState&);

    BindingPattern(BindingPattern&&);
    ~BindingPattern();
};


// XXX: EXTENSION, incompatible with standard
struct TypeAnnotation {
    IdentifierName type;

    static std::optional<TypeAnnotation> parse(ParserState& state);
};

struct BindingElement {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,  // Initializer[+In, Yield, Await]
        BindingPattern,
        std::pair<BindingPattern, InitializerPtr>  // Initializer[+In, Yield, Await]
    > value;
    std::optional<TypeAnnotation> type;

    static std::optional<BindingElement> parse(ParserState& state);
};

struct BindingProperty {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,  // Initializer[+In, Yield, Await]
        std::pair<PropertyName, BindingElement>
    > value;
};

using FormalParameter = BindingElement;

struct BindingRestElement {
    std::variant<
        BindingIdentifier,
        BindingPattern
    > value;

    static std::optional<BindingRestElement> parse(ParserState&);
};

struct FormalParameters {
    std::vector<FormalParameter> parameterList;
    std::optional<BindingRestElement> restParameter;

    static std::optional<FormalParameters> parse(ParserState& state);
};

struct UniqueFormalParameters {
    FormalParameters parameters;
};

struct Statement;

using StatementPtr = std::unique_ptr<Statement>;

struct StatementListItem;

struct StatementList {
    std::vector<StatementListItem> items;

    static std::optional<StatementList> parse(ParserState& state);

    ~StatementList();
    StatementList();
    StatementList(StatementList&&);
};

struct FunctionBody {
    StatementList statementList;  // StatementList[Yield, Await, +Return]

    static std::optional<FunctionBody> parse(ParserState& state);
};

using GeneratorBody = FunctionBody;         // FunctionBody[+Yield, -Await]
using AsyncFunctionBody = FunctionBody;     // FunctionBody[-Yield, +Await]
using AsyncGeneratorBody = FunctionBody;     // FunctionBody[+Yield, +Await]

struct Expression {
    std::vector<AssignmentExpressionPtr> items;

    static std::optional<Expression> parse(ParserState& state);

    static std::optional<Expression> parseParenthesised(ParserState& state);
};

struct Block {
    std::optional<StatementList> statementList;

    static std::optional<Block> parse(ParserState& state);
};

struct LexicalBinding {
    std::variant<
        BindingIdentifier,
        std::pair<BindingIdentifier, InitializerPtr>,
        std::pair<BindingPattern, InitializerPtr>
    > value;
    std::optional<TypeAnnotation> type;  // TODO: maybe move to BindingIdentifier?

    static std::optional<LexicalBinding> parse(ParserState& state);
};

struct LexicalDeclaration {
    bool isConst;
    std::vector<LexicalBinding> bindings;

    static std::optional<LexicalDeclaration> parse(ParserState& state);
};

using BlockStatement = Block;

struct VariableDeclaration {
    BindingIdentifier identifier;
    std::optional<InitializerPtr> initializer;
};

struct VariableDeclarationList {
    std::vector<std::variant<VariableDeclaration>> declarations;

};

struct VariableStatement {
    VariableDeclarationList declarationList;  // VariableDeclarationList[+In, Yield, Await]

    static std::optional<VariableStatement> parse(ParserState&);
};

struct EmptyStatement {
    static std::optional<EmptyStatement> parse(ParserState& state);
};

struct ExpressionStatement {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ExpressionStatement> parse(ParserState& state);
};

struct IfStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    StatementPtr consequent;
    std::optional<StatementPtr> alternate;

    static std::optional<IfStatement> parse(ParserState& state);
};

struct DoWhileStatement {
    StatementPtr statement;
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<DoWhileStatement> parse(ParserState& state);
};

struct WhileStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    StatementPtr statement;

    static std::optional<WhileStatement> parse(ParserState& state);
};

struct ForStatement {
    std::variant<
        std::monostate,
        Expression,               // Expression[-In, Yield, Await]
        VariableDeclarationList,  // VariableDeclarationList[-In, Yield, Await]
        LexicalDeclaration        // LexicalDeclaration[-In, Yield, Await]
    > init;
    std::optional<Expression> condition;  // Expression[+In, Yield, Await]
    std::optional<Expression> update;     // Expression[+In, Yield, Await]
    StatementPtr statement;

    static std::optional<ForStatement> parse(ParserState& state);
};

struct ForInOfStatement {
    // XXX: ignore for now

    static std::optional<ForInOfStatement> parse(ParserState&);
};

struct IterationStatement {
    std::variant<
        DoWhileStatement,
        WhileStatement,
        ForStatement,
        ForInOfStatement
    > value;

    static std::optional<IterationStatement> parse(ParserState& state);
};

struct CaseClause {
    Expression expression;  // Expression[+In, Yield, Await]
    std::optional<StatementList> statementList;
};

struct DefaultClause {
    std::optional<StatementList> statementList;
};

struct SwitchStatement {
    Expression expression;  // Expression[+In, Yield, Await]
    std::vector<CaseClause> caseBlock;
    std::optional<DefaultClause> defaultClause;

    static std::optional<SwitchStatement> parse(ParserState&);
};

struct BreakableStatement {
    std::variant<
        IterationStatement,
        SwitchStatement
    > value;

    static std::optional<BreakableStatement> parse(ParserState& state);
};

struct ContinueStatement {
    std::optional<LabelIdentifier> label;

    static std::optional<ContinueStatement> parse(ParserState& state);
};

struct BreakStatement {
    std::optional<LabelIdentifier> label;

    static std::optional<BreakStatement> parse(ParserState& state);
};

struct ReturnStatement {
    std::optional<Expression> expression;  // Expression[+In, Yield, Await]

    static std::optional<ReturnStatement> parse(ParserState& state);
};

struct WithStatement {
    // XXX: ignore for now

    static std::optional<WithStatement> parse(ParserState&);
};

struct FunctionDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;       // FormalParameters[-Yield, -Await]
    std::optional<FunctionBody> body;  // FunctionBody[-Yield, -Await]
    std::optional<TypeAnnotation> returnType;

    std::string_view code;

    static std::optional<FunctionDeclaration> parse(ParserState& state, bool Default);
};

struct LabeledStatement {
    LabelIdentifier label;
    std::variant<
        StatementPtr,
        FunctionDeclaration  // FunctionDeclaration[Yield, Await, -Default]
    > statement;

    static std::optional<LabeledStatement> parse(ParserState&);
};

struct ThrowStatement {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ThrowStatement> parse(ParserState&);
};

struct CatchParameter {
    std::variant<
        BindingIdentifier,
        BindingPattern
    > value;
};

struct Catch {
    CatchParameter parameter;
    Block block;
};

struct TryStatement {
    Block block;
    std::optional<Catch> catchClause;
    std::optional<Block> finallyClause;

    static std::optional<TryStatement> parse(ParserState&);
};

struct DebuggerStatement {
    static std::optional<DebuggerStatement> parse(ParserState& state);
};

struct Statement {
    std::variant<
        BlockStatement,
        VariableStatement,
        EmptyStatement,
        ExpressionStatement,
        IfStatement,
        BreakableStatement,
        ContinueStatement,
        BreakStatement,
        ReturnStatement, // [+Return]
        WithStatement,
        LabeledStatement,
        ThrowStatement,
        TryStatement,
        DebuggerStatement
    > value;

    static std::optional<Statement> parse(ParserState& state);
};

struct GeneratorDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    GeneratorBody body;

    static std::optional<GeneratorDeclaration> parse(ParserState&, bool);
};

struct AsyncFunctionDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    AsyncFunctionBody body;

    static std::optional<AsyncFunctionDeclaration> parse(ParserState&, bool);
};

struct AsyncGeneratorDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    FormalParameters parameters;  // FormalParameters[-Yield, -Await]
    AsyncGeneratorBody body;

    static std::optional<AsyncGeneratorDeclaration> parse(ParserState&, bool);
};

struct HoistableDeclaration {
    std::variant<
        FunctionDeclaration,
        GeneratorDeclaration,
        AsyncFunctionDeclaration,
        AsyncGeneratorDeclaration
    > value;

    static std::optional<HoistableDeclaration> parse(ParserState& state, bool Default);
};

struct ClassHeritage {
    LeftHandSideExpressionPtr value;
};

struct ClassStaticBlock {
    std::optional<StatementList> statementList;  // StatementList[-Yield, -Await, -Return]
};

struct GetFunctionBody : public FunctionBody {};  // FunctionBody[-Yield, -Await]
struct SetFunctionBody : public FunctionBody {};  // FunctionBody[-Yield, -Await]

struct MethodDefinition {

    std::variant<  // UniqueFormalParameters[-Yield, -Await]
        std::tuple<ClassElementName, UniqueFormalParameters, FunctionBody>,  // FunctionBody[-Yield, -Await]
        std::tuple<ClassElementName, UniqueFormalParameters, GeneratorBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, AsyncFunctionBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, AsyncGeneratorBody>,
        std::tuple<ClassElementName, GetFunctionBody>,
        std::tuple<ClassElementName, UniqueFormalParameters, SetFunctionBody>
    > value;
};

struct FieldDefinition {
    ClassElementName name;
    std::optional<InitializerPtr> initializer;  // Initializer[+In, Yield, Await]
};

struct ClassElement {
    std::variant<
        std::monostate,  // semicolon
        std::pair<bool, MethodDefinition>,  // bool: static
        std::pair<bool, FieldDefinition>,  // bool: static
        ClassStaticBlock
    > value;
};

struct ClassBody {
    std::vector<ClassElement> elements;
};

struct ClassDeclaration {
    std::optional<BindingIdentifier> name;  // optional only when [+Default]
    std::optional<ClassHeritage> heritage;
    ClassBody body;

    static std::optional<ClassDeclaration> parse(ParserState&, bool);
};

struct Declaration {
    std::variant<
        HoistableDeclaration,  // HoistableDeclaration[Yield, Await, -Default]
        ClassDeclaration,      // ClassDeclaration[Yield, Await, -Default]
        LexicalDeclaration     // LexicalDeclaration[+In, Yield, Await]
    > value;

    static std::optional<Declaration> parse(ParserState& state);
};

struct StatementListItem {
    std::variant<
        Statement,
        Declaration
    > value;

    StatementListItem(Statement statement): value(std::move(statement)) {}
    StatementListItem(Declaration declaration): value(std::move(declaration)) {}

    static std::optional<StatementListItem> parse(ParserState& state);
};

struct CoverInitializedName {
    IdentifierReference identifier;
    InitializerPtr initializer;  // Initializer[+In, Yield, Await]
};

struct PropertyDefinition {
    using SpreadAssignmentExpressionPtr = AssignmentExpressionPtr;  // AssignmentExpression[+In, Yield, Await]

    std::variant<
        IdentifierReference,
        CoverInitializedName,
        std::pair<PropertyName, AssignmentExpressionPtr>,  // AssignmentExpression[+In, Yield, Await]
        MethodDefinition,
        SpreadAssignmentExpressionPtr
    > value;
};

struct ObjectLiteral {
    std::vector<PropertyDefinition> properties;
};

struct FunctionExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    FunctionBody body;                      // FunctionBody[-Yield, -Await]
};

struct ClassExpression {
    std::optional<BindingIdentifier> name;
    std::optional<ClassHeritage> heritage;
    ClassBody body;
};

struct GeneratorExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    GeneratorBody body;
};

struct AsyncFunctionExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    AsyncFunctionBody body;
};

struct AsyncGeneratorExpression {
    std::optional<BindingIdentifier> name;  // BindingIdentifier[-Yield, -Await]
    FormalParameters parameters;            // FormalParameters[-Yield, -Await]
    AsyncGeneratorBody body;
};

struct RegularExpressionLiteral {
    // XXX: ignore for now

    static std::optional<RegularExpressionLiteral> parse(ParserState&);
};

struct TemplateLiteral {
    // XXX: ignore for now

    static std::optional<TemplateLiteral> parse(ParserState&, bool);
};

struct ParenthesizedExpression {
    Expression expression;  // Expression[+In, Yield, Await]
};

struct CoverParenthesizedExpressionAndArrowParameterList {
    std::optional<Expression> expression;  // Expression[+In, Yield, Await]
    std::variant<
        std::monostate,  // no parameters
        BindingIdentifier,
        BindingPattern
    > parameters;

    static std::optional<CoverParenthesizedExpressionAndArrowParameterList> parse(ParserState& state);

    std::optional<ParenthesizedExpression> refineParExp();
};

struct PrimaryExpression {
    std::variant<
        ThisExpr,
        IdentifierReference,
        Literal,
        ArrayLiteral,
        ObjectLiteral,
        FunctionExpression,
        ClassExpression,
        GeneratorExpression,
        AsyncFunctionExpression,
        AsyncGeneratorExpression,
        RegularExpressionLiteral,
        TemplateLiteral,  // TemplateLiteral[In, Yield, -Tagged]
        ParenthesizedExpression  // TODO: check if ok
    > value;

    static std::optional<PrimaryExpression> parse(ParserState& state);
};

struct SuperProperty {
    std::variant<
        IdentifierName,  // dot
        Expression  // bracket; Expression[+In, Yield, Await]
    > value;

    static std::optional<SuperProperty> parse(ParserState&);
};

struct MetaProperty {
    enum Kind {
        NewTarget,
        ImportMeta
    };

    Kind kind;

    static std::optional<MetaProperty> parse(ParserState&);
};

struct Arguments {
    std::vector<std::pair<bool, AssignmentExpressionPtr>> arguments;  // bool: spread; AssignmentExpression[+In, Yield, Await]

    static std::optional<Arguments> parse(ParserState& state);
};

struct MemberExpression {
    using MemberExpressionPtr = std::unique_ptr<MemberExpression>;

    std::variant<
        SuperProperty,
        MetaProperty,
        PrimaryExpression,
        std::pair<MemberExpressionPtr, Expression>,  // bracket; Expression[+In, Yield, Await]
        std::pair<MemberExpressionPtr, IdentifierName>, // dot
        std::pair<MemberExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<MemberExpressionPtr, TemplateLiteral>,  // tag; TemplateLiteral[In, Yield, +Tagged]
        std::pair<MemberExpressionPtr, Arguments>  // new
    > value;

    static std::optional<MemberExpression> parse(ParserState& state);
};
using MemberExpressionPtr = std::unique_ptr<MemberExpression>;


struct NewExpression;

using NewExpressionPtr = std::unique_ptr<NewExpression>;

struct NewExpression {

    std::variant<
        MemberExpression,
        NewExpressionPtr
    > value;

    static std::optional<NewExpression> parse(ParserState& state);
};
using NewExpressionPtr = std::unique_ptr<NewExpression>;

struct SuperCall {
    Arguments arguments;

    static std::optional<SuperCall> parse(ParserState&);
};

struct ImportCall {
    Expression expression;  // Expression[+In, Yield, Await]

    static std::optional<ImportCall> parse(ParserState&);
};

struct CoverCallExpressionAndAsyncArrowHead {
    MemberExpression memberExpression;
    Arguments arguments;

    static std::optional<CoverCallExpressionAndAsyncArrowHead> parse(ParserState& state);
};

struct CallExpression {
    using CallExpressionPtr = std::unique_ptr<CallExpression>;

    // grammar: CoverCallExpressionAndAsyncArrowHead
    std::variant<
        SuperCall,
        ImportCall,
        std::pair<MemberExpression, Arguments>,  // cover grammar
        std::pair<CallExpressionPtr, Arguments>,
        std::pair<CallExpressionPtr, Expression>,  // bracket; Expression[+In, Yield, Await]
        std::pair<CallExpressionPtr, IdentifierName>,  // dot
        std::pair<CallExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<CallExpressionPtr, TemplateLiteral>  // tag; TemplateLiteral[In, Yield, +Tagged]
    > value;

    static std::optional<CallExpression> parse(ParserState& state);
};

using CallExpressionPtr = std::unique_ptr<CallExpression>;

struct OptionalChain {
    using OptionalChainPtr = std::unique_ptr<OptionalChain>;

    std::optional<OptionalChainPtr> chain;
    std::variant<
        Arguments,  // ?.
        Expression,  // ?.[   Expression[+In, Yield, Await]
        IdentifierName,  // ?.identifier
        PrivateIdentifier,  // ?.#identifier
        TemplateLiteral  // ?.tag   // TemplateLiteral[In, Yield, +Tagged]
    > value;
};

struct OptionalExpression {
    using OptionalExpressionPtr = std::unique_ptr<OptionalExpression>;

    std::variant<
        MemberExpression,
        CallExpression,
        OptionalExpressionPtr
    > value;
    OptionalChain chain;
};

struct LeftHandSideExpression {
    std::variant<
        CallExpression,
        NewExpression
        // OptionalExpression
    > value;

    static std::optional<LeftHandSideExpression> parse(ParserState& state);
};

struct YieldExpression {
    bool star;
    std::optional<AssignmentExpressionPtr> expression;

    static std::optional<YieldExpression> parse(ParserState&);
};

struct ArrowParameters {
    std::variant<
        BindingIdentifier,
        CoverParenthesizedExpressionAndArrowParameterList
    > value;
};

struct ConciseBody {
    std::variant<
        AssignmentExpressionPtr,  // lookahead not {, AssignmentExpression[In, -Yield, -Await]
        FunctionBody  // FunctionBody[-Yield, -Await]
    > value;
};

struct ArrowFunction {
    ArrowParameters parameters;
    ConciseBody body;

    static std::optional<ArrowFunction> parse(ParserState&);
};

struct AsyncConciseBody {
    std::variant<
        AssignmentExpressionPtr,  // lookahead not {, AssignmentExpression[In, -Yield, -Await]
        AsyncFunctionBody
    > value;
};

struct AsyncArrowFunction {
    std::variant<
        BindingIdentifier,  // BindingIdentifier[Yield, +Await]
        CoverCallExpressionAndAsyncArrowHead
    > parameters;
    AsyncConciseBody body;

    static std::optional<AsyncArrowFunction> parse(ParserState&);
};

struct Assignment {
    LeftHandSideExpression lhs;
    AssignmentExpressionPtr rhs;
    std::string_view op;

    static std::optional<Assignment> parse(ParserState& state);
};

struct AssignmentExpression {
    std::variant<
        ConditionalExpression,
        YieldExpression,  // only when [+Yield]
        ArrowFunction,
        AsyncArrowFunction,
        Assignment
    > value;

    static std::optional<AssignmentExpression> parse(ParserState& state);
};


struct Script {
    std::optional<StatementList> statementList;  // StatementList[-Yield, -Await, -Return]

    static std::optional<Script> parse(ParserState& state);
};

struct ImportDeclaration {
    // XXX: ignore for now
};

struct ExportDeclaration {
    // XXX: ignore for now
};

struct ModuleItem {
    std::variant<
        ImportDeclaration,
        ExportDeclaration,
        StatementListItem  // StatementListItem[-Yield, +Await, -Return]
    > value;
};

struct ModuleItemList {
    std::vector<ModuleItem> items;
};

struct Module {
    std::optional<ModuleItemList> moduleItemList;
};


inline StatementList::~StatementList() = default;

inline StatementList::StatementList() = default;

inline StatementList::StatementList(StatementList&&) = default;

inline BindingPattern::~BindingPattern() = default;

inline BindingPattern::BindingPattern(BindingPattern&&) = default;


}  // namespace jac::ast
