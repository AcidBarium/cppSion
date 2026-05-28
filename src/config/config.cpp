#include "config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
    bool hasFlag(const std::string &arg, const std::string &flag)
    {
        return arg == flag;
    }

    std::string consumeValue(int &i, int argc, char **argv, const std::string &flagName)
    {
        if (i + 1 >= argc)
        {
            throw std::runtime_error("missing value for " + flagName);
        }
        ++i;
        return std::string(argv[i]);
    }

    double parseDouble(const std::string &v)
    {
        return std::stod(v);
    }

    int parseInt(const std::string &v)
    {
        return std::stoi(v);
    }

    std::uint64_t parseUInt64(const std::string &v)
    {
        return static_cast<std::uint64_t>(std::stoull(v));
    }
}

GenerationConfig ConfigParser::fromArgs(int argc, char **argv)
{
    GenerationConfig cfg;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (hasFlag(arg, "--help") || hasFlag(arg, "-h"))
        {
            std::cout << "cppsion options:\n"
                      << "  --seed <u64>              Set RNG seed (0 => random)\n"
                      << "  --out <path>              Output file (default stdout)\n"
                      << "  --emit-ast                Also emit AST JSON to <out>.json or stdout\n"
                      << "  --lines <int>             Target lines\n"
                      << "  --functions <int>         Max functions\n"
                      << "  --complexity <int>        Max statements (budget)\n"
                      << "  --max-depth <int>         Max nested depth\n"
                      << "  --max-expr <int>          Max expression nodes\n"
                      << "  --compute-weight <double> Weight for compute-heavy paths\n"
                      << "  --branch-weight <double>  Weight for branch-heavy paths\n"
                      << "  --memory-weight <double>  Weight for memory-heavy paths\n"
                      << "  --io-weight <double>      Weight for IO-heavy paths\n"
                      << "  --recursion-weight <double> Weight for recursion-heavy paths\n"
                      << "  --template-weight <double>  Weight for template-heavy paths\n"
                      << std::endl;
            std::exit(0);
        }
        else if (hasFlag(arg, "--seed"))
        {
            cfg.seed = parseUInt64(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--out"))
        {
            cfg.outputPath = consumeValue(i, argc, argv, arg);
        }
        else if (hasFlag(arg, "--emit-ast"))
        {
            cfg.emitAstJson = true;
        }
        else if (hasFlag(arg, "--lines"))
        {
            cfg.budget.targetLines = parseInt(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--functions"))
        {
            cfg.budget.maxFunctions = parseInt(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--complexity"))
        {
            cfg.budget.maxStatements = parseInt(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--max-depth"))
        {
            cfg.budget.maxDepth = parseInt(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--max-expr"))
        {
            cfg.budget.maxExprNodes = parseInt(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--compute-weight"))
        {
            cfg.weights.compute = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--branch-weight"))
        {
            cfg.weights.branch = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--memory-weight"))
        {
            cfg.weights.memory = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--io-weight"))
        {
            cfg.weights.io = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--recursion-weight"))
        {
            cfg.weights.recursion = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else if (hasFlag(arg, "--template-weight"))
        {
            cfg.weights.templ = parseDouble(consumeValue(i, argc, argv, arg));
        }
        else
        {
            throw std::runtime_error("unknown flag: " + arg);
        }
    }

    auto checkWeight = [](double w, const std::string &name)
    {
        if (w < 0.0 || w > 1.0)
        {
            throw std::runtime_error(name + " must be in [0, 1], got " + std::to_string(w));
        }
    };
    checkWeight(cfg.weights.compute, "--compute-weight");
    checkWeight(cfg.weights.branch, "--branch-weight");
    checkWeight(cfg.weights.memory, "--memory-weight");
    checkWeight(cfg.weights.io, "--io-weight");
    checkWeight(cfg.weights.recursion, "--recursion-weight");
    checkWeight(cfg.weights.templ, "--template-weight");

    return cfg;
}
