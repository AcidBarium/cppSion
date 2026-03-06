#include "printer.h"

#include <sstream>

namespace
{
    std::string escapeString(const std::string &s)
    {
        std::ostringstream oss;
        for (char c : s)
        {
            switch (c)
            {
            case '\\':
                oss << "\\\\";
                break;
            case '"':
                oss << "\\\"";
                break;
            case '\n':
                oss << "\\n";
                break;
            default:
                oss << c;
                break;
            }
        }
        return oss.str();
    }
}

void AstPrinter::indent(std::ostream &os, int indent) const
{
    for (int i = 0; i < indent; ++i)
    {
        os << ' ';
    }
}

std::string AstPrinter::literalToString(const LiteralExpr &lit) const
{
    return lit.value;
}

std::string AstPrinter::typeToString(const Type &type) const
{
    switch (type.kind)
    {
    case Type::Kind::Builtin:
        switch (type.builtin)
        {
        case Type::Builtin::Int:
            return "int";
        case Type::Builtin::Double:
            return "double";
        case Type::Builtin::Bool:
            return "bool";
        case Type::Builtin::Void:
            return "void";
        }
        break;
    case Type::Kind::Pointer:
        return typeToString(*type.element) + " *";
    case Type::Kind::Array:
        return typeToString(*type.element) + "[" + std::to_string(type.arraySize) + "]";
    }
    return "int";
}

void AstPrinter::printExpression(const Expression &expr, std::ostream &os) const
{
    std::visit(
        [&](auto &&node)
        {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, LiteralExpr>)
            {
                os << literalToString(node);
            }
            else if constexpr (std::is_same_v<T, VariableRefExpr>)
            {
                os << node.name;
            }
            else if constexpr (std::is_same_v<T, BinaryExpr>)
            {
                os << '(';
                printExpression(*node.lhs, os);
                os << ' ' << node.op << ' ';
                printExpression(*node.rhs, os);
                os << ')';
            }
            else if constexpr (std::is_same_v<T, CallExpr>)
            {
                os << node.callee << '(';
                for (std::size_t i = 0; i < node.args.size(); ++i)
                {
                    if (i > 0)
                    {
                        os << ", ";
                    }
                    printExpression(*node.args[i], os);
                }
                os << ')';
            }
            else if constexpr (std::is_same_v<T, CastExpr>)
            {
                os << "static_cast<" << typeToString(node.targetType) << ">( ";
                printExpression(*node.expr, os);
                os << " )";
            }
        },
        expr.data);
}

void AstPrinter::printStatement(const Statement &stmt, std::ostream &os, int indentLevel) const
{
    indent(os, indentLevel);
    std::visit(
        [&](auto &&node)
        {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, VarDeclStmt>)
            {
                os << typeToString(node.type) << ' ' << node.name;
                if (node.init)
                {
                    os << " = ";
                    printExpression(**node.init, os);
                }
                os << ";\n";
            }
            else if constexpr (std::is_same_v<T, AssignStmt>)
            {
                os << node.name << " = ";
                printExpression(*node.expr, os);
                os << ";\n";
            }
            else if constexpr (std::is_same_v<T, ExprStmt>)
            {
                printExpression(*node.expr, os);
                os << ";\n";
            }
            else if constexpr (std::is_same_v<T, ReturnStmt>)
            {
                os << "return";
                if (node.expr)
                {
                    os << ' ';
                    printExpression(**node.expr, os);
                }
                os << ";\n";
            }
            else if constexpr (std::is_same_v<T, IfStmt>)
            {
                os << "if (";
                printExpression(*node.condition, os);
                os << ")\n";
                indent(os, indentLevel);
                os << "{\n";
                printStatement(*node.thenBranch, os, indentLevel + 4);
                indent(os, indentLevel);
                os << "}\n";
                if (node.elseBranch)
                {
                    indent(os, indentLevel);
                    os << "else\n";
                    indent(os, indentLevel);
                    os << "{\n";
                    printStatement(**node.elseBranch, os, indentLevel + 4);
                    indent(os, indentLevel);
                    os << "}\n";
                }
            }
            else if constexpr (std::is_same_v<T, WhileStmt>)
            {
                os << "while (";
                printExpression(*node.condition, os);
                os << ")\n";
                indent(os, indentLevel);
                os << "{\n";
                printStatement(*node.body, os, indentLevel + 4);
                indent(os, indentLevel);
                os << "}\n";
            }
            else if constexpr (std::is_same_v<T, BlockStmt>)
            {
                for (const auto &s : node.statements)
                {
                    printStatement(*s, os, indentLevel);
                }
            }
        },
        stmt.data);
}

void AstPrinter::printFunction(const Function &fn, std::ostream &os, int indentLevel) const
{
    indent(os, indentLevel);
    os << typeToString(fn.signature.returnType) << ' ' << fn.signature.name << '(';
    for (std::size_t i = 0; i < fn.signature.parameters.size(); ++i)
    {
        if (i > 0)
        {
            os << ", ";
        }
        os << typeToString(fn.signature.parameters[i]) << " p" << i;
    }
    os << ")\n";
    indent(os, indentLevel);
    os << "{\n";
    for (const auto &stmt : fn.body.statements)
    {
        printStatement(*stmt, os, indentLevel + 4);
    }
    indent(os, indentLevel);
    os << "}\n\n";
}

void AstPrinter::printProgram(const Program &program, std::ostream &os) const
{
    os << "#include <iostream>\n";
    os << "#include <cstdint>\n\n";

    for (const auto &fn : program.functions)
    {
        printFunction(fn, os, 0);
    }
}

void AstPrinter::printAstJson(const Program &program, std::ostream &os) const
{
    os << "{\n  \"functions\": [\n";
    for (std::size_t i = 0; i < program.functions.size(); ++i)
    {
        const auto &fn = program.functions[i];
        os << "    {\n";
        os << "      \"name\": \"" << escapeString(fn.signature.name) << "\",\n";
        os << "      \"return\": \"" << escapeString(typeToString(fn.signature.returnType)) << "\",\n";
        os << "      \"params\": [";
        for (std::size_t p = 0; p < fn.signature.parameters.size(); ++p)
        {
            if (p > 0)
                os << ", ";
            os << "\"" << escapeString(typeToString(fn.signature.parameters[p])) << "\"";
        }
        os << "],\n";
        os << "      \"body_lines\": " << fn.body.statements.size() << "\n";
        os << "    }";
        if (i + 1 < program.functions.size())
        {
            os << ',';
        }
        os << "\n";
    }
    os << "  ]\n}\n";
}
