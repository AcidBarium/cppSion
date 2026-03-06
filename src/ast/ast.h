#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "semantic/type_system.h"

struct Expression;
struct Statement;

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

enum class ExprKind
{
    Literal,
    VariableRef,
    Binary,
    Call,
    Cast
};

enum class StmtKind
{
    VarDecl,
    Assign,
    ExprStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    Block
};

enum class LiteralKind
{
    Integer,
    Double,
    Bool
};

struct LiteralExpr
{
    LiteralKind kind = LiteralKind::Integer;
    std::string value;
    Type type;
};

struct VariableRefExpr
{
    std::string name;
};

struct BinaryExpr
{
    std::string op;
    ExprPtr lhs;
    ExprPtr rhs;
    Type type;
};

struct CallExpr
{
    std::string callee;
    std::vector<ExprPtr> args;
};

struct CastExpr
{
    Type targetType;
    ExprPtr expr;
};

using ExprPayload = std::variant<LiteralExpr, VariableRefExpr, BinaryExpr, CallExpr, CastExpr>;

struct Expression
{
    ExprKind kind = ExprKind::Literal;
    ExprPayload data;
};

struct VarDeclStmt
{
    Type type;
    std::string name;
    std::optional<ExprPtr> init;
};

struct AssignStmt
{
    std::string name;
    ExprPtr expr;
};

struct ExprStmt
{
    ExprPtr expr;
};

struct ReturnStmt
{
    std::optional<ExprPtr> expr;
};

struct IfStmt
{
    ExprPtr condition;
    StmtPtr thenBranch;
    std::optional<StmtPtr> elseBranch;
};

struct WhileStmt
{
    ExprPtr condition;
    StmtPtr body;
};

struct BlockStmt
{
    std::vector<StmtPtr> statements;
};

using StmtPayload = std::variant<VarDeclStmt, AssignStmt, ExprStmt, ReturnStmt, IfStmt, WhileStmt, BlockStmt>;

struct Statement
{
    StmtKind kind = StmtKind::ExprStmt;
    StmtPayload data;
};

struct Function
{
    FunctionSignature signature;
    BlockStmt body;
};

struct Program
{
    std::vector<Function> functions;
};

inline ExprPtr makeLiteral(const LiteralExpr &lit)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Literal;
    ptr->data = lit;
    return ptr;
}

inline ExprPtr makeVariableRef(const std::string &name)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::VariableRef;
    ptr->data = VariableRefExpr{name};
    return ptr;
}

inline ExprPtr makeBinary(const std::string &op, ExprPtr lhs, ExprPtr rhs, const Type &type)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Binary;
    ptr->data = BinaryExpr{op, std::move(lhs), std::move(rhs), type};
    return ptr;
}

inline ExprPtr makeCall(const std::string &callee, std::vector<ExprPtr> args)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Call;
    ptr->data = CallExpr{callee, std::move(args)};
    return ptr;
}

inline ExprPtr makeCast(const Type &t, ExprPtr expr)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Cast;
    ptr->data = CastExpr{t, std::move(expr)};
    return ptr;
}

inline StmtPtr makeVarDecl(const Type &type, const std::string &name, std::optional<ExprPtr> init)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::VarDecl;
    ptr->data = VarDeclStmt{type, name, std::move(init)};
    return ptr;
}

inline StmtPtr makeAssign(const std::string &name, ExprPtr expr)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Assign;
    ptr->data = AssignStmt{name, std::move(expr)};
    return ptr;
}

inline StmtPtr makeExprStmt(ExprPtr expr)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::ExprStmt;
    ptr->data = ExprStmt{std::move(expr)};
    return ptr;
}

inline StmtPtr makeReturn(std::optional<ExprPtr> expr)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::ReturnStmt;
    ptr->data = ReturnStmt{std::move(expr)};
    return ptr;
}

inline StmtPtr makeIf(ExprPtr condition, StmtPtr thenBranch, std::optional<StmtPtr> elseBranch)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::IfStmt;
    ptr->data = IfStmt{std::move(condition), std::move(thenBranch), std::move(elseBranch)};
    return ptr;
}

inline StmtPtr makeWhile(ExprPtr condition, StmtPtr body)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::WhileStmt;
    ptr->data = WhileStmt{std::move(condition), std::move(body)};
    return ptr;
}

inline StmtPtr makeBlock(std::vector<StmtPtr> stmts)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Block;
    ptr->data = BlockStmt{std::move(stmts)};
    return ptr;
}
