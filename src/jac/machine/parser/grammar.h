#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "scanner.h"


namespace jac {


struct IdentifierName {
    std::string name;
};

struct Identifier {
    IdentifierName name;
};

template<bool Yield, bool Await>
struct IdentifierReference {
    Identifier identifier;
};

template<bool Yield, bool Await>
struct BindingIdentifier {
    Identifier identifier;
};

template<bool Yield, bool Await>
struct LabelIdentifier {
    Identifier identifier;
};

struct PrivateIdentifier {
    IdentifierName name;
};


struct ThisExpr {};

struct NullLiteral {};

struct BooleanLiteral {
    bool value;
};

struct NumericLiteral {
    std::variant<int32_t, double> value;  // TODO: bigint
};

struct StringLiteral {
    std::string value;
};

struct Literal {
    std::variant<NullLiteral, BooleanLiteral, NumericLiteral, StringLiteral> value;
};

template<bool In, bool Yield, bool Await>
struct AssignmentExpression;

template<bool In, bool Yield, bool Await>
using AssignmentExpressionPtr = std::unique_ptr<AssignmentExpression<In, Yield, Await>>;

template<bool In, bool Yield, bool Await>
using InitializerPtr = AssignmentExpressionPtr<In, Yield, Await>;

template<bool Yield, bool Await>
struct LeftHandSideExpression;
template<bool Yield, bool Await>
using LeftHandSideExpressionPtr = std::unique_ptr<LeftHandSideExpression<Yield, Await>>;

template<bool In, bool Yield, bool Await>
struct Assignment {
    LeftHandSideExpression<Yield, Await> lhs;
    AssignmentExpressionPtr<In, Yield, Await> rhs;
    std::string_view op;
};

template<bool Yield, bool Await>
struct UnaryExpression {
    using UnaryExpressionPtr = std::unique_ptr<UnaryExpression<Yield, Await>>;

    std::variant<
        UnaryExpressionPtr,
        LeftHandSideExpressionPtr<Yield, Await>
    > value;

    std::string_view op;
};

template<bool In, bool Yield, bool Await>
struct BinaryExpression {
    using BinaryExpressionPtr = std::unique_ptr<BinaryExpression<In, Yield, Await>>;

