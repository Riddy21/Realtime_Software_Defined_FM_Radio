#include "dy4.h"
#include <complex>
#include <unistd.h>
#include "rds_block.h"

// initialisation
RdsBlock::RdsBlock (int operating_mode){
    if (operating_mode == 1 || operating_mode == 3) {
        std::cerr << "RDS DISABLED\n";
        return;
    }
    std::cerr << "RDS ENABLED\n";

    If_Fs = IF_FS[operating_mode];
    sps = SPS[operating_mode];
    audio_Fs = sps * 2375.0;

    lcm = compute_lcm((unsigned int)If_Fs, (unsigned int)audio_Fs);

    up_decim = lcm / If_Fs;
    down_decim = lcm / audio_Fs;

    //Setup states
    extraction_state.assign(AUDIO_TAPS-1, 0.0);
    pllin_state.assign(AUDIO_TAPS-1, 0.0);
    delayed_RDS_state.assign((AUDIO_TAPS-1)/2, 0.0);
    mixed_RDS_data_filt_state.assign(AUDIO_TAPS * up_decim - 1, 0.0);
    rrc_state.assign(AUDIO_TAPS * up_decim / down_decim -1, 0.0);
    demodulation_state = sps;
    bits_from_symbols_state = true;
    is_bits_from_symbols_state = false;
    last_bit = -1;
    locate_bitstream = false;
    y.resize(10);


    counter = 0;

    //compute filter coeff
    impulseResponseBPF(If_Fs, RDS_EXTRACTION_FC1, RDS_EXTRACTION_FC2,
                       AUDIO_TAPS, extraction_filter_coeff);
    impulseResponseBPF(If_Fs, RDS_PLL_FC1, RDS_PLL_FC2,
                       AUDIO_TAPS, pllin_filter_coeff);
    impulseResponseLPF(If_Fs*up_decim, RDS_DEMOD_FC, AUDIO_TAPS*up_decim,
                       demodulation_filter_coeff);
    impulseResponseRootRaisedCosine(
                       audio_Fs, AUDIO_TAPS*up_decim/down_decim,
                       rrc_filter_coeff);
}

void RdsBlock::compute_syndrome(std::vector<bool> &x,
                                std::vector<bool> &y) {
    if (x.size() != 26) std::cerr << "Wrong size in compute syndrome!" << "\n";

    y[0] = x[0] ^ x[10] ^ x[13] ^ x[14] ^ x[15] ^ x[16] ^ x[17] ^ x[19] ^ x[20] ^ x[23] ^ x[24] ^ x[25];
    y[1] = x[1] ^ x[11] ^ x[14] ^ x[15] ^ x[16] ^ x[17] ^ x[18] ^ x[20] ^ x[21] ^ x[24] ^ x[25];
    y[2] = x[2] ^ x[10] ^ x[12] ^ x[13] ^ x[14] ^ x[18] ^ x[20] ^ x[21] ^ x[22] ^ x[23] ^ x[24];
    y[3] = x[3] ^ x[10] ^ x[11] ^ x[16] ^ x[17] ^ x[20] ^ x[21] ^ x[22];
    y[4] = x[4] ^ x[11] ^ x[12] ^ x[17] ^ x[18] ^ x[21] ^ x[22] ^ x[23];
    y[5] = x[5] ^ x[10] ^ x[12] ^ x[14] ^ x[15] ^ x[16] ^ x[17] ^ x[18] ^ x[20] ^ x[22] ^ x[25];
    y[6] = x[6] ^ x[10] ^ x[11] ^ x[14] ^ x[18] ^ x[20] ^ x[21] ^ x[24] ^ x[25];
    y[7] = x[7] ^ x[10] ^ x[11] ^ x[12] ^ x[13] ^ x[14] ^ x[16] ^ x[17] ^ x[20] ^ x[21] ^ x[22] ^ x[23] ^ x[24];
    y[8] = x[8] ^ x[11] ^ x[12] ^ x[13] ^ x[14] ^ x[15] ^ x[17] ^ x[18] ^ x[21] ^ x[22] ^ x[23] ^ x[24] ^ x[25];
    y[9] = x[9] ^ x[12] ^ x[13] ^ x[14] ^ x[15] ^ x[16] ^ x[18] ^ x[19] ^ x[22] ^ x[23] ^ x[24] ^ x[25];
}

