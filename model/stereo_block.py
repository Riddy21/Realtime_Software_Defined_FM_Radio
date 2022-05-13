from scipy.io import wavfile
from scipy import signal
import numpy as np
import matplotlib.pyplot as plt
import math, cmath
from config import *
from filter import *
from utils import *
from fmPll import fmPll
from fm_support_lib import fmPlotPSD


class StereoBlock():

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

        self.pilot_state = np.zeros(AUDIO_TAPS-1)
        self.audio_state = np.zeros(AUDIO_TAPS-1)
        self.stereo_state = np.zeros(AUDIO_TAPS*self.up_decim-1)
        self.pll_state = [1.0, 0.0, 0.0, 0.0, 0, 1.0]

        self.pilot_filter_coeff = self.gen_filter_coeff(0)
        self.audio_filter_coeff = self.gen_filter_coeff(1)
        self.stereo_filter_coeff = self.gen_filter_coeff(2)

    def gen_filter_coeff(self, selector):
        """
        selector selects the filter we want
        0 for pilot_filter_coeff,
        1 for audio_filter_coeff,
        2 for stereo_filter_coeff
        """
        if selector == 0:
            return gen_bpf_impulse_response(self.If_Fs,
                                            PILOT_FC1, PILOT_FC2,
                                            AUDIO_TAPS)
        if selector == 1:
            return gen_bpf_impulse_response(self.If_Fs,
                                            STEREO_FC1, STEREO_FC2,
                                            AUDIO_TAPS)
        if selector == 2:
            return gen_lpf_impulse_response(self.If_Fs*self.up_decim,
                                            MONO_FC,
                                            AUDIO_TAPS*self.up_decim)
        return

    def process(self, fm_demod):

        ####### use our own function instead so that only the useful samples are calculated #######
        audio_filt, self.audio_state = my_lfilter(self.audio_filter_coeff, fm_demod, self.audio_state, 1, 1)

        # stereo carrier recovery through PLL and NCO
        pilot_filt, self.pilot_state = my_lfilter(self.pilot_filter_coeff, fm_demod, self.pilot_state, 1, 1)


        recovered_stereo_carrier, self.pll_state = fmPll(pilot_filt, 19e3, self.If_Fs, ncoScale = 2.0, phaseAdjust = 0.0, normBandwidth = 0.01, states = self.pll_state, is_RDS = False)

        # stereo processing (pointwise multiplication)
        mixed_stereo_audio = 2 * audio_filt * recovered_stereo_carrier

        mixed_stereo_audio_ups = up_sample(mixed_stereo_audio, self.up_decim)

        stereo_filt, self.stereo_state = my_lfilter(self.stereo_filter_coeff, mixed_stereo_audio_ups, self.stereo_state, self.up_decim, self.down_decim)

        # Downsampling
        audio_block = stereo_filt[::self.down_decim]
        #plt.plot(mixed_stereo_audio[::self.down_decim])
        #plt.plot(audio_block)
        #plt.show()
        #exit()
        #self.plot(audio_block, mixed_stereo_audio[::self.down_decim], audio_filt[::self.down_decim]/2)

        return audio_block

    def plot(self, plot1, plot2, plot3):

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
        fmPlotPSD(ax0, plot1, self.audio_Fs/1e3, subfig_height[0], \
        		'Original FM demod')
        
        # plot PSextractionD of selected block after extracting mono audio
        ax1.clear()
        fmPlotPSD(ax1, plot2, (self.up_decim*rf_Fs/rf_decim)/1e3, subfig_height[1], \
        		'Channel extraction')
        
        # plot PSD of selected block after downsampling mono audio
        ax2.clear()
        fmPlotPSD(ax2, plot3, (rf_Fs/rf_decim)/1e3, subfig_height[2], \
        		'Pilot channel')


        ## save figure to file
        fig.savefig("../data/fmStereoBlock1.png")


