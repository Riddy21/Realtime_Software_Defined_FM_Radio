#ifndef DY4_RF_FE_H
#define DY4_RF_FE_H

#include "filter.h"
#include "sampling.h"
#include <vector>
#include <cmath>

class RfFeBlock{
    private:
	    // parameters related to the demodulation processing
	    double state_I;
	    double state_Q;

	    // parameters related to the front-end processing
	    float Rf_Fs;
	    float If_Fs;

        std::vector<double> rf_coeff;

	    // vectors related to the front-end processing
	    std::vector<double> state_i_lpf_100k;
	    std::vector<double> state_q_lpf_100k;

	    // storing i q data
	    std::vector<double> i_data_block;
	    std::vector<double> q_data_block;

    	// storing filtered data
    	std::vector<double> i_filt;
    	std::vector<double> q_filt;

	    // storing downsampled
	    std::vector<double> i_dsampled;
	    std::vector<double> q_dsampled;

        void split_sample_channels(const std::vector<float> &,
                                   std::vector<double> &,
                                   std::vector<double> &);

	    void fmDemod_WO_Arctan(const std::vector<double> &,
                               const std::vector<double> &,
                               std::vector<double> &,
                               double,
                               double);
        
    public:
	    unsigned int rf_decim;

	    RfFeBlock (int);

        void setup_vectors(long);

	    void process(std::vector<float> &,
                     std::vector<double> &);
};

#endif

