#include "program_generator.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

ProgramGenerator::ProgramGenerator(const GenerationConfig &cfg, Random &rng)
    : config(cfg), rng(rng)
{
}

std::string ProgramGenerator::uniqueName(const std::string &prefix)
{
    return prefix + std::to_string(++nameCounter);
}

ExprPtr ProgramGenerator::makeSafeIntLiteral(int value) const
{
    LiteralExpr lit;
    lit.kind = LiteralKind::Integer;
    lit.value = std::to_string(value);
    lit.type = types.intType();
    return makeLiteral(lit);
}

ExprPtr ProgramGenerator::makeSafeAdd(ExprPtr a, ExprPtr b) const
{
    // Wrap addition through unsigned to avoid signed overflow UB.
    auto ua = makeCast(Type::makeBuiltin(Type::Builtin::Int), std::move(a));
    auto ub = makeCast(Type::makeBuiltin(Type::Builtin::Int), std::move(b));
    return makeBinary("+", std::move(ua), std::move(ub), types.intType());
}

ExprPtr ProgramGenerator::makeSafeBinary(GenerationContext &ctx, const std::string &op, ExprPtr a, ExprPtr b, const Type &type)
{
    if (!ctx.consumeExpr())
    {
        return makeSafeIntLiteral(0);
    }
    return makeBinary(op, std::move(a), std::move(b), type);
}

ExprPtr ProgramGenerator::makeBoundedRandInt(int min, int max)
{
    int value = rng.nextInt(min, max);
    return makeSafeIntLiteral(value);
}

ExprPtr ProgramGenerator::makeExpr(GenerationContext &ctx, int depth, const std::vector<std::string> &vars)
{
    ctx.noteDepth(depth);
    if (!ctx.consumeExpr())
    {
        return makeSafeIntLiteral(0);
    }

    bool allowBinary = depth < config.budget.maxDepth && ctx.hasExprBudget();
    enum Choice
    {
        Literal,
        VarRef,
        Binary
    };

    double computeW = config.weights.compute;
    std::vector<double> weights;
    weights.push_back(0.2);                                   // Literal baseline
    weights.push_back(vars.empty() ? 0.0 : 0.2);              // VarRef baseline
    double binW = allowBinary ? (0.1 + 0.8 * computeW) : 0.0; // Binary favored by compute weight
    weights.push_back(binW);

    std::size_t pick = rng.weightedIndex(weights);
    if (pick == Literal)
    {
        return makeSafeIntLiteral(rng.nextInt(0, 1000));
    }
    if (pick == VarRef && !vars.empty())
    {
        return makeVariableRef(vars[rng.nextInt(0, static_cast<int>(vars.size() - 1))]);
    }
    // Binary
    auto lhs = makeExpr(ctx, depth + 1, vars);
    auto rhs = makeExpr(ctx, depth + 1, vars);
    return makeSafeBinary(ctx, "+", std::move(lhs), std::move(rhs), types.intType());
}

StmtPtr ProgramGenerator::makeVarDeclStmt(GenerationContext &ctx, std::vector<std::string> &vars, int depth)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    std::string name = uniqueName("v");
    symbols.addVariable(VariableSymbol{name, types.intType()});
    vars.push_back(name);
    auto init = makeExpr(ctx, depth + 1, vars);
    return makeVarDecl(types.intType(), name, init);
}

StmtPtr ProgramGenerator::makeAssignStmt(GenerationContext &ctx, const std::vector<std::string> &vars, int depth)
{
    if (vars.empty() || !ctx.consumeStatement())
    {
        return nullptr;
    }
    std::string name = vars[rng.nextInt(0, static_cast<int>(vars.size() - 1))];
    auto expr = makeExpr(ctx, depth + 1, vars);
    return makeAssign(name, expr);
}

StmtPtr ProgramGenerator::makeIfStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ctx.noteBranch();

    auto cond = makeExpr(ctx, depth + 1, vars);

    symbols.pushScope();
    auto thenVars = vars;
    auto thenStmt = makeVarDeclStmt(ctx, thenVars, depth + 1);
    symbols.popScope();

    symbols.pushScope();
    auto elseVars = vars;
    StmtPtr elseStmt = nullptr;
    if (rng.nextBool(0.5))
    {
        elseStmt = makeVarDeclStmt(ctx, elseVars, depth + 1);
    }
    symbols.popScope();

    if (!thenStmt)
    {
        thenStmt = makeExprStmt(makeSafeIntLiteral(0));
    }

    std::optional<StmtPtr> elseOpt;
    if (elseStmt)
    {
        elseOpt = elseStmt;
    }

    return makeIf(cond, thenStmt, elseOpt);
}

