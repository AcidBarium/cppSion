#include <iostream>
#include "util/random.h"

int main()
{
    Random rng;

    std::cout << "Random double: "
              << rng.nextDouble()
              << std::endl;

    std::cout << "Random int [1,10]: "
              << rng.nextInt(1, 10)
              << std::endl;

    return 0;
}