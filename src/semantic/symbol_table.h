#pragma once

#include <string>
#include <vector>

#include "scope.h"
#include "type_system.h"

struct FunctionSymbol
{
    FunctionSignature signature;
};

class SymbolTable
{
public:
    void pushScope()
    {
        scopes.emplace_back();
    }

    void popScope()
    {
        if (!scopes.empty())
        {
            scopes.pop_back();
        }
    }

    void addVariable(const VariableSymbol &var)
    {
        if (scopes.empty())
        {
            pushScope();
        }
        scopes.back().addVariable(var);
    }

    const VariableSymbol *lookupVariable(const std::string &name) const
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            if (auto *v = it->lookup(name))
            {
                return v;
            }
        }
        return nullptr;
    }

    void addFunction(const FunctionSymbol &fn)
    {
        functions.push_back(fn);
    }

    const FunctionSymbol *lookupFunction(const std::string &name) const
    {
        for (const auto &fn : functions)
        {
            if (fn.signature.name == name)
            {
                return &fn;
            }
        }
        return nullptr;
    }

private:
    std::vector<Scope> scopes;
    std::vector<FunctionSymbol> functions;
};
