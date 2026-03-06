#pragma once

#include <cstdint>

#include "config/config.h"
#include "semantic/symbol_table.h"

struct GenerationStats
{
    int functions = 0;
    int statements = 0;
    int loops = 0;
    int branches = 0;
    int expressions = 0;
    int maxDepth = 0;
    int memoryOps = 0;
};

// Tracks budgets and shared state for generation.
class GenerationContext
{
public:
    GenerationContext(const GenerationConfig &cfg, SymbolTable &symbols, GenerationStats &stats)
        : config(cfg), table(symbols), stats(stats)
    {
    }

    bool consumeStatement()
    {
        if (remainingStatements <= 0)
        {
            return false;
        }
        --remainingStatements;
        ++stats.statements;
        return true;
    }

    bool consumeExpr()
    {
        if (remainingExpr <= 0)
        {
            return false;
        }
        --remainingExpr;
        ++stats.expressions;
        return true;
    }

    bool hasExprBudget() const
    {
        return remainingExpr > 0;
    }

    bool canAddFunction() const
    {
        return stats.functions < config.budget.maxFunctions;
    }

    void noteFunction()
    {
        ++stats.functions;
    }

    void noteBranch()
    {
        ++stats.branches;
    }

    void noteLoop()
    {
        ++stats.loops;
    }

    void noteDepth(int depth)
    {
        if (depth > stats.maxDepth)
        {
            stats.maxDepth = depth;
        }
    }

    const GenerationConfig &config;
    SymbolTable &table;
    GenerationStats &stats;

private:
    int remainingStatements = config.budget.maxStatements;
    int remainingExpr = config.budget.maxExprNodes;
};