char RdsBlock::compute_offset(std::vector<bool> &x) {
    if (x.size() != 10) std::cerr << "Wrong offset size" << "\n";

    int x_int = list_to_int(x);

    if (x_int == 111) return 'A';
    if (x_int == 175) return 'B';
    if (x_int == 233) return 'C';
    if (x_int == 207) return 'P';
    if (x_int == 105) return 'D';
    
    return 'X';
}

int RdsBlock::list_to_int(std::vector<bool> &x) {
    int data = 0;
    for (int i=x.size() - 1; i >= 0; i--) {
        data *= 2;
        data += int(x[i]);
    }
    return data;
}

void RdsBlock::demodulate_data(std::vector<double> &rrc_out,
                               std::vector<bool> &demodulated_data){
    demodulated_data.clear();
    int i;
    for (i = (int)demodulation_state; i < (int)rrc_out.size(); i += (int)sps){
        demodulated_data.push_back(rrc_out[i] > 0);
    }
    demodulation_state = i - rrc_out.size();
}

void RdsBlock::manchester_decode(std::vector<bool> &demodulated_data,
                                 std::vector<bool> &bits_from_symbols) {
    bits_from_symbols.clear();
    unsigned int start;
    if (is_bits_from_symbols_state){
        bits_from_symbols.push_back(manchester_diff(bits_from_symbols_state, demodulated_data[0]));
        start = 1;
    } else {
        start = 0;
    }

    unsigned int k;
    for (k=start; k<demodulated_data.size()-1; k+=2){
        bits_from_symbols.push_back(
                manchester_diff(demodulated_data[k], demodulated_data[k+1]));
    }

    if (k - 2 == demodulated_data.size()-2){
        is_bits_from_symbols_state = false;
    } else if (k - 2 == demodulated_data.size()-3){
        bits_from_symbols_state = demodulated_data[demodulated_data.size() - 1];
        is_bits_from_symbols_state = true;
    }
    //FIXME: might need to catch the error after, but leaving it be for now
}

bool RdsBlock::manchester_diff(bool s1, bool s2){
    if (s1 && !s2) return true;
    else if (!s1 && s2) return false;
    else {
        //std::cerr << "Decoding error detected" << "\n";
        return s1;
    }
}

void RdsBlock::differential_decode(std::vector<bool> &bits_from_symbols,
        std::vector<bool> &differential_bitstream) {

    differential_bitstream.clear();
    differential_bitstream.push_back(bits_from_symbols[0]);
    for (unsigned int i=1; i<bits_from_symbols.size(); i++){
        differential_bitstream.push_back(bits_from_symbols[i-1] ^ bits_from_symbols[i]);
    }

}

void RdsBlock::setup_vectors(long in_block_size){
    //RDS channel extraction
    extraction_filt.assign(in_block_size, 0.0);

    //Carrier recovery
    squared_audio_filt.assign(in_block_size, 0.0);
    pllin.assign(in_block_size, 0.0);
    recovered_RDS_carrier_I.assign(in_block_size, 0.0);
    recovered_RDS_carrier_Q.assign(in_block_size, 0.0);

    // RDS Demodulation
    mixed_RDS_data.assign(in_block_size*up_decim, 0.0);
    mixed_RDS_data_filt.assign(in_block_size*up_decim, 0.0);
    downs_mixed_RDS_data_filt.assign(in_block_size*up_decim/down_decim, 0.0);
    rrc_out.assign(in_block_size*up_decim/down_decim, 0.0);
    demodulated_data.reserve(rrc_out.size()/sps);

    // Data processing
    bits_from_symbols.reserve(demodulated_data.capacity()/2);
    differential_bitstream.reserve(bits_from_symbols.capacity()*2);
    differential_bitstream_state.reserve(bits_from_symbols.capacity());

}

