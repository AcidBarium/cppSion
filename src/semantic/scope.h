#pragma once

#include <string>
#include <unordered_map>

#include "type_system.h"

struct VariableSymbol
{
    std::string name;
    Type type;
};

class Scope
{
public:
    void addVariable(const VariableSymbol &var)
    {
        variables[var.name] = var;
    }

    const VariableSymbol *lookup(const std::string &name) const
    {
        auto it = variables.find(name);
        if (it != variables.end())
        {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, VariableSymbol> variables;
};
