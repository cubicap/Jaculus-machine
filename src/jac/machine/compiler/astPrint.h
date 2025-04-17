#pragma once

#include <iomanip>
#include <ostream>

#include "ast.h"


namespace jac::ast::print {


struct SetNesting {
    int newNesting;
    SetNesting(int newNesting_) : newNesting(newNesting_) {}
};

struct NestLine {};


struct NestOStream {
    std::ostream& os;
    int nesting = 0;

    struct Nest {
        NestOStream& nos;
        std::string_view name;

        Nest(NestOStream& nos_, std::string_view name_) : nos(nos_), name(name_) {
            nos << NestLine{} << "<" << name << ">";
            nos.nesting += 1;
        }
        ~Nest() {
            nos.nesting -= 1;
            nos << NestLine{} << "</" << name << ">";
        }
    };

    NestOStream(std::ostream& os_) : os(os_) {}
    [[nodiscard]] Nest nest(std::string_view name) {
        return Nest(*this, name);
    }

    void printIndent() {
        os << '\n' << std::string(nesting, ' ');
    }

    void terminalNode(std::string_view name) {
        os << '\n' << std::string(nesting, ' ');
        os << "<" << name << "/>";
    }

    NestOStream& operator<<(const auto& value) {
        printIndent();
        os << value;
        return *this;
    }

    NestOStream& operator<<(SetNesting setNesting) {
        nesting = setNesting.newNesting;
        return *this;
    }