StmtPtr ProgramGenerator::makeWhileStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ctx.noteLoop();

    std::string counter = uniqueName("i");
    symbols.addVariable(VariableSymbol{counter, types.intType()});
    auto loopVars = vars;
    loopVars.push_back(counter);

    auto init = makeSafeIntLiteral(0);
    auto counterDecl = makeVarDecl(types.intType(), counter, init);

    int limit = rng.nextInt(1, 5);
    auto cond = makeSafeBinary(ctx, "<", makeVariableRef(counter), makeSafeIntLiteral(limit), types.boolType());

    std::vector<StmtPtr> bodyStmts;
    auto incExpr = makeSafeBinary(ctx, "+", makeVariableRef(counter), makeSafeIntLiteral(1), types.intType());
    bodyStmts.push_back(makeAssign(counter, incExpr));

    if (!loopVars.empty())
    {
        auto target = loopVars[rng.nextInt(0, static_cast<int>(loopVars.size() - 1))];
        auto addExpr = makeSafeBinary(ctx, "+", makeVariableRef(target), makeSafeIntLiteral(rng.nextInt(1, 3)), types.intType());
        bodyStmts.push_back(makeAssign(target, addExpr));
    }

    auto body = makeBlock(bodyStmts);

    auto whileStmt = makeWhile(cond, body);

    return makeBlock({counterDecl, whileStmt});
}

StmtPtr ProgramGenerator::makeReturnStmt(GenerationContext &ctx, const std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ExprPtr expr;
    if (!vars.empty())
    {
        expr = makeVariableRef(vars[rng.nextInt(0, static_cast<int>(vars.size() - 1))]);
    }
    else
    {
        expr = makeSafeIntLiteral(0);
    }
    return makeReturn(expr);
}

BlockStmt ProgramGenerator::makeFunctionBody(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    std::vector<StmtPtr> stmts;
    int target = std::max(4, config.budget.maxStatements / std::max(1, config.budget.maxFunctions));

    while ((int)stmts.size() < target)
    {
        double branchW = config.weights.branch;
        double computeW = config.weights.compute;
        double memoryW = config.weights.memory;

        double wDecl = 0.15 + 0.30 * memoryW;
        double wAssign = 0.15 + 0.20 * computeW;
        double wIf = 0.10 + 0.80 * branchW;
        double wWhile = 0.05 + 0.40 * branchW + 0.10 * computeW;
        double wExpr = 0.10 + 0.60 * computeW;

        std::vector<double> weights = {wDecl, wAssign, wIf, wWhile, wExpr};
        std::size_t choice = rng.weightedIndex(weights);
        StmtPtr stmt;
        switch (choice)
        {
        case 0:
            stmt = makeVarDeclStmt(ctx, vars, depth + 1);
            break;
        case 1:
            stmt = makeAssignStmt(ctx, vars, depth + 1);
            break;
        case 2:
            if (depth < config.budget.maxDepth)
            {
                stmt = makeIfStmt(ctx, depth + 1, vars);
            }
            break;
        case 3:
            if (depth < config.budget.maxDepth)
            {
                stmt = makeWhileStmt(ctx, depth + 1, vars);
            }
            break;
        case 4:
        default:
            if (ctx.consumeStatement())
            {
                auto e = makeExpr(ctx, depth + 1, vars);
                stmt = makeExprStmt(e);
            }
            break;
        }

        if (stmt)
        {
            stmts.push_back(stmt);
        }
        else
        {
            break;
        }
    }

    if (stmts.empty())
    {
        stmts.push_back(makeVarDecl(types.intType(), uniqueName("v"), makeSafeIntLiteral(0)));
    }

    // Ensure return at end
    if (auto ret = makeReturnStmt(ctx, vars))
    {
        stmts.push_back(ret);
    }

    return BlockStmt{std::move(stmts)};
}

Function ProgramGenerator::makeMainFunction()
{
    Function fn;
    fn.signature.name = "main";
    fn.signature.returnType = types.intType();

    symbols.addFunction(FunctionSymbol{FunctionSignature{fn.signature.name, fn.signature.returnType, {}}});

    GenerationContext ctx(config, symbols, stats_);
    ctx.noteFunction();

    symbols.pushScope();
    std::vector<std::string> vars;
    fn.body = makeFunctionBody(ctx, 0, vars);
    symbols.popScope();

    return fn;
}

Function ProgramGenerator::makeHelperFunction(int index)
{
    Function fn;
    fn.signature.name = "func" + std::to_string(index);
    fn.signature.returnType = types.intType();
    fn.signature.parameters = {types.intType()};

    symbols.addFunction(FunctionSymbol{fn.signature});

    GenerationContext ctx(config, symbols, stats_);
    ctx.noteFunction();

    symbols.pushScope();
    std::vector<std::string> vars;
    symbols.addVariable(VariableSymbol{"p0", types.intType()});
    vars.push_back("p0");

    fn.body = makeFunctionBody(ctx, 0, vars);

    symbols.popScope();
    return fn;
}

Program ProgramGenerator::generate()
{
    Program program;
    if (!config.budget.maxFunctions)
    {
        return program;
    }

    int fnCount = std::max(1, std::min(config.budget.maxFunctions, 1 + config.budget.maxFunctions / 4));
    for (int i = 0; i < fnCount - 1; ++i)
    {
        program.functions.push_back(makeHelperFunction(i));
    }

    program.functions.push_back(makeMainFunction());
    return program;
}
