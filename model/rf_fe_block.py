from scipy.io import wavfile
from scipy import signal
import numpy as np
import math, cmath
from config import *
from filter import *

# use fmDemodArctan and fmPlotPSD
from fm_support_lib import fmDemod_WO_Arctan

class RfFeBlock(object):
    def __init__(self, operating_mode):
        """
        Initialize all related variables in one place to seperate from other variables
        """
        self.Rf_Fs = RF_FS[operating_mode]
        self.If_Fs = IF_FS[operating_mode]

        # Decimators for downsampling
        self.rf_decim = int(self.Rf_Fs/self.If_Fs)

        self.state_i_lpf_100k = np.zeros(RF_TAPS-1)
        self.state_q_lpf_100k = np.zeros(RF_TAPS-1)

        self.state_I = 0
        self.state_Q = 0

        self.rf_coeff = self.gen_rf_filter_coeff()

    def gen_rf_filter_coeff(self):
        """Generates the impulse response of a low-pass filter for RF"""

        #return signal.firwin(RF_TAPS, RF_FC/(self.Rf_Fs/2), window=('hann'))
        return gen_lpf_impulse_response(self.Rf_Fs, RF_FC, RF_TAPS)
    
    def process(self, iq_data_block):
        """
        Process I and Q samples into fm demodulated signal
        """

    	# filter to extract the FM channel (I samples are even, Q samples are odd)
        i_filt, self.state_i_lpf_100k = signal.lfilter(self.rf_coeff, 1.0,
                                                       iq_data_block[::2],
                                                       zi=self.state_i_lpf_100k)
        q_filt, self.state_q_lpf_100k = signal.lfilter(self.rf_coeff, 1.0,
                                                       iq_data_block[1::2],
                                                       zi=self.state_q_lpf_100k)

        # downsample the I/Q data from the FM channel
        i_ds = i_filt[::self.rf_decim]
        q_ds = q_filt[::self.rf_decim]


        fm_demod, self.state_I, self.state_Q = fmDemod_WO_Arctan(i_ds, q_ds, self.state_I, self.state_Q)

        return fm_demod
    
