#ifndef DY4_PERFORMANCE_H
#define DY4_PERFORMANCE_H
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

class Performance{
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        std::chrono::time_point<std::chrono::high_resolution_clock> end;
    public:
        void begin();
        void stop();
};

#endif
