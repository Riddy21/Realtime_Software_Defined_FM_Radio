#ifndef DY4_RDS_H
#define DY4_RDS_H

#include "filter.h"
#include "sampling.h"
#include "channel.h"
#include <vector>
#include <string>

class RdsBlock{
    private:
    	// parameters related to the mono block processing
    	float If_Fs;
        float sps;
        float audio_Fs;

    	// states
    	std::vector<double> extraction_state;
        std::vector<double> pllin_state;
    	float pll_state[7] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
        std::vector<double> delayed_RDS_state;
        std::vector<double> mixed_RDS_data_filt_state;
        std::vector<double> rrc_state;
        float demodulation_state;
        bool bits_from_symbols_state;
        bool is_bits_from_symbols_state;
        int last_bit;
        std::vector<bool> differential_bitstream_state;
        bool locate_bitstream;

        std::vector<bool> block;
        char sync_letters[4] = {'A', 'B', 'C', 'D'};
        int counter;
        std::vector<bool> y;
        std::vector<char> offsets;
        std::vector<std::vector<bool>> msgs;
        
        // filtering
        std::vector<double> extraction_filter_coeff;
        std::vector<double> pllin_filter_coeff;
        std::vector<double> demodulation_filter_coeff;
        std::vector<double> rrc_filter_coeff;

        // intermediate vectors
        std::vector<double> extraction_filt;
        std::vector<double> squared_audio_filt;
        std::vector<double> pllin;
        std::vector<double> recovered_RDS_carrier_I;
        std::vector<double> recovered_RDS_carrier_Q;
        std::vector<double> mixed_RDS_data;
        std::vector<double> mixed_RDS_data_filt;
        std::vector<double> downs_mixed_RDS_data_filt;
        std::vector<double> rrc_out;
        std::vector<bool> demodulated_data;
        std::vector<bool> bits_from_symbols;
        std::vector<bool> differential_bitstream;

        void compute_syndrome(std::vector<bool> &,
                              std::vector<bool> &);

        char compute_offset(std::vector<bool> &);

        int list_to_int(std::vector<bool> &x);

        void demodulate_data(std::vector<double> &,
                             std::vector<bool> &);

        void manchester_decode(std::vector<bool> &,
                               std::vector<bool> &);

        bool manchester_diff(bool, bool);

        void differential_decode(std::vector<bool> &,
                               std::vector<bool> &);

    public:
        // Decimators for downsampling
    	unsigned int up_decim;
        unsigned int down_decim;
        unsigned int lcm;


    	// initialisation
    	RdsBlock (int);

        void setup_vectors(long);
    
    	void process(std::vector<double> &,
                     std::vector<double> &);
};

#endif
