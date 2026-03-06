#pragma once

#include <ostream>
#include <string>

#include "ast.h"

class AstPrinter
{
public:
    void printProgram(const Program &program, std::ostream &os) const;
    void printAstJson(const Program &program, std::ostream &os) const;

private:
    void printFunction(const Function &fn, std::ostream &os, int indent) const;
    void printStatement(const Statement &stmt, std::ostream &os, int indent) const;
    void printExpression(const Expression &expr, std::ostream &os) const;
    std::string typeToString(const Type &type) const;
    std::string literalToString(const LiteralExpr &lit) const;
    void indent(std::ostream &os, int indent) const;
};
