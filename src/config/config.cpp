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

    std::optional<std::string> consumeValue(int &i, int argc, char **argv)
    {
        if (i + 1 >= argc)
        {
            return std::nullopt;
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
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.seed = parseUInt64(*v);
            }
        }
        else if (hasFlag(arg, "--out"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.outputPath = *v;
            }
        }
        else if (hasFlag(arg, "--emit-ast"))
        {
            cfg.emitAstJson = true;
        }
        else if (hasFlag(arg, "--lines"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.budget.targetLines = parseInt(*v);
            }
        }
        else if (hasFlag(arg, "--functions"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.budget.maxFunctions = parseInt(*v);
            }
        }
        else if (hasFlag(arg, "--complexity"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.budget.maxStatements = parseInt(*v);
            }
        }
        else if (hasFlag(arg, "--max-depth"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.budget.maxDepth = parseInt(*v);
            }
        }
        else if (hasFlag(arg, "--max-expr"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.budget.maxExprNodes = parseInt(*v);
            }
        }
        else if (hasFlag(arg, "--compute-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.compute = parseDouble(*v);
            }
        }
        else if (hasFlag(arg, "--branch-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.branch = parseDouble(*v);
            }
        }
        else if (hasFlag(arg, "--memory-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.memory = parseDouble(*v);
            }
        }
        else if (hasFlag(arg, "--io-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.io = parseDouble(*v);
            }
        }
        else if (hasFlag(arg, "--recursion-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.recursion = parseDouble(*v);
            }
        }
        else if (hasFlag(arg, "--template-weight"))
        {
            if (auto v = consumeValue(i, argc, argv))
            {
                cfg.weights.templ = parseDouble(*v);
            }
        }
    }

    return cfg;
}
