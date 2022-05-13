#include "dy4.h"
#include "sampling.h"

void add_channels(std::vector<double> &channel_a,
                  std::vector<double> &channel_b,
                  std::vector<double> &out){
    for (unsigned int i = 0; i < out.size(); i++)
        out[i] = (channel_a[i] + channel_b[i])/2.0;
}

void sub_channels(std::vector<double> &channel_a,
                  std::vector<double> &channel_b,
                  std::vector<double> &out){
    for (unsigned int i = 0; i < out.size(); i++)
        out[i] = (channel_a[i] - channel_b[i])/2.0;
}

void mix_stereo(std::vector<double> &channel_a,
                std::vector<double> &channel_b,
                std::vector<double> &out){
    for (unsigned int i = 0; i < out.size(); i++)
        out[i] = channel_a[i] * channel_b[i] * 2.0;
}

void mix_and_upscale(std::vector<double> &channel_a,
                std::vector<double> &channel_b,
                std::vector<double> &out,
                unsigned int up_decim){
    for (unsigned int i = 0; i < out.size(); i+=up_decim)
        out[i] = channel_a[i/up_decim] * channel_b[i/up_decim] * 2.0;
}

void mult_channels(std::vector<double> &channel_a,
                std::vector<double> &channel_b,
                std::vector<double> &out){
    for (unsigned int i = 0; i < out.size(); i++)
        out[i] = channel_a[i] * channel_b[i];
}
