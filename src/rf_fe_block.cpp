#include "dy4.h"
#include "rf_fe_block.h"

	// function to split input data to in-phase and quadrature channel
void RfFeBlock::split_sample_channels(const std::vector<float> &sample_data,
                                      std::vector<double> &sample_i,
                                      std::vector<double> &sample_q){

	for (unsigned int i=0; i<sample_data.size(); i++) {
		if (i%2 == 0)
			sample_i[i/2] = (double)sample_data[i];
		else
			sample_q[(i-1)/2] = (double)sample_data[i];
	}
}

	// function to inplement demodulation without arctan
void RfFeBlock::fmDemod_WO_Arctan(const std::vector<double> &I,
                                  const std::vector<double> &Q,
                                  std::vector<double> &demod_signal,
                                  double prev_I, double prev_Q){

    double numerator;
    double denominator;

	for (int k=0; k<(int)(I.size()); k++){

		numerator = (I[k] * (Q[k]-prev_Q) - Q[k] * (I[k]-prev_I));
		denominator = (I[k]*I[k] + Q[k]*Q[k]);

		if(denominator != 0){
			demod_signal[k] = numerator/denominator;
		} else {
			demod_signal[k] = 0;
		}

		prev_I = I[k];
		prev_Q = Q[k];
	}

}

RfFeBlock::RfFeBlock (int operating_mode){
    state_I = 0.0;
	state_Q = 0.0;

	Rf_Fs = RF_FS[operating_mode];
	If_Fs = IF_FS[operating_mode];

	state_i_lpf_100k.resize(RF_TAPS, 0.0);
	state_q_lpf_100k.resize(RF_TAPS, 0.0);

	rf_decim = (unsigned int)(Rf_Fs/If_Fs);

    impulseResponseLPF(Rf_Fs, RF_FC, RF_TAPS, rf_coeff);
	
}

void RfFeBlock::setup_vectors(long in_block_size){
	i_data_block.assign(in_block_size/2, 0.0);
	q_data_block.assign(in_block_size/2, 0.0);

	i_filt.assign(in_block_size/2, 0.0);
	q_filt.assign(in_block_size/2, 0.0);

	i_dsampled.assign(in_block_size/2/rf_decim, 0.0);
	q_dsampled.assign(in_block_size/2/rf_decim, 0.0);
}

	// function to process I/Q data into FM demodulated signal
void RfFeBlock::process(std::vector<float> &iq_data_block,
                        std::vector<double> &fm_demod_data_block){
	// extract I/Q data from input data
	split_sample_channels(iq_data_block, i_data_block, q_data_block);

	// implement the filtering
	my_convolveFIR(i_filt, i_data_block, rf_coeff, state_i_lpf_100k, 1, rf_decim);
	my_convolveFIR(q_filt, q_data_block, rf_coeff, state_q_lpf_100k, 1, rf_decim);

	// downsample the I/Q data from the FM channel
    down_sample(i_filt, i_dsampled, rf_decim);
    down_sample(q_filt, q_dsampled, rf_decim);

	// implement the FM demodulation
	fmDemod_WO_Arctan(i_dsampled, q_dsampled, fm_demod_data_block, state_I, state_Q);
}