    std::variant<
        BinaryExpressionPtr,  // with higher precedence
        UnaryExpression<Yield, Await>
    > lhs;
    std::variant<
        BinaryExpressionPtr,  // with higher precedence
        UnaryExpression<Yield, Await>
    > rhs;
    std::string_view op;
};

template<bool In, bool Yield, bool Await>
struct ShortCircuitExpression {
    BinaryExpression<In, Yield, Await> expression;
};

template<bool In, bool Yield, bool Await>
struct ConditionalExpression {
    std::variant<
        ShortCircuitExpression<In, Yield, Await>,
        std::tuple<  // ternary conditional operator
            ShortCircuitExpression<In, Yield, Await>,
            AssignmentExpressionPtr<In, Yield, Await>,
            AssignmentExpressionPtr<In, Yield, Await>
        >
    > value;
};

struct Elision {};

template<bool Yield, bool Await>
struct ArrayLiteral {
    std::vector<std::variant<Elision, AssignmentExpressionPtr<true, Yield, Await>>> elementList;
    std::optional<AssignmentExpressionPtr<true, Yield, Await>> spreadElement;
};

template<bool Yield, bool Await>
struct ComputedPropertyName {
    AssignmentExpressionPtr<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct PropertyName {
    std::variant<
        IdentifierName,
        StringLiteral,
        NumericLiteral,
        ComputedPropertyName<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct ClassElementName {
    std::variant<
        PropertyName<Yield, Await>,
        PrivateIdentifier
    > value;
};

template<bool Yield, bool Await>
struct BindingProperty;

template<bool Yield, bool Await>
struct BindingElement;

template<bool Yield, bool Await>
struct ObjectBindingPattern {
    std::vector<BindingProperty<Yield, Await>> properties;
    std::optional<BindingIdentifier<Yield, Await>> rest;
};

template<bool Yield, bool Await>
struct ArrayBindingPattern {
    std::vector<std::variant<
        Elision,
        BindingElement<Yield, Await>
    >> elements;
    std::optional<BindingIdentifier<Yield, Await>> rest;
};

template<bool Yield, bool Await>
struct BindingPattern {
    std::variant<
        ObjectBindingPattern<Yield, Await>,
        ArrayBindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct BindingElement {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<true, Yield, Await>>,
        BindingPattern<Yield, Await>,
        std::pair<BindingPattern<Yield, Await>, InitializerPtr<true, Yield, Await>>
    > value;
};

template<bool Yield, bool Await>
struct BindingProperty {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<true, Yield, Await>>,
        std::pair<PropertyName<Yield, Await>, BindingElement<Yield, Await>>
    > value;
};

template<bool Yield, bool Await>
using FormalParameter = BindingElement<Yield, Await>;

template<bool Yield, bool Await>
struct BindingRestElement {
    std::variant<
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct FormalParameters {
    std::vector<FormalParameter<Yield, Await>> parameterList;
    std::optional<BindingRestElement<Yield, Await>> restParameter;
};

template<bool Yield, bool Await>
struct UniqueFormalParameters {
    FormalParameters<Yield, Await> parameters;
};

template<bool Yield, bool Await, bool Return>
struct Statement;

template<bool Yield, bool Await, bool Return>
using StatementPtr = std::unique_ptr<Statement<Yield, Await, Return>>;

template<bool Yield, bool Await, bool Return>
struct StatementListItem;

template<bool Yield, bool Await, bool Return>
struct StatementList {
    std::vector<StatementListItem<Yield, Await, Return>> items;
};

template<bool Yield, bool Await>
using FunctionBody = StatementList<Yield, Await, true>;
using GeneratorBody = FunctionBody<false, false>;
using AsyncFunctionBody = FunctionBody<false, false>;
using AsyncGeneratorBody = FunctionBody<false, false>;

template<bool In, bool Yield, bool Await>
struct Expression {
    std::vector<AssignmentExpressionPtr<In, Yield, Await>> items;
};

template<bool Yield, bool Await, bool Return>
struct Block {
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool In, bool Yield, bool Await>
struct LexicalBinding {
    std::variant<
        BindingIdentifier<Yield, Await>,
        std::pair<BindingIdentifier<Yield, Await>, InitializerPtr<In, Yield, Await>>,
        std::pair<BindingPattern<Yield, Await>, InitializerPtr<In, Yield, Await>>
    > value;
};

template<bool In, bool Yield, bool Await>
struct LexicalDeclaration {
    bool isConst;
    std::vector<LexicalBinding<In, Yield, Await>> bindings;
};

template<bool Yield, bool Await, bool Return>
using BlockStatement = Block<Yield, Await, Return>;

template<bool In, bool Yield, bool Await>
struct VariableDeclaration {
    BindingIdentifier<Yield, Await> identifier;
    std::optional<InitializerPtr<In, Yield, Await>> initializer;
};

template<bool In, bool Yield, bool Await>
struct VariableDeclarationList {
    std::vector<std::variant<VariableDeclaration<In, Yield, Await>>> declarations;

};

template<bool Yield, bool Await>
struct VariableStatement {
    VariableDeclarationList<true, Yield, Await> declarationList;
};

struct EmptyStatement {};

template<bool Yield, bool Await>
struct ExpressionStatement {
    Expression<false, Yield, Await> expression;
};

template<bool Yield, bool Await, bool Return>
struct IfStatement {
    Expression<true, Yield, Await> expression;
    StatementPtr<Yield, Await, Return> consequent;
    std::optional<StatementPtr<Yield, Await, Return>> alternate;
};

template<bool Yield, bool Await, bool Return>
struct DoWhileStatement {
    StatementPtr<Yield, Await, Return> statement;
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await, bool Return>
struct WhileStatement {
    Expression<true, Yield, Await> expression;
    StatementPtr<Yield, Await, Return> statement;
};

template<bool Yield, bool Await, bool Return>
struct ForStatement {
    std::variant<
        std::monostate,
        std::pair<LexicalDeclaration<false, Yield, Await>, Expression<true, Yield, Await>>,
        Expression<false, Yield, Await>
    > init;
    std::optional<Expression<true, Yield, Await>> condition;
    std::optional<Expression<true, Yield, Await>> update;
    StatementPtr<Yield, Await, Return> statement;
};

template<bool Yield, bool Await, bool Return>
struct ForInOfStatement {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Return>
struct IterationStatement {
    std::variant<
        DoWhileStatement<Yield, Await, Return>,
        WhileStatement<Yield, Await, Return>,
        ForStatement<Yield, Await, Return>,
        ForInOfStatement<Yield, Await, Return>
    > value;
};

template<bool Yield, bool Await, bool Return>
struct CaseClause {
    Expression<true, Yield, Await> expression;
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool Yield, bool Await, bool Return>
struct DefaultClause {
    std::optional<StatementList<Yield, Await, Return>> statementList;
};

template<bool Yield, bool Await, bool Return>
struct SwitchStatement {
    Expression<true, Yield, Await> expression;
    std::vector<CaseClause<Yield, Await, Return>> caseBlock;
    std::optional<DefaultClause<Yield, Await, Return>> defaultClause;
};

template<bool Yield, bool Await, bool Return>
struct BreakableStatement {
    std::variant<
        IterationStatement<Yield, Await, Return>,
        SwitchStatement<Yield, Await, Return>
    > value;
};

template<bool Yield, bool Await>
struct ContinueStatement {
    std::optional<LabelIdentifier<Yield, Await>> label;
};

template<bool Yield, bool Await>
struct BreakStatement {
    std::optional<LabelIdentifier<Yield, Await>> label;
};

template<bool Yield, bool Await>
struct ReturnStatement {
    std::optional<Expression<true, Yield, Await>> expression;
};

template<bool Yield, bool Await, bool Return>
struct WithStatement {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Default>
struct FunctionDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    FunctionBody<false, false> body;
};

template<bool Yield, bool Await, bool Return>
struct LabelledStatement {
    LabelIdentifier<Yield, Await> label;
    std::variant<
        StatementPtr<Yield, Await, Return>,
        FunctionDeclaration<Yield, Await, false>
    > statement;
};

template<bool Yield, bool Await>
struct ThrowStatement {
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct CatchParameter {
    std::variant<
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > value;
};

template<bool Yield, bool Await, bool Return>
struct Catch {
    CatchParameter<Yield, Await> parameter;
    Block<Yield, Await, Return> block;
};

template<bool Yield, bool Await, bool Return>
struct TryStatement {
    Block<Yield, Await, Return> block;
    std::optional<Catch<Yield, Await, Return>> catchClause;
    std::optional<Block<Yield, Await, Return>> finallyClause;
};

struct DebuggerStatement {};

template<bool Yield, bool Await, bool Return>
struct Statement {
    std::variant<
        BlockStatement<Yield, Await, Return>,
        VariableStatement<Yield, Await>,
        EmptyStatement,
        ExpressionStatement<Yield, Await>,
        IfStatement<Yield, Await, Return>,
        BreakableStatement<Yield, Await, Return>,
        ContinueStatement<Yield, Await>,
        BreakStatement<Yield, Await>,
        ReturnStatement<Yield, Await>, // [+Return]
        WithStatement<Yield, Await, Return>,
        LabelledStatement<Yield, Await, Return>,
        ThrowStatement<Yield, Await>,
        TryStatement<Yield, Await, Return>,
        DebuggerStatement
    > value;
};

template<bool Yield, bool Await, bool Default>
struct GeneratorDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    GeneratorBody body;
};

template<bool Yield, bool Await, bool Default>
struct AsyncFunctionDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    AsyncFunctionBody body;
};

template<bool Yield, bool Await, bool Default>
struct AsyncGeneratorDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    FormalParameters<false, false> parameters;
    AsyncGeneratorBody body;
};

template<bool Yield, bool Await, bool Default>
struct HoistableDeclaration {
    std::variant<
        FunctionDeclaration<Yield, Await, Default>,
        GeneratorDeclaration<Yield, Await, Default>,
        AsyncFunctionDeclaration<Yield, Await, Default>,
        AsyncGeneratorDeclaration<Yield, Await, Default>
    > value;
};

template<bool Yield, bool Await>
struct ClassHeritage {
    LeftHandSideExpressionPtr<Yield, Await> value;
};

struct ClassStaticBlock {
    std::optional<StatementList<false, false, false>> statementList;
};

template<bool Yield, bool Await>
struct MethodDefinition {
    struct GetFunctionBody : public FunctionBody<false, false> {};
    struct SetFunctionBody : public FunctionBody<false, false> {};

    std::variant<
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, FunctionBody<false, false>>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, GeneratorBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, AsyncFunctionBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, AsyncGeneratorBody>,
        std::tuple<ClassElementName<Yield, Await>, GetFunctionBody>,
        std::tuple<ClassElementName<Yield, Await>, UniqueFormalParameters<false, false>, SetFunctionBody>
    > value;
};

template<bool Yield, bool Await>
struct FieldDefinition {
    ClassElementName<Yield, Await> name;
    std::optional<InitializerPtr<true, Yield, Await>> initializer;
};

template<bool Yield, bool Await>
struct ClassElement {
    std::variant<
        std::monostate,  // semicolon
        std::pair<bool, MethodDefinition<Yield, Await>>,  // bool: static
        std::pair<bool, FieldDefinition<Yield, Await>>,  // bool: static
        ClassStaticBlock
    > value;
};

template<bool Yield, bool Await>
struct ClassBody {
    std::vector<ClassElement<Yield, Await>> elements;
};

template<bool Yield, bool Await, bool Default>
struct ClassDeclaration {
    std::optional<BindingIdentifier<Yield, Await>> name;  // optional only when [+Default]
    std::optional<ClassHeritage<Yield, Await>> heritage;
    ClassBody<Yield, Await> body;
};

template<bool Yield, bool Await, bool Return>
struct Declaration {
    std::variant<
        HoistableDeclaration<Yield, Await, false>,
        ClassDeclaration<Yield, Await, false>,
        LexicalDeclaration<true, Yield, Await>
    > value;
};

template<bool Yield, bool Await, bool Return>
struct StatementListItem {
    std::variant<
        Statement<Yield, Await, Return>,
        Declaration<Yield, Await, Return>
    > value;
};

template<bool Yield, bool Await>
struct CoverInitializedName {
    IdentifierReference<Yield, Await> identifier;
    InitializerPtr<true, Yield, Await> initializer;
};

template<bool Yield, bool Await>
struct PropertyDefinition {
    using SpreadAssignmentExpressionPtr = AssignmentExpressionPtr<true, Yield, Await>;

    std::variant<
        IdentifierReference<Yield, Await>,
        CoverInitializedName<Yield, Await>,
        std::pair<PropertyName<Yield, Await>, AssignmentExpressionPtr<true, Yield, Await>>,
        MethodDefinition<Yield, Await>,
        SpreadAssignmentExpressionPtr
    > value;
};

template<bool Yield, bool Await>
struct ObjectLiteral {
    std::vector<PropertyDefinition<Yield, Await>> properties;
};

// XXX: why are the following in the grammar twice?
struct FunctionExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    FunctionBody<false, false> body;
};

template<bool Yield, bool Await>
struct ClassExpression {
    std::optional<BindingIdentifier<Yield, Await>> name;
    std::optional<ClassHeritage<Yield, Await>> heritage;
    ClassBody<Yield, Await> body;
};

struct GeneratorExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    GeneratorBody body;
};

struct AsyncFunctionExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    AsyncFunctionBody body;
};

struct AsyncGeneratorExpression {
    std::optional<BindingIdentifier<false, false>> name;
    FormalParameters<false, false> parameters;
    AsyncGeneratorBody body;
};

struct RegularExpressionLiteral {
    // XXX: ignore for now
};

template<bool Yield, bool Await, bool Tagged>
struct TemplateLiteral {
    // XXX: ignore for now
};

template<bool Yield, bool Await>
struct CoverParenthesizedExpressionAndArrowParameterList {
    std::optional<Expression<true, Yield, Await>> expression;
    std::variant<
        std::monostate,  // no parameters
        BindingIdentifier<Yield, Await>,
        BindingPattern<Yield, Await>
    > parameters;
};

template<bool Yield, bool Await>
struct PrimaryExpression {
    std::variant<
        ThisExpr,
        IdentifierReference<Yield, Await>,
        Literal,
        ArrayLiteral<Yield, Await>,
        ObjectLiteral<Yield, Await>,
        FunctionExpression,
        ClassExpression<Yield, Await>,
        GeneratorExpression,
        AsyncFunctionExpression,
        AsyncGeneratorExpression,
        RegularExpressionLiteral,
        TemplateLiteral<Yield, Await, false>,
        CoverParenthesizedExpressionAndArrowParameterList<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct SuperProperty {
    std::variant<
        IdentifierName,  // dot
        Expression<true, Yield, Await>  // bracket
    > value;
};

struct MetaProperty {
    enum Kind {
        NewTarget,
        ImportMeta
    };

    Kind kind;
};

template<bool Yield, bool Await>
struct Arguments {
    std::vector<std::pair<bool, AssignmentExpressionPtr<true, Yield, Await>>> arguments;  // bool: spread
};

template<bool Yield, bool Await>
struct MemberExpression {
    std::variant<
        PrimaryExpression<Yield, Await>,
        std::pair<MemberExpression<Yield, Await>, std::variant<
            Expression<true, Yield, Await>,  // bracket
            IdentifierName, // dot
            PrivateIdentifier,  // dot
            TemplateLiteral<Yield, Await, true>,  // tag
            Arguments<Yield, Await>  // new
        >>,
        SuperProperty<Yield, Await>,
        MetaProperty
    > value;
};

template<bool Yield, bool Await>
struct NewExpression {
    using NewExpressionPtr = std::unique_ptr<NewExpression<Yield, Await>>;

    std::variant<
        MemberExpression<Yield, Await>,
        NewExpressionPtr
    > value;
};

template<bool Yield, bool Await>
struct SuperCall {
    Arguments<Yield, Await> arguments;
};

template<bool Yield, bool Await>
struct ImportCall {
    Expression<true, Yield, Await> expression;
};

template<bool Yield, bool Await>
struct CoverCallExpressionAndAsyncArrowHead {
    std::variant<
        MemberExpression<Yield, Await>,
        Arguments<Yield, Await>
    > value;
};

template<bool Yield, bool Await>
struct CallExpression {
    using CallExpressionPtr = std::unique_ptr<CallExpression<Yield, Await>>;

    std::variant<
        CoverCallExpressionAndAsyncArrowHead<Yield, Await>,
        SuperCall<Yield, Await>,
        ImportCall<Yield, Await>,
        std::pair<CallExpressionPtr, Arguments<Yield, Await>>,
        std::pair<CallExpressionPtr, Expression<true, Yield, Await>>,  // bracket
        std::pair<CallExpressionPtr, IdentifierName>,  // dot
        std::pair<CallExpressionPtr, PrivateIdentifier>,  // dot
        std::pair<CallExpressionPtr, TemplateLiteral<Yield, Await, true>>  // tag
    > value;
};

template<bool Yield, bool Await>
struct OptionalChain {
    using OptionalChainPtr = std::unique_ptr<OptionalChain<Yield, Await>>;

    std::optional<OptionalChainPtr> chain;
    std::variant<
        Arguments<Yield, Await>,  // ?.
        Expression<true, Yield, Await>,  // ?.[
        IdentifierName,  // ?.identifier
        PrivateIdentifier,  // ?.#identifier
        TemplateLiteral<Yield, Await, true>  // ?.tag
    > value;
};

template<bool Yield, bool Await>
struct OptionalExpression {
    using OptionalExpressionPtr = std::unique_ptr<OptionalExpression<Yield, Await>>;

    std::variant<
        MemberExpression<Yield, Await>,
        CallExpression<Yield, Await>,
        OptionalExpressionPtr
    > value;
    OptionalChain<Yield, Await> chain;
};

template<bool Yield, bool Await>
struct LeftHandSideExpression {
    std::variant<
        NewExpression<Yield, Await>,
        CallExpression<Yield, Await>,
        OptionalExpression<Yield, Await>
    > value;
};

template<bool In, bool Yield, bool Await>
struct YieldExpression {
    bool star;
    std::optional<AssignmentExpressionPtr<In, Yield, Await>> expression;
};

template<bool Yield, bool Await>
struct ArrowParameters {
    std::variant<
        BindingIdentifier<Yield, Await>,
        CoverParenthesizedExpressionAndArrowParameterList<Yield, Await>
    > value;
};

template<bool In>
struct ConciseBody {
    std::variant<
        AssignmentExpressionPtr<In, false, false>,  // lookahead not {
        FunctionBody<false, false>
    > value;
};

template<bool In, bool Yield, bool Await>
struct ArrowFunction {
    ArrowParameters<Yield, Await> parameters;
    ConciseBody<In> body;
};

template<bool In>
struct AsyncConciseBody {
    std::variant<
        AssignmentExpressionPtr<In, false, true>,  // lookahead not {
        AsyncFunctionBody
    > value;
};

template<bool In, bool Yield, bool Await>
struct AsyncArrowFunction {
    std::variant<
        BindingIdentifier<Yield, true>,
        CoverCallExpressionAndAsyncArrowHead<Yield, Await>
    > parameters;
    AsyncConciseBody<In> body;
};

template<bool In, bool Yield, bool Await>
struct AssignmentExpression {
    std::variant<
        ConditionalExpression<In, Yield, Await>,
        YieldExpression<In, Yield, Await>,  // only when [+Yield]
        ArrowFunction<In, Yield, Await>,
        AsyncArrowFunction<In, Yield, Await>,
        Assignment<In, Yield, Await>
    > value;
};


}  // namespace jac
