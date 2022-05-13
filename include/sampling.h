#ifndef DY4_SAMPLING_H
#define DY4_SAMPLING_H

#include <vector>
#include <iostream>

int compute_lcm(unsigned int, unsigned int);

int compute_gcd(unsigned int, unsigned int);

void up_sample(const std::vector<double> &,
               std::vector<double> &,
               const unsigned int);

void down_sample(const std::vector<double> &,
                 std::vector<double> &,
                 const unsigned int);

#endif
