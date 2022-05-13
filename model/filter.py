import math
import numpy as np

def gen_lpf_impulse_response(Fs, Fc, N_taps):
        h = np.zeros(shape = N_taps)

        # Define explicitly the normalized cutoff frequency
        norm_cutoff = Fc / (Fs / 2.0)
        
        for i in range(N_taps):
                if i == (N_taps - 1.0)/2.0:
                        # avoid division by zero in sinc when N_taps is odd
                        h[i] = norm_cutoff
                else:
                        x = math.pi * norm_cutoff * (i - (N_taps - 1.0) / 2.0)
                        h[i] = norm_cutoff * math.sin(x) / x
                # apply the Hann window
                h[i] = h[i] * math.sin(i * math.pi / N_taps) ** 2.0

        return h

def gen_bpf_impulse_response(Fs, Fb, Fe, N_taps):
        # Fb is the beginning of the pass band
        # Fe is the end of the pass band

        h = np.zeros(shape = N_taps)

        # Define explicitly the normalized center frequency and pass band
        norm_center = ((Fe + Fb)/2.0) / (Fs/2.0)
        norm_pass = (Fe - Fb) / (Fs/2.0) 
        
        for i in range(N_taps):
                if i == (N_taps - 1.0)/2.0:
                        # avoid division by zero in sinc for the center tap when N_taps is odd
                        h[i] = norm_pass
                else:
                        x = math.pi * (norm_pass/2.0) * (i - (N_taps - 1.0) / 2.0)
                        h[i] = norm_pass * math.sin(x) / x
                # apply a freqency shift by the center frequency
                h[i] = h[i] * math.cos(i * math.pi * norm_center)
                # apply the Hann window
                h[i] = h[i] * math.sin(i * math.pi / N_taps) ** 2.0

        return h

def my_lfilter(coeff, x_block, state, up_decim=1, down_decim=1):
        
        block_size = len(x_block)
        filter_len = len(coeff)
        filtered_x = np.zeros(shape = block_size)
        
        # TODO: Make more efficient by removing useless iterations
        for i in range(0, block_size, down_decim):
            for j in range(i%up_decim, filter_len, up_decim):
                if i - j >= 0:
                    filtered_x[i] += up_decim * coeff[j] * x_block[i-j]
                else:
                    filtered_x[i] += up_decim * coeff[j] * state[i-j]

        return filtered_x, x_block[-filter_len + 1:]
