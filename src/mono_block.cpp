#include "dy4.h"
#include "mono_block.h"

// initialisation
MonoBlock::MonoBlock (int operating_mode){
	If_Fs = IF_FS[operating_mode];
	audio_Fs = AUDIO_FS[operating_mode];

	lcm = compute_lcm((unsigned int)If_Fs, (unsigned int)audio_Fs);
    
    up_decim = lcm / If_Fs;
    down_decim = lcm / audio_Fs;
    std::cerr << up_decim << ", " << down_decim << "\n";
    
    state.resize(AUDIO_TAPS*up_decim-1);

    impulseResponseLPF(If_Fs*up_decim, MONO_FC, AUDIO_TAPS*up_decim, mono_coeff);
}

void MonoBlock::setup_vectors(long in_block_size){
    fm_demod_up_sampled.assign(in_block_size*up_decim, 0.0);
    audio_filt.assign(in_block_size*up_decim, 0.0);
}

void MonoBlock::process(std::vector<double> &fm_demod,
                        std::vector<double> &audio_block){
    // upsampling
    up_sample(fm_demod, fm_demod_up_sampled, up_decim);

    // filtering
    my_convolveFIR(audio_filt, fm_demod_up_sampled, mono_coeff,
                   state, up_decim, down_decim);

    // downsampling
    down_sample(audio_filt, audio_block, down_decim);

    // phase shift
    shiftSamples(audio_block, AUDIO_TAPS*up_decim/down_decim/2);

} 
