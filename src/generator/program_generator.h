#pragma once

#include "ast/ast.h"
#include "config/config.h"
#include "generator/generation_context.h"
#include "semantic/type_system.h"
#include "util/random.h"

class ProgramGenerator
{
public:
    ProgramGenerator(const GenerationConfig &cfg, Random &rng);

    Program generate();
    const GenerationStats &stats() const { return stats_; }

private:
    Function makeMainFunction();
    Function makeHelperFunction(int index);
    BlockStmt makeFunctionBody(GenerationContext &ctx, int depth, std::vector<std::string> &vars);
    StmtPtr makeVarDeclStmt(GenerationContext &ctx, std::vector<std::string> &vars, int depth);
    StmtPtr makeAssignStmt(GenerationContext &ctx, const std::vector<std::string> &vars, int depth);
    StmtPtr makeIfStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars);
    StmtPtr makeWhileStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars);
    StmtPtr makeReturnStmt(GenerationContext &ctx, const std::vector<std::string> &vars);
    ExprPtr makeExpr(GenerationContext &ctx, int depth, const std::vector<std::string> &vars);
    ExprPtr makeSafeBinary(GenerationContext &ctx, const std::string &op, ExprPtr a, ExprPtr b, const Type &type);
    std::string uniqueName(const std::string &prefix);
    ExprPtr makeSafeIntLiteral(int value) const;
    ExprPtr makeSafeAdd(ExprPtr a, ExprPtr b) const;
    ExprPtr makeBoundedRandInt(int min, int max);

    GenerationConfig config;
    Random &rng;
    SymbolTable symbols;
    GenerationStats stats_;
    TypeRegistry types;
    int nameCounter = 0;
};
