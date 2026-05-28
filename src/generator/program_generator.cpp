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

ExprPtr ProgramGenerator::makeSafeBinary(GenerationContext &ctx, const std::string &op, ExprPtr a, ExprPtr b, const Type &type)
{
    if (!ctx.consumeExpr())
    {
        return makeSafeIntLiteral(0);
    }
    return makeBinary(op, std::move(a), std::move(b), type);
}

std::string pickArithOp(Random &rng)
{
    static const std::vector<std::string> ops = {"+", "-", "*", "/", "%", "&", "|", "^"};
    return ops[rng.nextInt(0, static_cast<int>(ops.size() - 1))];
}

std::string pickCmpOp(Random &rng)
{
    static const std::vector<std::string> ops = {"&&", "||", "<", ">", "<=", ">=", "==", "!="};
    return ops[rng.nextInt(0, static_cast<int>(ops.size() - 1))];
}

UnaryOp pickUnaryOp(Random &rng)
{
    // Not (!) excluded: it produces bool in C++, causing template T=bool deduction
    static const std::vector<UnaryOp> ops = {UnaryOp::Minus, UnaryOp::BitNot};
    return ops[rng.nextInt(0, static_cast<int>(ops.size() - 1))];
}

UnaryOp pickDerefOp(Random &rng)
{
    return rng.nextBool(0.5) ? UnaryOp::Deref : UnaryOp::Addr;
}

ExprPtr ProgramGenerator::makeExpr(GenerationContext &ctx, int depth, const std::vector<std::string> &vars, const Type &hint)
{
    ctx.noteDepth(depth);
    if (!ctx.consumeExpr())
    {
        return makeSafeIntLiteral(0);
    }

    bool allowNested = depth < config.budget.maxDepth && ctx.hasExprBudget();
    double computeW = config.weights.compute;

    double recursionW = config.weights.recursion;
    bool hasFuncs = symbols.lookupFunction("main") != nullptr;

    enum Choice { Literal, VarRef, Binary, UnaryChoice, CallChoice };
    std::vector<double> weights;
    weights.push_back(0.15);
    weights.push_back(vars.empty() ? 0.0 : 0.15);
    weights.push_back(allowNested ? (0.10 + 0.60 * computeW) : 0.0);
    weights.push_back(allowNested ? (0.05 + 0.20 * computeW) : 0.0);
    weights.push_back(allowNested && hasFuncs ? (0.05 + 0.50 * recursionW) : 0.0);

    std::size_t pick = rng.weightedIndex(weights);

    if (pick == Literal)
    {
        if (hint.kind == Type::Kind::Builtin)
        {
            if (hint.builtin == Type::Builtin::Double)
            {
                LiteralExpr lit;
                lit.kind = LiteralKind::Double;
                lit.value = std::to_string(rng.nextInt(0, 1000)) + "." + std::to_string(rng.nextInt(0, 999));
                lit.type = types.doubleType();
                return makeLiteral(lit);
            }
            if (hint.builtin == Type::Builtin::Bool)
            {
                LiteralExpr lit;
                lit.kind = LiteralKind::Bool;
                lit.value = rng.nextBool(0.5) ? "true" : "false";
                lit.type = types.boolType();
                return makeLiteral(lit);
            }
        }
        // Default to integer
        LiteralExpr lit;
        lit.kind = LiteralKind::Integer;
        lit.value = std::to_string(rng.nextInt(0, 1000));
        lit.type = types.intType();
        return makeLiteral(lit);
    }

    if (pick == VarRef && !vars.empty())
    {
        // Only reference int-typed vars (not pointer/array types)
        std::vector<std::string> intVars;
        for (const auto &v : vars)
        {
            auto *sym = symbols.lookupVariable(v);
            if (sym && sym->type.kind == Type::Kind::Builtin && sym->type.builtin == Type::Builtin::Int)
                intVars.push_back(v);
        }
        if (!intVars.empty())
        {
            return makeVariableRef(intVars[rng.nextInt(0, static_cast<int>(intVars.size() - 1))]);
        }
    }

    if (pick == UnaryChoice && allowNested)
    {
        UnaryOp op = pickUnaryOp(rng);
        auto inner = makeExpr(ctx, depth + 1, vars, types.intType());
        return makeUnary(op, inner, types.intType());
    }

    if (pick == CallChoice && allowNested && hasFuncs)
    {
        // Pick a callable function (not main)
        auto &fns = symbols.allFunctions();
        std::vector<const FunctionSymbol *> callable;
        for (auto &fn : fns)
            if (fn.signature.name != "main")
                callable.push_back(&fn);
        if (!callable.empty())
        {
            const FunctionSymbol *target = callable[rng.nextInt(0, static_cast<int>(callable.size() - 1))];
            ctx.stats.expressions++;
            std::vector<ExprPtr> args;
            for (const auto &paramType : target->signature.parameters)
            {
                args.push_back(makeExpr(ctx, depth + 1, vars, paramType));
            }
            return makeCall(target->signature.name, std::move(args));
        }
    }

    // Binary
    bool useCmp = (hint.kind == Type::Kind::Builtin && hint.builtin == Type::Builtin::Bool);
    std::string op = useCmp ? pickCmpOp(rng) : pickArithOp(rng);
    Type resultType = useCmp ? types.boolType() : types.intType();

    ExprPtr lhs = makeExpr(ctx, depth + 1, vars, types.intType());
    ExprPtr rhs;
    if (op == "/" || op == "%")
    {
        rhs = makeSafeIntLiteral(rng.nextInt(1, 1000));
    }
    else
    {
        rhs = makeExpr(ctx, depth + 1, vars, types.intType());
    }
    return makeSafeBinary(ctx, op, std::move(lhs), std::move(rhs), resultType);
}

