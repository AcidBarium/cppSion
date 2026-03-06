#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

// Simple seedable RNG wrapper to keep deterministic behavior cross-platform.
class Random
{
public:
    explicit Random(std::uint64_t seed = seedFromDevice());

    // 返回 [0,1) 的 double
    double nextDouble();

    // 返回 [min, max] 的 int
    int nextInt(int min, int max);

    // 带概率的布尔值，默认 50%
    bool nextBool(double trueProbability = 0.5);

    // 按权重选择索引，weights 必须非空且全非负
    std::size_t weightedIndex(const std::vector<double> &weights);

    static std::uint64_t seedFromDevice();

private:
    std::mt19937 engine;
};