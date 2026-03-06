#include "random.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

Random::Random(std::uint64_t seed)
{
    engine = std::mt19937(static_cast<std::mt19937::result_type>(seed));
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

bool Random::nextBool(double trueProbability)
{
    std::bernoulli_distribution dist(trueProbability);
    return dist(engine);
}

std::size_t Random::weightedIndex(const std::vector<double> &weights)
{
    if (weights.empty())
    {
        throw std::invalid_argument("weights must not be empty");
    }

    for (double w : weights)
    {
        if (w < 0.0)
        {
            throw std::invalid_argument("weights must be non-negative");
        }
    }

    double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (sum <= 0.0)
    {
        throw std::invalid_argument("weights sum must be positive");
    }

    std::uniform_real_distribution<double> dist(0.0, sum);
    double pick = dist(engine);

    double cumulative = 0.0;
    for (std::size_t i = 0; i < weights.size(); ++i)
    {
        cumulative += weights[i];
        if (pick <= cumulative)
        {
            return i;
        }
    }

    return weights.size() - 1; // fallback for floating point edge
}

std::uint64_t Random::seedFromDevice()
{
    std::random_device rd;
    return static_cast<std::uint64_t>(rd());
}