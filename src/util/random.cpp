#include "random.h"

Random::Random()
{
    std::random_device rd;
    engine = std::mt19937(rd());
}

double Random::nextDouble()
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(engine);
}

int Random::nextInt(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine);
}