void RdsBlock::process(std::vector<double> &fm_demod,
                        std::vector<double> &rds_block){

    // RDS channel extraction
    my_convolveFIR(extraction_filt, fm_demod, extraction_filter_coeff,
                   extraction_state, 1, 1);


    // RDS carrier recovery
    // squaring nonlinearity
    mult_channels(extraction_filt, extraction_filt, squared_audio_filt);

    //// bandpass filter
    my_convolveFIR(pllin, squared_audio_filt, pllin_filter_coeff,
                   pllin_state, 1, 1);

    // PLL and NCO
    fmPLL_rds(pllin, 114e3, If_Fs, 0.5, 0.0, 0.001, pll_state, recovered_RDS_carrier_I, recovered_RDS_carrier_Q);

    // all pass filter
    allPassFilt(extraction_filt, delayed_RDS_state, delayed_RDS_state.size());

    // RDS demodulation

    // mixer with zero padding
    mix_and_upscale(recovered_RDS_carrier_I, extraction_filt, mixed_RDS_data, up_decim);

    // low pass filter with resampling (rational resampler)
    my_convolveFIR(mixed_RDS_data_filt, mixed_RDS_data, demodulation_filter_coeff,
                   mixed_RDS_data_filt_state, up_decim, down_decim);

    down_sample(mixed_RDS_data_filt, downs_mixed_RDS_data_filt, down_decim);

    // root=raised cosine filter
    my_convolveFIR(rrc_out, downs_mixed_RDS_data_filt, rrc_filter_coeff,
                   rrc_state, 1, 1);

    // clock and data recovery
    demodulate_data(rrc_out, demodulated_data);

    // RDS data processing
    // Manchester decoding
    manchester_decode(demodulated_data, bits_from_symbols);

    // differential decoding
    differential_decode(bits_from_symbols, differential_bitstream);

    last_bit = bits_from_symbols[bits_from_symbols.size() - 1];

    // frame synchrization
    differential_bitstream.insert(differential_bitstream.begin(), differential_bitstream_state.begin(), differential_bitstream_state.end());

    int i = 0;
    char offset;

    // find the first 26-size block which has the correct offset
    while ((!locate_bitstream) && (i<=(int)differential_bitstream.size()-26)){
        std::vector<bool> block(differential_bitstream.begin()+i, differential_bitstream.begin()+i+26);
        compute_syndrome(block, y);
        offset = compute_offset(y);

        if (offset == 'X') locate_bitstream = false;
        else locate_bitstream = true;

        if (locate_bitstream){
            std::vector<bool> msg(block.begin(), block.begin() + 15);
            if (offset == 'A'){
                counter = 0;
            } else if (offset == 'B'){
                counter = 1;
            } else if ((offset == 'C') || (offset == 'P')){
                counter = 2;
            } else {
                counter = 3;
            }
            i += 26;
            msgs.push_back(msg);
            offsets.push_back(offset);

            break;
        }
        i += 1;
    }
    
    while ((locate_bitstream) && (i<(int)differential_bitstream.size()-26)){
        std::vector<bool> block(differential_bitstream.begin()+i, differential_bitstream.begin()+i+26);

        std::vector<bool> msg(block.begin(), block.begin() + 15);
        counter = (counter + 1) % 4;
        if (counter == 0){
            offset = 'A';
        } else if (counter == 1){
            offset = 'B';
        } else if (counter == 2){
            if ((offset != 'C') && (offset != 'P')){
                offset = 'C';
            }       
        } else {
            offset = 'D';
        }
        msgs.push_back(msg);
        offsets.push_back(offset);
        i += 26;
    }
    std::vector<bool> vec(differential_bitstream.begin() + i, differential_bitstream.end());
    differential_bitstream_state.swap(vec);
} 