StmtPtr ProgramGenerator::makeVarDeclStmt(GenerationContext &ctx, std::vector<std::string> &vars, int depth)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    std::string name = uniqueName("v");

    Type varType = types.intType();
    std::optional<ExprPtr> init;

    if (rng.nextBool(config.weights.memory * 0.7))
    {
        varType = Type::makePointer(types.intType());
        init = makeNewExpr(types.intType());
        ctx.stats.memoryOps++;
    }
    else if (false) // arrays disabled: need brace initialization support
    {
        std::size_t arrSize = static_cast<std::size_t>(rng.nextInt(1, 20));
        varType = Type::makeArray(types.intType(), arrSize);
        ctx.stats.memoryOps++;
    }

    if (!init)
    {
        init = makeExpr(ctx, depth + 1, vars, types.intType());
    }

    symbols.addVariable(VariableSymbol{name, varType});
    vars.push_back(name);
    return makeVarDecl(varType, name, init);
}

StmtPtr ProgramGenerator::makeAssignStmt(GenerationContext &ctx, const std::vector<std::string> &vars, int depth)
{
    if (vars.empty() || !ctx.consumeStatement())
    {
        return nullptr;
    }
    int idx = rng.nextInt(0, static_cast<int>(vars.size() - 1));
    std::string name = vars[idx];
    auto *sym = symbols.lookupVariable(name);
    bool isPtr = sym && sym->type.kind == Type::Kind::Pointer;
    bool isArr = sym && sym->type.kind == Type::Kind::Array;

    if (isPtr)
    {
        if (rng.nextBool(0.5))
        {
            auto val = makeExpr(ctx, depth + 1, vars, types.intType());
            ctx.stats.memoryOps++;
            return makeAssign(makeUnary(UnaryOp::Deref, makeVariableRef(name), types.intType()), val);
        }
        else
        {
            auto newExpr = makeNewExpr(types.intType());
            ctx.stats.memoryOps++;
            return makeAssign(makeVariableRef(name), newExpr);
        }
    }
    if (isArr && rng.nextBool(0.3))
    {
        auto idxExpr = makeExpr(ctx, depth + 1, vars, types.intType());
        auto val = makeExpr(ctx, depth + 1, vars, types.intType());
        ctx.stats.memoryOps++;
        return makeAssign(makeArraySub(makeVariableRef(name), idxExpr, types.intType()), val);
    }
    auto expr = makeExpr(ctx, depth + 1, vars, types.intType());
    return makeAssign(makeVariableRef(name), expr);
}

