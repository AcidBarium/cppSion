#pragma once

#include <random>

class Random
{
public:
    Random();

    // 返回 [0,1) 的 double
    double nextDouble();

    // 返回 [min, max] 的 int
    int nextInt(int min, int max);

private:
    std::mt19937 engine;
};