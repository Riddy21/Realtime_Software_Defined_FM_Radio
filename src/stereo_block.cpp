#include "dy4.h"
#include "stereo_block.h"

// initialisation
StereoBlock::StereoBlock(int operating_mode){
    If_Fs = IF_FS[operating_mode];
    audio_Fs = AUDIO_FS[operating_mode];

    // decimators for downsampling
    lcm = compute_lcm((unsigned int)If_Fs, (unsigned int)audio_Fs);

    up_decim = lcm / If_Fs;
    down_decim = lcm / audio_Fs;
    
    pilot_state.assign(AUDIO_TAPS-1,0.0);
    audio_state.assign(AUDIO_TAPS-1, 0.0);
    stereo_state.assign(AUDIO_TAPS*up_decim-1, 0.0);

    impulseResponseBPF(If_Fs, PILOT_FC1, PILOT_FC2, AUDIO_TAPS, pilot_filter_coeff);
    impulseResponseBPF(If_Fs, STEREO_FC1, STEREO_FC2, AUDIO_TAPS, audio_filter_coeff);
    impulseResponseLPF(If_Fs*up_decim, MONO_FC, AUDIO_TAPS*up_decim, stereo_filter_coeff);
}

void StereoBlock::setup_vectors(long in_block_size){
    audio_filt.assign(in_block_size, 0.0);
    pilot_filt.assign(in_block_size, 0.0);
    recovered_stereo_carrier.assign(in_block_size, 0.0);
    mixed_stereo_audio.assign(in_block_size, 0.0);
    mixed_stereo_audio_ups.assign(in_block_size*up_decim, 0.0);
    stereo_filt.assign(in_block_size*up_decim, 0.0);
    audio_block.assign(in_block_size*up_decim/down_decim, 0.0);
}

void StereoBlock::process(std::vector<double> &fm_demod,
                        std::vector<double> &audio_block){
    // stereo carrier extraction
    my_convolveFIR(audio_filt, fm_demod, audio_filter_coeff,
                   audio_state, 1, 1);
    
    // stereo carrier recovery through PLL and NCO
    my_convolveFIR(pilot_filt, fm_demod, pilot_filter_coeff,
                   pilot_state, 1, 1);
    fmPLL_stereo(pilot_filt, 19e3, If_Fs, 2.0, 0.0, 0.01, pll_state, recovered_stereo_carrier);

    // stereo processing (pointwise multiplication)
    mix_stereo(audio_filt, recovered_stereo_carrier, mixed_stereo_audio);
    up_sample(mixed_stereo_audio, mixed_stereo_audio_ups, up_decim);
    my_convolveFIR(stereo_filt, mixed_stereo_audio_ups, stereo_filter_coeff,
                   stereo_state, up_decim, down_decim);

    // downsampling
    down_sample(stereo_filt, audio_block, down_decim);
}
