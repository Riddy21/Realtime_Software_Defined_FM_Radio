from scipy.io import wavfile
from scipy import signal
import numpy as np
import math, cmath
from config import *
from filter import *
from utils import *
from fmPll import fmPll
from fmRRC import impulseResponseRootRaisedCosine
import application as app

import sys
import rf_fe_block as rf
import time
from scipy.io import wavfile
import matplotlib.pyplot as plt

class RDSBlock():

    def __init__(self, operating_mode):
        """
        Initialize all related variables in one place to seperate from other variables
        """
        if operating_mode not in [0,1,2,3]: raise
        if operating_mode == 1 or operating_mode == 3:
            print("RDS is not supported in this mode!")
            return
            

        self.If_Fs = IF_FS[operating_mode]
        self.SPS = SPS[operating_mode]
        self.audio_Fs = self.SPS * 2375

        # Decimators for downsampling
        lowest_common_multiple = compute_lcm(self.If_Fs, self.audio_Fs)
        self.up_decim = int(lowest_common_multiple/self.If_Fs) 
        self.down_decim = int(lowest_common_multiple/self.audio_Fs)

        # states
        self.extraction_state = np.zeros(AUDIO_TAPS-1)
        self.PLLin_state = np.zeros(AUDIO_TAPS-1)
        self.pll_states = [1.0, 0.0, 0.0, 0.0, 0, 1.0]
        self.pll_state_q = 0.0
        self.delayed_RDS_state = np.zeros(int((AUDIO_TAPS-1)/2))
        self.mixed_RDS_data_filt_state = np.zeros(AUDIO_TAPS * self.up_decim - 1)
        self.RRC_state = np.zeros(int(AUDIO_TAPS * self.up_decim / self.down_decim -1))
        self.demodulation_state = self.SPS
        self.bits_from_symbols_state = True
        self.is_bits_from_symbols_state = False
        self.last_bit = -1
        self.differential_bitstream_state = []
        self.locate_bitstream = False

        # frame sync
        self.sync_letters = ['A','B','C','D']
        self.counter = 0
        self.offsets = []
        self.msgs = []

        # application layer
        self.app = app.ApplicationLayer()

        # filter coefficients
        self.extraction_filter_coeff = self.gen_filter_coeff(0)
        self.PLLin_filter_coeff = self.gen_filter_coeff(1)
        self.demodulation_filter_coeff = self.gen_filter_coeff(2)
        self.RRC_filter_coeff = self.gen_filter_coeff(3)

        # 
        # self.parity_check_matrix = np.matrix([
        #                     0 [1,0,0,0,0,0,0,0,0,0],
        #                     1 [0,1,0,0,0,0,0,0,0,0],
        #                     2 [0,0,1,0,0,0,0,0,0,0],
        #                     3 [0,0,0,1,0,0,0,0,0,0],
        #                     4 [0,0,0,0,1,0,0,0,0,0],
        #                     5 [0,0,0,0,0,1,0,0,0,0],
        #                     6 [0,0,0,0,0,0,1,0,0,0],
        #                     7 [0,0,0,0,0,0,0,1,0,0],
        #                     8 [0,0,0,0,0,0,0,0,1,0],
        #                     9 [0,0,0,0,0,0,0,0,0,1],
        #                     10[1,0,1,1,0,1,1,1,0,0],
        #                     11[0,1,0,1,1,0,1,1,1,0],
        #                     12[0,0,1,0,1,1,0,1,1,1],
        #                     13[1,0,1,0,0,0,0,1,1,1],
        #                     14[1,1,1,0,0,1,1,1,1,1],
        #                     15[1,1,0,0,0,1,0,0,1,1],
        #                     16[1,1,0,1,0,1,0,1,0,1],
        #                     17[1,1,0,1,1,1,0,1,1,0],
        #                     18[0,1,1,0,1,1,1,0,1,1],
        #                     19[1,0,0,0,0,0,0,0,0,1],
        #                     20[1,1,1,1,0,1,1,1,0,0],
        #                     21[0,1,1,1,1,0,1,1,1,0],
        #                     22[0,0,1,1,1,1,0,1,1,1],
        #                     23[1,0,1,0,1,0,0,1,1,1],
        #                     24[1,1,1,0,0,0,1,1,1,1],
        #                     25[1,1,0,0,0,1,1,0,1,1]
        #                     ])

    def compute_syndrome(self, x):

        if (len(x) != 26): raise

        y = [False]*10
        y[0] = bool(x[0]) ^ bool(x[10]) ^ bool(x[13]) ^ bool(x[14]) ^ bool(x[15]) ^ bool(x[16]) ^ bool(x[17]) ^ bool(x[19]) ^ bool(x[20]) ^ bool(x[23]) ^ bool(x[24]) ^ bool(x[25])
        y[1] = bool(x[1]) ^ bool(x[11]) ^ bool(x[14]) ^ bool(x[15]) ^ bool(x[16]) ^ bool(x[17]) ^ bool(x[18]) ^ bool(x[20]) ^ bool(x[21]) ^ bool(x[24]) ^ bool(x[25])
        y[2] = bool(x[2]) ^ bool(x[10]) ^ bool(x[12]) ^ bool(x[13]) ^ bool(x[14]) ^ bool(x[18]) ^ bool(x[20]) ^ bool(x[21]) ^ bool(x[22]) ^ bool(x[23]) ^ bool(x[24])
        y[3] = bool(x[3]) ^ bool(x[10]) ^ bool(x[11]) ^ bool(x[16]) ^ bool(x[17]) ^ bool(x[20]) ^ bool(x[21]) ^ bool(x[22])
        y[4] = bool(x[4]) ^ bool(x[11]) ^ bool(x[12]) ^ bool(x[17]) ^ bool(x[18]) ^ bool(x[21]) ^ bool(x[22]) ^ bool(x[23])
        y[5] = bool(x[5]) ^ bool(x[10]) ^ bool(x[12]) ^ bool(x[14]) ^ bool(x[15]) ^ bool(x[16]) ^ bool(x[17]) ^ bool(x[18]) ^ bool(x[20]) ^ bool(x[22]) ^ bool(x[25])
        y[6] = bool(x[6]) ^ bool(x[10]) ^ bool(x[11]) ^ bool(x[14]) ^ bool(x[18]) ^ bool(x[20]) ^ bool(x[21]) ^ bool(x[24]) ^ bool(x[25])
        y[7] = bool(x[7]) ^ bool(x[10]) ^ bool(x[11]) ^ bool(x[12]) ^ bool(x[13]) ^ bool(x[14]) ^ bool(x[16]) ^ bool(x[17]) ^ bool(x[20]) ^ bool(x[21]) ^ bool(x[22]) ^ bool(x[23]) ^ bool(x[24])
        y[8] = bool(x[8]) ^ bool(x[11]) ^ bool(x[12]) ^ bool(x[13]) ^ bool(x[14]) ^ bool(x[15]) ^ bool(x[17]) ^ bool(x[18]) ^ bool(x[21]) ^ bool(x[22]) ^ bool(x[23]) ^ bool(x[24]) ^ bool(x[25])
        y[9] = bool(x[9]) ^ bool(x[12]) ^ bool(x[13]) ^ bool(x[14]) ^ bool(x[15]) ^ bool(x[16]) ^ bool(x[18]) ^ bool(x[19]) ^ bool(x[22]) ^ bool(x[23]) ^ bool(x[24]) ^ bool(x[25])

        return y

    def compute_offset(self, x):
        if (len(x) != 10): raise
        if (x == [True,True,True,True,False,True,True,False,False,False]):
            return 'A', True
        if (x == [True,True,True,True,False,True,False,True,False,False]):
            return 'B', True
        if (x == [True,False,False,True,False,True,True,True,False,False]):
            return 'C', True
        if (x == [True,True,True,True,False,False,True,True,False,False]):
            return 'C\'', True
        if (x == [True,False,False,True,False,True,True,False,False,False]):
            return 'D', True
        return 'X', False


        # plt.plot(np.arange(len(self.RRC_filter_coeff)), self.RRC_filter_coeff)
        # plt.title("RRC_filter_coeff")
        # plt.show()

        # plt.plot(np.arange(len(self.demodulation_filter_coeff)), self.demodulation_filter_coeff)
        # plt.title("demodulation_filter_coeff")
        # plt.show()
        

    def gen_filter_coeff(self, selector):
        """
        selector selects the filter we want
        0 for pilot_filter_coeff,
        1 for audio_filter_coeff,
        2 for stereo_filter_coeff
        """
        if selector == 0:
            return gen_bpf_impulse_response(self.If_Fs,
                                            54e3, 60e3,
                                            AUDIO_TAPS)
        if selector == 1:
            return gen_bpf_impulse_response(self.If_Fs,
                                            113.5e3, 114.5e3,
                                            AUDIO_TAPS)
        if selector == 2:
            return gen_lpf_impulse_response(self.If_Fs*self.up_decim,
                                            3e3,
                                            AUDIO_TAPS*self.up_decim)
        if selector == 3:
            return impulseResponseRootRaisedCosine(self.audio_Fs, int(AUDIO_TAPS * self.up_decim / self.down_decim))
        return

    def allPass(self, input_block, state_block):

        output_block = np.concatenate((state_block, input_block[:-len(state_block)]))
        state_block = input_block[-len(state_block):]
        
        return output_block, state_block

    def Manchester_differential_decoding(self, s1, s2):
        if s1 == True and s2 == False:
            return True
        if s1 == False and s2 == True:
            return False
        else:
            print("decoding error detected.")
            return s1

    def process(self, fm_demod):

        """RDS channel extraction"""
        extraction_filt, self.extraction_state = my_lfilter(self.extraction_filter_coeff, fm_demod, self.extraction_state)

        # plt.plot(np.arange(len(extraction_filt)), extraction_filt)
        # plt.title("extraction_filt")
        # plt.show()

        """ RDS carrier recovery """
        # Squaring Nonlinearity
        squared_audio_filt = extraction_filt * extraction_filt

        # plt.plot(np.arange(len(squared_audio_filt)), squared_audio_filt)
        # plt.title("squared_audio_filt")
        # plt.show()

        # Bandpass filter
        PLLin, self.PLLin_state = my_lfilter(self.PLLin_filter_coeff, squared_audio_filt, self.PLLin_state)

        # plt.plot(np.arange(len(PLLin)), PLLin)
        # plt.title("PLLin")
        # plt.show()

        # PLL and NCO
        recovered_RDS_carrier_I, recovered_RDS_carrier_Q, self.pll_states, self.pll_state_q = fmPll(PLLin, 114e3, self.If_Fs, ncoScale = 0.5, phaseAdjust = 0.0, normBandwidth = 0.001, states = self.pll_states, state_q = self.pll_state_q, is_RDS = True)

        # plt.plot(np.arange(len(recovered_RDS_carrier_I)), recovered_RDS_carrier_I)
        # plt.title("recovered_RDS_carrier_I")
        # plt.show()

        # All pass filter
        delayed_RDS, self.delayed_RDS_state = self.allPass(extraction_filt, self.delayed_RDS_state)
        # plt.plot(np.arange(len(PLLin)), PLLin*2000, 'r', np.arange(len(delayed_RDS)), delayed_RDS, 'b')
        # plt.title("delayed_RDS")
        # plt.show()

        """ RDS demodulation """
        # mixer with zero padding
        mixed_RDS_data = np.zeros(self.up_decim*len(recovered_RDS_carrier_I))
        for i in range(len(recovered_RDS_carrier_I)):
            mixed_RDS_data[self.up_decim*i] = 2 * recovered_RDS_carrier_I[i] * delayed_RDS[i]

        # plt.plot(np.arange(len(mixed_RDS_data)), mixed_RDS_data)
        # plt.title("mixed_RDS_data")
        # plt.show()
      
        # plt.plot(np.arange(len(PLLin)), PLLin*2000, 'r', np.arange(len(delayed_RDS)), delayed_RDS, 'b')
        # plt.title("delayed_RDS")
        # plt.show()

        # low pass filter with resampling (rational resampler)
        mixed_RDS_data_filt, self.mixed_RDS_data_filt_state = my_lfilter(self.demodulation_filter_coeff, mixed_RDS_data, self.mixed_RDS_data_filt_state, self.up_decim, self.down_decim)
        downsampled_mixed_RDS_data_filt = mixed_RDS_data_filt[::self.down_decim]

        # plt.plot(np.arange(len(downsampled_mixed_RDS_data_filt)), downsampled_mixed_RDS_data_filt)
        # plt.title("downsampled_mixed_RDS_data_filt")
        # plt.show()
        
        # root-raised cosine filter
        RRC_out, self.RRC_state = my_lfilter(self.RRC_filter_coeff, downsampled_mixed_RDS_data_filt, self.RRC_state)
        # RRC_out = RRC_out[24:]
        # plt.plot(np.arange(len(downsampled_mixed_RDS_data_filt)) + 5, downsampled_mixed_RDS_data_filt, 'r', np.arange(len(RRC_out)), RRC_out, 'b')
        # plt.title("RRC_out")
        # plt.show()

        # clock and data recovery
        demodulated_data = []
        i = self.demodulation_state
        while i < len(RRC_out):
            demodulated_data.append(RRC_out[i]>0)
            i += self.SPS
        self.demodulation_state = i - len(RRC_out)
        # print(demodulated_data)

        """RDS data processing"""
        # Manchester decoding
        bits_from_symbols = []
        if self.is_bits_from_symbols_state == True:
            bits_from_symbols.append(self.Manchester_differential_decoding(self.bits_from_symbols_state, demodulated_data[0]))
            start = 1
        else:
            start = 0

        for i in range(start, len(demodulated_data)-1, 2):
            bits_from_symbols.append(self.Manchester_differential_decoding(demodulated_data[i], demodulated_data[i+1]))

        if i == len(demodulated_data) - 2:
            self.is_bits_from_symbols_state = False
        elif i == len(demodulated_data) - 3:
            self.bits_from_symbols_state = demodulated_data[-1]
            self.is_bits_from_symbols_state = True
        else:
            raise

        # differential decoding
        differential_bitstream = [0] * len(bits_from_symbols)
        if self.last_bit == -1:
            # -1 means it's the first block which has no state
            differential_bitstream[0] = int(bits_from_symbols[0])
        else:
            differential_bitstream[0] = int(bool(self.last_bit)^bool(bits_from_symbols[0]))

        for i in range(1, len(bits_from_symbols)):
            differential_bitstream[i] = int(bool(bits_from_symbols[i-1])^bool(bits_from_symbols[i]))

        self.last_bit = bits_from_symbols[-1]

        
        ## frame synchronization
        differential_bitstream = np.concatenate((self.differential_bitstream_state, differential_bitstream))
        i = 0

        self.offsets = []
        self.msgs = []

        # find the first 26-size block which has the correct offset
        while not self.locate_bitstream and i <= len(differential_bitstream) - 26:
            block = differential_bitstream[i:i+26]
            for j in range(len(block)):
                block[j] = bool(block[j])

            offset, self.locate_bitstream = self.compute_offset(self.compute_syndrome(block))
            # print("offset ", offset)

            if self.locate_bitstream:
                msg = block[:16]
                if offset == 'A':
                    self.counter = 0
                elif offset == 'B':
                    self.counter = 1
                elif offset == 'C' or offset == 'C\'':
                    self.counter = 2
                else:
                    self.counter = 3
                i += 26

                self.msgs.append(msg)
                self.offsets.append(offset)

                break
            i += 1

        # after finding the first block, we just need to increase the index by 26 every time
        while self.locate_bitstream and i <= len(differential_bitstream) - 26:
            block = differential_bitstream[i:i+26]
            for j in range(len(block)):
                block[j] = bool(block[j])
            msg = block[:16]
            offset, _ = self.compute_offset(self.compute_syndrome(block))
            self.counter = (self.counter + 1) % 4
            if self.counter == 0:
                offset = 'A'
            elif self.counter == 1:
                offset = 'B'
            elif self.counter == 2:
                if offset != 'C' and offset != 'C\'':
                    offset = 'C'
            else:
                offset = 'D'

            # print("offset ", offset)
            self.msgs.append(msg)
            self.offsets.append(offset)
            i += 26
        self.differential_bitstream_state = differential_bitstream[i:]

        ## application layer
        # self.app.get_message(self.msgs)

        return self.msgs, self.offsets


