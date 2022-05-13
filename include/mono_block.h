#ifndef DY4_MONO_H
#define DY4_MONO_H

#include "filter.h"
#include "sampling.h"
#include <vector>
#include <cmath>

class MonoBlock{
    private:
    	// parameters related to the mono block processing
    	float If_Fs;
    	float audio_Fs;

    	// vector related to the mono block processing
    	std::vector<double> state;

        std::vector<double> mono_coeff;
        
        // Upsampling
        std::vector<double> fm_demod_up_sampled;
            
        // filtering
        std::vector<double> audio_filt;

    public:
    	unsigned int up_decim;
        unsigned int down_decim;
        unsigned int lcm;


    	// initialisation
    	MonoBlock (int);

        void setup_vectors(long);
    
    	void process(std::vector<double> &,
                     std::vector<double> &);
};

#endif
