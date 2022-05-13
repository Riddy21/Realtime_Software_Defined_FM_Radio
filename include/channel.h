#ifndef DY4_CHANNEL_H
#define DY4_CHANNEL_H

#include <vector>

void add_channels(std::vector<double> &, std::vector<double> &,
                  std::vector<double> &);

void sub_channels(std::vector<double> &, std::vector<double> &,
                  std::vector<double> &);

void mix_and_upscale(std::vector<double> &, std::vector<double> &,
                std::vector<double> &, unsigned int);

void mix_stereo(std::vector<double> &, std::vector<double> &,
                  std::vector<double> &);

void mult_channels(std::vector<double> &, std::vector<double> &,
                  std::vector<double> &);

#endif
