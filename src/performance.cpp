#include "performance.h"

void Performance::begin() {
    start = std::chrono::high_resolution_clock::now();
}

void Performance::stop() {
    end = std::chrono::high_resolution_clock::now();

    double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    time_taken *= 1e-9;
    std::cerr  << " Time taken by program is : " << std::fixed << time_taken << std::setprecision(9);
    std::cerr << " sec " << std::endl;
}
