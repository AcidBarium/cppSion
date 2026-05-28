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
    Cast,
    Unary,
    Ternary,
    NewExpr,
    DeleteExpr,
    ArraySub
};

enum class StmtKind
{
    VarDecl,
    Assign,
    ExprStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    Block,
    Switch,
    Break,
    Continue,
    CompoundAssign
};

enum class LiteralKind
{
    Integer,
    Double,
    Bool,
    String
};

enum class UnaryOp
{
    Plus,
    Minus,
    Not,
    BitNot,
    PreInc,
    PostInc,
    PreDec,
    PostDec,
    Deref,
    Addr
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

struct UnaryExpr
{
    UnaryOp op;
    ExprPtr expr;
    Type type;
};

struct TernaryExpr
{
    ExprPtr condition;
    ExprPtr thenExpr;
    ExprPtr elseExpr;
    Type type;
};

struct NewExprData
{
    Type allocatedType;
    ExprPtr arraySize; // null => scalar new
};

struct DeleteExprData
{
    ExprPtr expr;
    bool isArray = false;
};

struct ArraySubExpr
{
    ExprPtr array;
    ExprPtr index;
    Type type;
};

using ExprPayload = std::variant<LiteralExpr, VariableRefExpr, BinaryExpr, CallExpr, CastExpr,
                                 UnaryExpr, TernaryExpr, NewExprData, DeleteExprData, ArraySubExpr>;

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
    ExprPtr lhs;
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

struct SwitchCase
{
    std::optional<ExprPtr> value; // nullopt => default
    std::vector<StmtPtr> body;
};

struct SwitchStmt
{
    ExprPtr condition;
    std::vector<SwitchCase> cases;
};

struct BreakStmt
{
};

struct ContinueStmt
{
};

struct CompoundAssignStmt
{
    std::string op; // +=, -=, *=, /=, %=, <<=, >>=, &=, |=, ^=
    std::string name;
    ExprPtr expr;
};

struct BlockStmt
{
    std::vector<StmtPtr> statements;
};

using StmtPayload = std::variant<VarDeclStmt, AssignStmt, ExprStmt, ReturnStmt, IfStmt, WhileStmt,
                                 SwitchStmt, BreakStmt, ContinueStmt, CompoundAssignStmt, BlockStmt>;

struct Statement
{
    StmtKind kind = StmtKind::ExprStmt;
    StmtPayload data;
};

struct Function
{
    FunctionSignature signature;
    BlockStmt body;
    bool isTemplate = false;
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

inline StmtPtr makeAssign(ExprPtr lhs, ExprPtr expr)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Assign;
    ptr->data = AssignStmt{std::move(lhs), std::move(expr)};
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

inline ExprPtr makeUnary(UnaryOp op, ExprPtr expr, const Type &type)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Unary;
    ptr->data = UnaryExpr{op, std::move(expr), type};
    return ptr;
}

inline ExprPtr makeTernary(ExprPtr cond, ExprPtr thenExpr, ExprPtr elseExpr, const Type &type)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::Ternary;
    ptr->data = TernaryExpr{std::move(cond), std::move(thenExpr), std::move(elseExpr), type};
    return ptr;
}

inline ExprPtr makeNewExpr(const Type &allocatedType, ExprPtr arraySize = nullptr)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::NewExpr;
    ptr->data = NewExprData{allocatedType, std::move(arraySize)};
    return ptr;
}

inline ExprPtr makeDeleteExpr(ExprPtr expr, bool isArray = false)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::DeleteExpr;
    ptr->data = DeleteExprData{std::move(expr), isArray};
    return ptr;
}

inline ExprPtr makeArraySub(ExprPtr array, ExprPtr index, const Type &type)
{
    auto ptr = std::make_shared<Expression>();
    ptr->kind = ExprKind::ArraySub;
    ptr->data = ArraySubExpr{std::move(array), std::move(index), type};
    return ptr;
}

inline StmtPtr makeSwitch(ExprPtr condition, std::vector<SwitchCase> cases)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Switch;
    ptr->data = SwitchStmt{std::move(condition), std::move(cases)};
    return ptr;
}

inline StmtPtr makeBreak()
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Break;
    ptr->data = BreakStmt{};
    return ptr;
}

inline StmtPtr makeContinue()
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Continue;
    ptr->data = ContinueStmt{};
    return ptr;
}

inline StmtPtr makeCompoundAssign(const std::string &op, const std::string &name, ExprPtr expr)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::CompoundAssign;
    ptr->data = CompoundAssignStmt{op, name, std::move(expr)};
    return ptr;
}

inline StmtPtr makeBlock(std::vector<StmtPtr> stmts)
{
    auto ptr = std::make_shared<Statement>();
    ptr->kind = StmtKind::Block;
    ptr->data = BlockStmt{std::move(stmts)};
    return ptr;
}