    std::ostream& operator<<(NestLine) {
        printIndent();
        return os;
    }
};


template<typename... Ts>
inline NestOStream& operator<<(NestOStream& os, const std::variant<Ts...>& var) {
    std::visit([&os](const auto& value) { os << value; }, var);
    return os;
}

inline NestOStream& operator<<(NestOStream& os, std::monostate) {
    os << "<monostate>";
    return os;
}

template<typename... Ts>
inline NestOStream& operator<<(NestOStream& os, const std::tuple<Ts...>& tuple) {
    auto _ = os.nest("tuple");
    std::apply([&os](const auto&... values) { ((os << values), ...); }, tuple);
    return os;
}

template<typename A, typename B>
inline NestOStream& operator<<(NestOStream& os, const std::pair<A, B>& pair) {
    auto _ = os.nest("pair");
    os << pair.first;
    os << pair.second;
    return os;
}

template<typename T>
inline NestOStream& operator<<(NestOStream& os, const std::optional<T>& opt) {
    if (opt) {
        os << *opt;
    }
    else {
        os << "<null>";
    }
    return os;
}

template<typename T>
inline NestOStream& operator<<(NestOStream& os, const std::unique_ptr<T>& ptr) {
    if (ptr) {
        os << *ptr;
    }
    else {
        os << "<nullptr>";
    }
    return os;
}

template<typename T>
inline NestOStream& operator<<(NestOStream& os, const std::vector<T>& vec) {
    auto _ = os.nest("vector");
    for (const auto& value : vec) {
        os << value;
    }
    return os;
}


inline NestOStream& operator<<(NestOStream& os, const IdentifierName& node) {
    auto _ = os.nest("IdentifierName");
    os << node.name;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Identifier& node) {
    auto _ = os.nest("Identifier");
    os << node.name;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const IdentifierReference& node) {
    auto _ = os.nest("IdentifierReference");
    os << node.identifier;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BindingIdentifier& node) {
    auto _ = os.nest("BindingIdentifier");
    os << node.identifier;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const LabelIdentifier& node) {
    auto _ = os.nest("LabelIdentifier");
    os << node.identifier;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const PrivateIdentifier& node) {
    auto _ = os.nest("PrivateIdentifier");
    os << node.name;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ThisExpr&) {
    os.terminalNode("ThisExpression");
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const NullLiteral&) {
    os.terminalNode("NullLiteral");
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BooleanLiteral& node) {
    auto _ = os.nest("BooleanLiteral");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const NumericLiteral& node) {
    auto _ = os.nest("NumericLiteral");
    if (auto i = std::get_if<std::int32_t>(&node.value)) {
        os << NestLine{} << "i32(" << *i << ")";
    }
    else if (auto d = std::get_if<double>(&node.value)) {
        os << NestLine{} << "double(" << *d << ")";
    }

    return os;
}

inline NestOStream& operator<<(NestOStream& os, const StringLiteral& node) {
    auto _ = os.nest("StringLiteral");
    os << std::quoted(node.value);
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Literal& node) {
    auto _ = os.nest("Literal");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AssignmentExpression& node);

inline NestOStream& operator<<(NestOStream& os, const LeftHandSideExpression& node);

inline NestOStream& operator<<(NestOStream& os, const UnaryExpression& node) {
    auto _ = os.nest("UnaryExpression");
    os << node.value;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, UpdateKind kind) {
    switch (kind) {
        case UpdateKind::None:
            os << "None";
            break;
        case UpdateKind::PreInc:
            os << "PreInc";
            break;
        case UpdateKind::PreDec:
            os << "PreDec";
            break;
        case UpdateKind::PostInc:
            os << "PostInc";
            break;
        case UpdateKind::PostDec:
            os << "PostDec";
            break;
    }
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const UpdateExpression& node);

inline NestOStream& operator<<(NestOStream& os, const BinaryExpression& node);

inline NestOStream& operator<<(NestOStream& os, const ConditionalExpression& node) {
    auto _ = os.nest("ConditionalExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Elision&) {
    os.terminalNode("Elision");
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ArrayLiteral& node) {
    auto _ = os.nest("ArrayLiteral");
    os << node.elementList;
    os << node.spreadElement;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ComputedPropertyName& name) {
    auto _ = os.nest("ComputedPropertyName");
    os << name.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const PropertyName& node) {
    auto _ = os.nest("PropertyName");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassElementName& node) {
    auto _ = os.nest("ClassElementName");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BindingProperty& node) {
    auto _ = os.nest("BindingProperty");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BindingElement& node) {
    auto _ = os.nest("BindingElement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ObjectBindingPattern& node) {
    auto _ = os.nest("ObjectBindingPattern");
    os << node.properties;
    os << node.rest;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ArrayBindingPattern& node) {
    auto _ = os.nest("ArrayBindingPattern");
    os << node.elements;
    os << node.rest;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BindingPattern& node) {
    auto _ = os.nest("BindingPattern");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const TypeAnnotation& node) {
    auto _ = os.nest("TypeAnnotation");
    os << node.type;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BindingRestElement& node) {
    auto _ = os.nest("BindingRestElement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const FormalParameters& node) {
    auto _ = os.nest("FormalParameters");
    os << node.parameterList;
    os << node.restParameter;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const UniqueFormalParameters& node) {
    auto _ = os.nest("UniqueFormalParameters");
    os << node.parameters;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Statement& node);

inline NestOStream& operator<<(NestOStream& os, const StatementListItem& node);

inline NestOStream& operator<<(NestOStream& os, const StatementList& node);

inline NestOStream& operator<<(NestOStream& os, const Expression& node) {
    auto _ = os.nest("Expression");
    os << node.items;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Block& node) {
    auto _ = os.nest("Block");
    os << node.statementList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const LexicalBinding& node) {
    auto _ = os.nest("LexicalBinding");
    os << node.value;
    os << node.type;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const LexicalDeclaration& node) {
    auto _ = os.nest("LexicalDeclaration");
    os << node.isConst;
    os << node.bindings;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const VariableDeclaration& node) {
    auto _ = os.nest("VariableDeclaration");
    os << node.identifier;
    os << node.initializer;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const VariableDeclarationList& node) {
    auto _ = os.nest("VariableDeclarationList");
    os << node.declarations;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const VariableStatement& node) {
    auto _ = os.nest("VariableStatement");
    os << node.declarationList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const EmptyStatement&) {
    os.terminalNode("EmptyStatement");
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ExpressionStatement& node) {
    auto _ = os.nest("ExpressionStatement");
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const IfStatement& node) {
    auto _ = os.nest("IfStatement");
    os << node.expression;
    os << node.consequent;
    os << node.alternate;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const DoWhileStatement& node) {
    auto _ = os.nest("DoWhileStatement");
    os << node.statement;
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const WhileStatement& node) {
    auto _ = os.nest("WhileStatement");
    os << node.expression;
    os << node.statement;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ForStatement& node) {
    auto _ = os.nest("ForStatement");
    os << node.init;
    os << node.condition;
    os << node.update;
    os << node.statement;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ForInOfStatement& node) {
    auto _ = os.nest("ForInOfStatement");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const IterationStatement& node) {
    auto _ = os.nest("IterationStatement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CaseClause& node) {
    auto _ = os.nest("CaseClause");
    os << node.expression;
    os << node.statementList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const DefaultClause& node) {
    auto _ = os.nest("DefaultClause");
    os << node.statementList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const SwitchStatement& node) {
    auto _ = os.nest("SwitchStatement");
    os << node.expression;
    os << node.caseBlock;
    os << node.defaultClause;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BreakableStatement& node) {
    auto _ = os.nest("BreakableStatement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ContinueStatement& node) {
    auto _ = os.nest("ContinueStatement");
    os << node.label;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BreakStatement& node) {
    auto _ = os.nest("BreakStatement");
    os << node.label;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ReturnStatement& node) {
    auto _ = os.nest("ReturnStatement");
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const WithStatement& node) {
    auto _ = os.nest("WithStatement");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const FunctionBody& node) {
    auto _ = os.nest("FunctionBody");
    os << node.statementList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const FunctionDeclaration& node) {
    auto _ = os.nest("FunctionDeclaration");
    os << node.name;
    os << node.parameters;
    os << node.body;
    os << node.returnType;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const LabeledStatement& node) {
    auto _ = os.nest("LabeledStatement");
    os << node.label;
    os << node.statement;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ThrowStatement& node) {
    auto _ = os.nest("ThrowStatement");
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CatchParameter& node) {
    auto _ = os.nest("CatchParameter");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Catch& node) {
    auto _ = os.nest("Catch");
    os << node.parameter;
    os << node.block;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const TryStatement& node) {
    auto _ = os.nest("TryStatement");
    os << node.block;
    os << node.catchClause;
    os << node.finallyClause;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const DebuggerStatement&) {
    os.terminalNode("DebuggerStatement");
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const GeneratorDeclaration& node) {
    auto _ = os.nest("GeneratorDeclaration");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AsyncFunctionDeclaration& node) {
    auto _ = os.nest("AsyncFunctionDeclaration");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}
inline NestOStream& operator<<(NestOStream& os, const AsyncGeneratorDeclaration& node) {
    auto _ = os.nest("AsyncGeneratorDeclaration");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const HoistableDeclaration& node) {
    auto _ = os.nest("HoistableDeclaration");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassHeritage& node) {
    auto _ = os.nest("ClassHeritage");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassStaticBlock& node) {
    auto _ = os.nest("ClassStaticBlock");
    os << node.statementList;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const SetFunctionBody& node) {
    auto _ = os.nest("SetFunctionBody");
    const FunctionBody& body = node;
    os << body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const GetFunctionBody& node) {
    auto _ = os.nest("GetFunctionBody");
    const FunctionBody& body = node;
    os << body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const MethodDefinition& node) {
    auto _ = os.nest("MethodDefinition");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const FieldDefinition& node) {
    auto _ = os.nest("FieldDefinition");
    os << node.name;
    os << node.initializer;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassElement& node) {
    auto _ = os.nest("ClassElement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassBody& node) {
    auto _ = os.nest("ClassBody");
    os << node.elements;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassDeclaration& node) {
    auto _ = os.nest("ClassDeclaration");
    os << node.name;
    os << node.heritage;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Declaration& node) {
    auto _ = os.nest("Declaration");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CoverInitializedName& node) {
    auto _ = os.nest("CoverInitializedName");
    os << node.identifier;
    os << node.initializer;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const PropertyDefinition& node) {
    auto _ = os.nest("PropertyDefinition");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ObjectLiteral& node) {
    auto _ = os.nest("ObjectLiteral");
    os << node.properties;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const FunctionExpression& node) {
    auto _ = os.nest("FunctionExpression");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ClassExpression& node) {
    auto _ = os.nest("ClassExpression");
    os << node.name;
    os << node.heritage;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const GeneratorExpression& node) {
    auto _ = os.nest("GeneratorExpression");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AsyncFunctionExpression& node) {
    auto _ = os.nest("AsyncFunctionExpression");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AsyncGeneratorExpression& node) {
    auto _ = os.nest("AsyncGeneratorExpression");
    os << node.name;
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const RegularExpressionLiteral& node) {
    os.terminalNode("RegularExpressionLiteral");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const TemplateLiteral& node) {
    os.terminalNode("TemplateLiteral");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ParenthesizedExpression& node) {
    auto _ = os.nest("ParenthesizedExpression");
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CoverParenthesizedExpressionAndArrowParameterList& node) {
    auto _ = os.nest("CoverParenthesizedExpressionAndArrowParameterList");
    os << node.expression;
    os << node.parameters;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const PrimaryExpression& node) {
    auto _ = os.nest("PrimaryExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const SuperProperty& node) {
    auto _ = os.nest("SuperProperty");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const MetaProperty& node) {
    auto _ = os.nest("MetaProperty");
    switch (node.kind) {
        case MetaProperty::Kind::NewTarget:
            os << "NewTarget";
            break;
        case MetaProperty::Kind::ImportMeta:
            os << "ImportMeta";
            break;
    }
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Arguments& node) {
    auto _ = os.nest("Arguments");
    os << node.arguments;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const MemberExpression& node) {
    auto _ = os.nest("MemberExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const NewExpression& node) {
    auto _ = os.nest("NewExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const SuperCall& node) {
    auto _ = os.nest("SuperCall");
    os << node.arguments;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ImportCall& node) {
    auto _ = os.nest("ImportCall");
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CoverCallExpressionAndAsyncArrowHead& node) {
    auto _ = os.nest("CoverCallExpressionAndAsyncArrowHead");
    os << node.memberExpression;
    os << node.arguments;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const CallExpression& node) {
    auto _ = os.nest("CallExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const OptionalChain& node) {
    auto _ = os.nest("OptionalChain");
    os << node.chain;
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const OptionalExpression& node) {
    auto _ = os.nest("OptionalExpression");
    os << node.value;
    os << node.chain;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const YieldExpression& node) {
    auto _ = os.nest("YieldExpression");
    os << node.star;
    os << node.expression;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ArrowParameters& node) {
    auto _ = os.nest("ArrowParameters");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ConciseBody& node) {
    auto _ = os.nest("ConciseBody");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ArrowFunction& node) {
    auto _ = os.nest("ArrowFunction");
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AsyncConciseBody& node) {
    auto _ = os.nest("AsyncConciseBody");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AsyncArrowFunction& node) {
    auto _ = os.nest("AsyncArrowFunction");
    os << node.parameters;
    os << node.body;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Assignment& node) {
    auto _ = os.nest("Assignment");
    os << node.lhs;
    os << node.rhs;
    os << node.op;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Script& node) {
    auto _ = os.nest("Script");
    if (node.statementList) {
        os << *node.statementList;
    }
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ImportDeclaration& node) {
    os.terminalNode("ImportDeclaration");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ExportDeclaration& node) {
    os.terminalNode("ExportDeclaration");
    // incomplete
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ModuleItem& node) {
    auto _ = os.nest("ModuleItem");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const ModuleItemList& node) {
    auto _ = os.nest("ModuleItemList");
    os << node.items;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Module& node) {
    auto _ = os.nest("Module");
    if (node.moduleItemList) {
        os << *node.moduleItemList;
    }
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const Statement& node) {
    auto _ = os.nest("Statement");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const StatementListItem& node) {
    auto _ = os.nest("StatementListItem");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const AssignmentExpression& node) {
    auto _ = os.nest("AssignmentExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const LeftHandSideExpression& node) {
    auto _ = os.nest("LeftHandSideExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const UpdateExpression& node) {
    auto _ = os.nest("UpdateExpression");
    os << NestLine{} << node.kind;
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const BinaryExpression& node) {
    auto _ = os.nest("BinaryExpression");
    os << node.value;
    return os;
}

inline NestOStream& operator<<(NestOStream& os, const StatementList& node) {
    auto _ = os.nest("StatementList");
    os << node.items;
    return os;
}



}  // namespace jac::ast::print