StmtPtr ProgramGenerator::makeIfStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ctx.noteBranch();

    auto cond = makeExpr(ctx, depth + 1, vars, types.boolType());

    // Sometimes generate ternary instead of if/else (only for int vars, not pointers)
    if (rng.nextBool(0.2) && !vars.empty() && ctx.hasExprBudget() && ctx.consumeStatement())
    {
        // Find an int-typed var
        std::vector<std::string> intVars;
        for (const auto &v : vars)
        {
            auto *sym = symbols.lookupVariable(v);
            if (sym && sym->type.kind == Type::Kind::Builtin && sym->type.builtin == Type::Builtin::Int)
                intVars.push_back(v);
        }
        if (!intVars.empty())
        {
            std::string target = intVars[rng.nextInt(0, static_cast<int>(intVars.size() - 1))];
            auto thenVal = makeExpr(ctx, depth + 2, vars, types.intType());
            auto elseVal = makeExpr(ctx, depth + 2, vars, types.intType());
            return makeAssign(makeVariableRef(target), makeTernary(cond, thenVal, elseVal, types.intType()));
        }
    }

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

    symbols.pushScope();
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
        bodyStmts.push_back(makeAssign(makeVariableRef(counter), incExpr));

        if (!loopVars.empty())
        {
            auto target = loopVars[rng.nextInt(0, static_cast<int>(loopVars.size() - 1))];
            auto addExpr = makeSafeBinary(ctx, "+", makeVariableRef(target), makeSafeIntLiteral(rng.nextInt(1, 3)), types.intType());
            bodyStmts.push_back(makeAssign(makeVariableRef(target), addExpr));
        }

    auto body = makeBlock(bodyStmts);

    auto whileStmt = makeWhile(cond, body);
    symbols.popScope();

    return makeBlock({counterDecl, whileStmt});
}

StmtPtr ProgramGenerator::makeSwitchStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ctx.noteBranch();

    ExprPtr cond = makeExpr(ctx, depth + 1, vars, types.intType());
    // Force int type: (cond + 0) avoids bool-switch warnings
    cond = makeBinary("+", cond, makeSafeIntLiteral(0), types.intType());
    int numCases = rng.nextInt(2, 4);
    std::vector<SwitchCase> cases;
    std::vector<int> usedValues;

    for (int c = 0; c < numCases; ++c)
    {
        if (!ctx.hasExprBudget() || vars.empty())
            break;

        std::optional<ExprPtr> val;
        if (c < numCases - 1 || rng.nextBool(0.7))
        {
            int cv;
            do {
                cv = rng.nextInt(0, 20);
            } while (std::find(usedValues.begin(), usedValues.end(), cv) != usedValues.end());
            usedValues.push_back(cv);
            val = makeSafeIntLiteral(cv);
        }

        std::vector<StmtPtr> body;
        int stmtsInCase = rng.nextInt(1, 2);
        for (int s = 0; s < stmtsInCase; ++s)
        {
            StmtPtr sptr;
            int choice = rng.nextInt(0, 1);
            if (choice == 0 && !vars.empty())
            {
                sptr = makeAssignStmt(ctx, vars, depth + 1);
            }
            else
            {
                auto e = makeExpr(ctx, depth + 1, vars, types.intType());
                sptr = makeExprStmt(e);
                ctx.consumeStatement();
            }
            if (sptr)
                body.push_back(sptr);
        }
        if (rng.nextBool(0.8))
        {
            body.push_back(makeBreak());
        }
        cases.push_back(SwitchCase{val, std::move(body)});
    }

    return makeSwitch(cond, std::move(cases));
}