def read_iq_data_from_file(in_fname):
    """Read the IQ data from the recorded file"""
    raw_data = np.fromfile(in_fname, dtype='uint8')
    print("Read raw RF data from \"" + in_fname + "\" in unsigned 8-bit format")

    # IQ data is normalized between -1 and +1 in 32-bit float format
    iq_data = (np.float32(raw_data) - 128.0)/128.0
    print("Reformatted raw RF data to 32-bit float format (" + str(iq_data.size * iq_data.itemsize) + " bytes)")

    return iq_data

def read_iq_data_from_input_stream():
    """Read the IQ data from input stream"""
    # TODO: Read block from the input stream
    pass

def main():
    ####################
    # Get operating mode
    ####################
    if len(sys.argv) == 1:
        operating_mode = 0
    elif len(sys.argv) == 2:
        operating_mode = int(sys.argv[1])
    else:
        raise ValueError("Expected to have at most one argument.")

    # Read IQ data
    # FIXME: When performing block processing, delete this, use console read
    # file_name = "../data/iq_samples.raw"
    # file_name = "../data/stereo_l1_r8.raw"
    file_name = "../data/samples3.raw"
    iq_data = read_iq_data_from_file(file_name)

    ######################
    # Create block objects
    ######################
    rf_fe_block = rf.RfFeBlock(operating_mode)
    RDS_block = RDSBlock(operating_mode)
    ################
    # Block settings
    ################
    # select a block_size that is a multiple of KB
    # and a multiple of decimation factors
    # FIXME: Change the block size as more decimator are added
    if (RDS_block.down_decim < 50):
        block_size = 1024*RDS_block.down_decim * rf_fe_block.rf_decim * 2
    else:
        block_size = RDS_block.down_decim * rf_fe_block.rf_decim * 2
    block_count = 0
    
    ##############
    # Data buffers
    ##############
    # Used to concatenate audio blocks
    demodulated_data = np.array([])
    RDS_audio_data = np.array([])
    
    # if the number of samples in the last block is less than the block size
    # it is fine to ignore the last few samples from the raw IQ file
    # while (block_count+1)*block_size < len(iq_data):
    while (block_count < 200):
        print('Processing block ' + str(block_count))
        # FIXME When doing real time, use this:
        # iq_data = read_iq_data_from_input_stream()
        iq_data_block = iq_data[(block_count)*block_size:(block_count+1)*block_size]
    
        # TODO: Do RF Front end processing
        fm_demod = rf_fe_block.process(iq_data_block)
    
        # # TODO: Do mono path
        # mono_audio_data_block = mono_block.process(fm_demod)
        # mono_audio_data = np.concatenate((mono_audio_data, mono_audio_data_block))
    
        # # TODO: Do stereo path
        # stereo_audio_data_block = stereo_block.process(fm_demod)
        # stereo_audio_data = np.concatenate((stereo_audio_data, stereo_audio_data_block))

        # TODO: Do radio data path
        # demodulated_block, RDS_audio_data_block = RDS_block.process(fm_demod)
        # demodulated_data = np.concatenate((demodulated_data, demodulated_block))
        # RDS_audio_data = np.concatenate((RDS_audio_data, RDS_audio_data_block))
        msgs, offsets = RDS_block.process(fm_demod)
        print(msgs)
        print(offsets)
        # FIXME: When doing real time, output audio here
    
        block_count += 1

    # plt.plot(np.arange(len(RDS_audio_data))*(RDS_block.SPS)+RDS_block.SPS/2+5, RDS_audio_data, 'bo')
    # # plt.plot((np.arange(len(demodulated_data))-5)/11, demodulated_data)
    # plt.plot(np.arange(len(demodulated_data)), demodulated_data)
    # plt.show()

    # calculate left and right channel
    # left_channel_data = (mono_audio_data + stereo_audio_data) / 2
    # right_channel_data = (mono_audio_data - stereo_audio_data) / 2
    # dual_channel = np.array([np.int16(left_channel_data*32767), np.int16(right_channel_data*32767)]).T

    print('Finished processing all the blocks from the recorded I/Q samples')
    # plt.plot(np.arrange(len(RDS_audio_data)), RDS_audio_data)
    # plt.show()
    
    # write audio data to file
    # FIXME: Delete when doing real time processing
    # FIXME: Commented out for now until mono audio is done
    # generate wavefile for mono
    # out_fname = "../data/fmMonoBlock.wav"
    # wavfile.write(out_fname, int(mono_block.audio_Fs), np.int16((mono_audio_data/2)*32767))
    # print("Written audio samples to \"" + out_fname + "\" in signed 16-bit format")

    # # generate wavefile for left and right channel
    # out_fname = "../data/fmStereoBlock.wav"
    # wavfile.write(out_fname, int(mono_block.audio_Fs), dual_channel)
    # print("Written audio samples to \"" + out_fname + "\" in signed 16-bit format")


if __name__ == "__main__":
    main()



