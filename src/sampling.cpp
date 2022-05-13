#include "dy4.h"
#include "sampling.h"

int compute_lcm(unsigned int x, unsigned int y){
    int lcm = ((int64_t)x*(int64_t)y)/compute_gcd(x, y);
    return lcm;
}

int compute_gcd(unsigned int x, unsigned int y){
    int temp;
    while (y){
        temp = x;
        x = y;
        y = temp % y;
    }
    return x;
}

void up_sample(const std::vector<double> &in_samples,
               std::vector<double> &out_samples,
               const unsigned int sample_rate){

    if (sample_rate == 1){
        out_samples = in_samples;
        return;
    }

    for (int i=0; i < (int)in_samples.size(); i++) {
        out_samples[i*sample_rate] = in_samples[i];
    }
}

void down_sample(const std::vector<double> &in_samples,
               std::vector<double> &out_samples,
               const unsigned int sample_rate){

    if (sample_rate == 1){
        out_samples = in_samples;
        return;
    }

    for (int i=0; i < (int)in_samples.size(); i+=sample_rate) {
        out_samples[i/sample_rate] = in_samples[i];
    }
}