StmtPtr ProgramGenerator::makeIoStmt(GenerationContext &ctx, int depth, std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }

    if (rng.nextBool(0.6))
    {
        // cout << ...
        auto coutRef = makeVariableRef("std::cout");
        bool hasString = rng.nextBool(0.3);
        ExprPtr ioExpr;

        if (hasString)
        {
            LiteralExpr strLit;
            strLit.kind = LiteralKind::String;
            strLit.value = "val = ";
            strLit.type = types.intType(); // placeholder, not really used
            ioExpr = makeBinary("<<", coutRef, makeLiteral(strLit), types.intType());
        }
        else
        {
            ioExpr = coutRef;
        }

        // Chain << expr
        int numOutputs = rng.nextInt(1, 2);
        for (int i = 0; i < numOutputs; ++i)
        {
            auto val = makeExpr(ctx, depth + 1, vars, types.intType());
            if (i == 0 && !hasString)
            {
                ioExpr = makeBinary("<<", ioExpr, val, types.intType());
            }
            else
            {
                ioExpr = makeBinary("<<", ioExpr, val, types.intType());
            }
        }
        if (rng.nextBool(0.5))
        {
            LiteralExpr endlLit;
            endlLit.kind = LiteralKind::String;
            endlLit.value = "\n";
            ioExpr = makeBinary("<<", ioExpr, makeLiteral(endlLit), types.intType());
        }
        return makeExprStmt(ioExpr);
    }
    else
    {
        // fin >> var (file input, not stdin) — only int (non-pointer) vars
        std::vector<std::string> intVars;
        for (const auto &v : vars)
        {
            auto *sym = symbols.lookupVariable(v);
            if (sym && sym->type.kind == Type::Kind::Builtin && sym->type.builtin == Type::Builtin::Int)
                intVars.push_back(v);
        }
        if (intVars.empty())
        {
            auto temp = makeExpr(ctx, depth + 1, vars, types.intType());
            return makeExprStmt(temp);
        }
        int idx = rng.nextInt(0, static_cast<int>(intVars.size() - 1));
        int val = rng.nextInt(0, 1000);
        inputValues_.push_back(val);
        ioReadCount_++;
        auto finRef = makeVariableRef("fin");
        auto ioExpr = makeBinary(">>", finRef, makeVariableRef(intVars[idx]), types.intType());
        return makeExprStmt(ioExpr);
    }
}

