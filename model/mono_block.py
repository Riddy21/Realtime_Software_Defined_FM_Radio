from scipy.io import wavfile
import time
from scipy import signal
import numpy as np
import math, cmath
import matplotlib.pyplot as plt
from config import *
from filter import *
from utils import *
from fm_support_lib import fmPlotPSD


class MonoBlock():

    def __init__(self, operating_mode):
        """
        Initialize all related variables in one place to seperate from other variables
        """
        self.If_Fs = IF_FS[operating_mode]
        self.audio_Fs = AUDIO_FS[operating_mode]

        # Decimators for downsampling
        lowest_common_multiple = compute_lcm(self.If_Fs, self.audio_Fs)
        self.up_decim = int(lowest_common_multiple/self.If_Fs) 
        self.down_decim = int(lowest_common_multiple/self.audio_Fs)
        print(self.up_decim, self.down_decim)

        self.state = np.zeros(AUDIO_TAPS*self.up_decim-1)

        self.filter_coeff = self.gen_filter_coeff()

    def gen_filter_coeff(self):
        """
        Generates the impulse response of a low-pass filter for mono path
        """
        ## return signal.firwin(AUDIO_TAPS, MONO_FC/(self.If_Fs/2), pass_zero='lowpass', window=('hann'))
        return gen_lpf_impulse_response(self.If_Fs*self.up_decim,
                                        MONO_FC, AUDIO_TAPS*self.up_decim)


    def process(self, fm_demod):
        # Up sample
        fm_demod_up_sampled = up_sample(fm_demod, self.up_decim)

        ####### use our own function instead so that only the useful samples are calculated #######
        ## audio_filt, self.state = signal.lfilter(self.filter_coeff, 1.0, fm_demod, zi=self.state)
        audio_filt, self.state = my_lfilter(self.filter_coeff, fm_demod_up_sampled, self.state, self.up_decim, self.down_decim)

        # Down sample
        audio_block = audio_filt[::self.down_decim]

        # Phase shift
        shift = int(AUDIO_TAPS*self.up_decim/self.down_decim/2)
        audio_block = np.concatenate((np.zeros(shift), audio_block[:-shift]))

        #self.plot(fm_demod, fm_demod_up_sampled, audio_block)
        return audio_block

    def plot(self, fm_demod, fm_demod_up_sampled, audio_block):

        block_count = 1
        rf_decim = 10
        rf_Fs = 2400e3
        audio_Fs = self.audio_Fs

        #PLOTTING FOR DEBUGGING
        #set up the subfigures for plotting
        subfig_height = np.array([0.8, 2, 1.6]) # relative heights of the subfigures
        plt.rc('figure', figsize=(7.5, 7.5))	# the size of the entire figure
        fig, (ax0, ax1, ax2) = plt.subplots(nrows=3, gridspec_kw={'height_ratios': subfig_height})
        fig.subplots_adjust(hspace = .6)

        ax0.clear()
        fmPlotPSD(ax0, fm_demod, (rf_Fs/rf_decim)/1e3, subfig_height[0], \
        		'Demodulated FM (block ' + str(block_count) + ')')
        
        # plot PSD of selected block after extracting mono audio
        ax1.clear()
        fmPlotPSD(ax1, fm_demod_up_sampled, (self.up_decim*rf_Fs/rf_decim)/1e3, subfig_height[0], \
        		'Demodulated and upsampled FM (block ' + str(block_count) + ')')
        
        # plot PSD of selected block after downsampling mono audio
        ax2.clear()
        fmPlotPSD(ax2, audio_block, audio_Fs/1e3, subfig_height[0], \
        		'Downsampled FM (block ' + str(block_count) + ')')


        ## save figure to file
        fig.savefig("../data/fmMonoBlock" + str(block_count) + ".png")
