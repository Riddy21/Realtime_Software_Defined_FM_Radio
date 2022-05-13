#ifndef DY4_STEREO_H
#define DY4_STEREO_H

#include "filter.h"
#include "sampling.h"
#include "channel.h"
#include <vector>
#include <cmath>

class StereoBlock{
    private:
    	// parameters related to the stereo block processing
    	float If_Fs;
    	float audio_Fs;
    	float pll_state[6] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    	// vector related to the stereo block processing
    	std::vector<double> pilot_state;
		std::vector<double> audio_state;
		std::vector<double> stereo_state;

        std::vector<double> audio_filt;
        std::vector<double> pilot_filt;
        std::vector<double> recovered_stereo_carrier;
        std::vector<double> mixed_stereo_audio;
        std::vector<double> mixed_stereo_audio_ups;
        std::vector<double> stereo_filt;
        std::vector<double> audio_block;

        std::vector<double> pilot_filter_coeff;
        std::vector<double> audio_filter_coeff;
        std::vector<double> stereo_filter_coeff;

    public:
    	unsigned int up_decim;
        unsigned int down_decim;
        unsigned int lcm;


    	// initialisation
    	StereoBlock (int);

        void setup_vectors(long);

    	void process(std::vector<double> &,
                     std::vector<double> &);
};

#endif