StmtPtr ProgramGenerator::makeReturnStmt(GenerationContext &ctx, const std::vector<std::string> &vars)
{
    if (!ctx.consumeStatement())
    {
        return nullptr;
    }
    ExprPtr expr;
    // Find an int variable (not pointer/array)
    int idx = -1;
    for (int i = 0; i < (int)vars.size(); ++i)
    {
        auto *sym = symbols.lookupVariable(vars[i]);
        if (sym && sym->type.kind == Type::Kind::Builtin && sym->type.builtin == Type::Builtin::Int)
        {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
    {
        expr = makeVariableRef(vars[idx]);
    }
    else
    {
        expr = makeSafeIntLiteral(0);
    }
    return makeReturn(expr);
}

BlockStmt ProgramGenerator::makeFunctionBody(GenerationContext &ctx, int depth, std::vector<std::string> &vars, const std::string &fnName)
{
    std::vector<StmtPtr> stmts;
    int target = std::max(4, config.budget.targetLines / std::max(1, config.budget.maxFunctions));

    while ((int)stmts.size() < target)
    {
        double branchW = config.weights.branch;
        double computeW = config.weights.compute;
        double memoryW = config.weights.memory;
        double ioW = config.weights.io;
        double wDecl = 0.10 + 0.30 * memoryW;
        double wAssign = 0.10 + 0.15 * computeW;
        double wIf = 0.08 + 0.50 * branchW;
        double wWhile = 0.05 + 0.30 * branchW + 0.10 * computeW;
        double wSwitch = 0.05 + 0.40 * branchW;
        double wCompound = 0.05 + 0.15 * computeW;
        double wExpr = 0.05 + 0.30 * computeW;
        double wIo = 0.05 + 0.60 * ioW;

        std::vector<double> weights = {wDecl, wAssign, wIf, wWhile, wSwitch, wCompound, wExpr, wIo};
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
            if (depth < config.budget.maxDepth && !vars.empty())
            {
                stmt = makeSwitchStmt(ctx, depth + 1, vars);
            }
            break;
        case 5:
            if (!vars.empty() && ctx.consumeStatement())
            {
                // Find a non-pointer var for compound assignment
                int idx = -1;
                for (int vi = 0; vi < (int)vars.size(); ++vi) {
                    auto *sym = symbols.lookupVariable(vars[vi]);
                    if (!sym || sym->type.kind == Type::Kind::Builtin) { idx = vi; break; }
                }
                if (idx >= 0) {
                    std::string name = vars[idx];
                    static const std::vector<std::string> compoundOps = {"+=", "-=", "*=", "/=", "%=", "&=", "|=", "^="};
                    std::string op = compoundOps[rng.nextInt(0, static_cast<int>(compoundOps.size() - 1))];
                    ExprPtr val;
                    if (op == "/=" || op == "%=")
                    {
                        // Avoid division/modulo by zero
                        val = makeSafeIntLiteral(rng.nextInt(1, 100));
                    }
                    else
                    {
                        val = makeExpr(ctx, depth + 1, vars, types.intType());
                    }
                    stmt = makeCompoundAssign(op, name, val);
                }
            }
            break;
        case 6:
            if (ctx.consumeStatement() && !vars.empty())
            {
                // Find an int-typed var (not pointer)
                std::vector<std::string> intOnlyVars;
                for (const auto &v : vars) {
                    auto *sym = symbols.lookupVariable(v);
                    if (sym && sym->type.kind == Type::Kind::Builtin && sym->type.builtin == Type::Builtin::Int)
                        intOnlyVars.push_back(v);
                }
                if (!intOnlyVars.empty())
                {
                    int idx = rng.nextInt(0, static_cast<int>(intOnlyVars.size() - 1));
                    std::string name = intOnlyVars[idx];
                    UnaryOp uop = rng.nextBool(0.5) ? UnaryOp::PostInc : UnaryOp::PostDec;
                    auto e = makeUnary(uop, makeVariableRef(name), types.intType());
                    stmt = makeExprStmt(e);
                }
            }
            break;
        case 7:
        default:
            stmt = makeIoStmt(ctx, depth + 1, vars);
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
    fn.body = makeFunctionBody(ctx, 0, vars, "main");

    // Insert calls to helper functions into main before the return statement
    auto allFns = symbols.allFunctions();
    std::vector<StmtPtr> callStmts;
    for (const auto &fnSym : allFns)
    {
        if (fnSym.signature.name == "main")
            continue;
        if (!ctx.consumeStatement())
            break;

        std::vector<ExprPtr> args;
        for (const auto &paramType : fnSym.signature.parameters)
        {
            auto arg = makeExpr(ctx, 0, vars, paramType);
            args.push_back(std::move(arg));
        }
        auto callExpr = makeCall(fnSym.signature.name, std::move(args));
        auto resultVar = uniqueName("v");
        symbols.addVariable(VariableSymbol{resultVar, types.intType()});
        vars.push_back(resultVar);

        auto decl = std::make_shared<Statement>();
        decl->kind = StmtKind::VarDecl;
        decl->data = VarDeclStmt{types.intType(), resultVar, callExpr};
        callStmts.push_back(decl);
        ++stats_.statements;
    }

    // Insert call statements before the final return
    if (!fn.body.statements.empty() && !callStmts.empty())
    {
        auto lastStmt = fn.body.statements.back();
        fn.body.statements.pop_back();
        for (auto &cs : callStmts)
            fn.body.statements.push_back(std::move(cs));
        fn.body.statements.push_back(lastStmt);
    }

    symbols.popScope();
    return fn;
}

Function ProgramGenerator::makeHelperFunction(int index)
{
    Function fn;
    fn.signature.name = "func" + std::to_string(index);

    if (rng.nextBool(config.weights.templ * 0.8))
    {
        fn.isTemplate = true;
        // For template functions, use T for param/return to allow deduction
        // Store a placeholder type; printer will emit "T" for template functions
        Type tType = Type::makeBuiltin(Type::Builtin::Int); // actual type doesn't matter for printing
        fn.signature.returnType = tType;
        fn.signature.parameters = {tType};
    }
    else
    {
        fn.signature.returnType = types.intType();
        fn.signature.parameters = {types.intType()};
    }

    symbols.addFunction(FunctionSymbol{fn.signature});

    GenerationContext ctx(config, symbols, stats_);
    ctx.noteFunction();

    symbols.pushScope();
    std::vector<std::string> vars;
    symbols.addVariable(VariableSymbol{"p0", types.intType()});
    vars.push_back("p0");

    fn.body = makeFunctionBody(ctx, 0, vars, fn.signature.name);

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

    int fnCount = std::max(2, config.budget.maxFunctions);
    for (int i = 0; i < fnCount - 1; ++i)
    {
        program.functions.push_back(makeHelperFunction(i));
    }

    program.functions.push_back(makeMainFunction());
    return program;
}
