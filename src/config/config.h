#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct FeatureWeights
{
    double compute = 0.4;
    double branch = 0.2;
    double memory = 0.1;
    double io = 0.1;
    double recursion = 0.1;
    double templ = 0.1;
};

struct ComplexityBudget
{
    int maxDepth = 8;
    int maxStatements = 500;
    int maxExprNodes = 2000;
    int maxFunctions = 8;
    int targetLines = 200;
};

struct GenerationConfig
{
    std::uint64_t seed = 0; // 0 means derive from random_device
    FeatureWeights weights;
    ComplexityBudget budget;
    bool emitAstJson = false;
    std::string outputPath; // empty => stdout
};

class ConfigParser
{
public:
    static GenerationConfig fromArgs(int argc, char **argv);
};
