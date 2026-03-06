#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "ast/printer.h"
#include "config/config.h"
#include "generator/program_generator.h"
#include "util/random.h"

namespace
{
    std::ostream &selectStream(const std::string &path, std::optional<std::ofstream> &file)
    {
        if (path.empty())
        {
            return std::cout;
        }

        file.emplace(path, std::ios::out | std::ios::trunc);
        return *file;
    }
}

int main(int argc, char **argv)
{
    try
    {
        auto cfg = ConfigParser::fromArgs(argc, argv);

        std::uint64_t seed = cfg.seed == 0 ? Random::seedFromDevice() : cfg.seed;
        Random rng(seed);

        ProgramGenerator generator(cfg, rng);
        Program program = generator.generate();

        AstPrinter printer;
        std::optional<std::ofstream> outFile;
        std::ostream &out = selectStream(cfg.outputPath, outFile);
        printer.printProgram(program, out);

        if (cfg.emitAstJson)
        {
            std::optional<std::ofstream> jsonFile;
            std::ostream *jsonOut = &std::cout;
            if (!cfg.outputPath.empty())
            {
                jsonFile.emplace(cfg.outputPath + ".json", std::ios::out | std::ios::trunc);
                jsonOut = &(*jsonFile);
            }
            printer.printAstJson(program, *jsonOut);
        }

        const auto &stats = generator.stats();
        std::cerr << "cppSion stats\n"
                  << "seed: " << seed << "\n"
                  << "functions: " << stats.functions << "\n"
                  << "statements: " << stats.statements << "\n"
                  << "loops: " << stats.loops << "\n"
                  << "branches: " << stats.branches << "\n"
                  << "expressions: " << stats.expressions << "\n"
                  << "max depth: " << stats.maxDepth << "\n"
                  << "memory ops: " << stats.memoryOps << "\n";

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "cppSion failed: " << ex.what() << "\n";
        return 1;
    }
}