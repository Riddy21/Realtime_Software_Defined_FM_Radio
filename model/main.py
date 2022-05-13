import sys
import time
import rf_fe_block as rf
import mono_block as mono
import stereo_block as stereo
import RDS_block as rds
import numpy as np
from scipy.io import wavfile
from config import *


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
    # file_name = "../data/stereo_l1_r8.raw"
    file_name = "../data/samples3.raw"
    iq_data = read_iq_data_from_file(file_name)

    ######################
    # Create block objects
    ######################
    rf_fe_block = rf.RfFeBlock(operating_mode)
    mono_block = mono.MonoBlock(operating_mode)
    stereo_block = stereo.StereoBlock(operating_mode)
    if operating_mode == 0 or operating_mode == 2:
        RDS_block = rds.RDSBlock(operating_mode)
    ################
    # Block settings
    ################
    # select a block_size that is a multiple of KB
    # and a multiple of decimation factors
    # FIXME: Change the block size as more decimator are added
    if (mono_block.down_decim < 20):
        block_size = 1024*mono_block.down_decim * rf_fe_block.rf_decim * 2
    else:
        block_size = mono_block.down_decim * rf_fe_block.rf_decim * 2

    # if (RDS_block.down_decim < 50):
    #     block_size = 1024*RDS_block.down_decim * rf_fe_block.rf_decim * 2
    # else:
    #     block_size = RDS_block.down_decim * rf_fe_block.rf_decim * 2
    block_count = 0
    
    ##############
    # Data buffers
    ##############
    # Used to concatenate audio blocks
    mono_audio_data = np.array([])
    stereo_audio_data = np.array([])
    left_channel_data = np.array([])
    right_channel_data = np.array([])
    RDS_msg_data = np.array([])
    RDS_offset_data = np.array([])
    
    # if the number of samples in the last block is less than the block size
    # it is fine to ignore the last few samples from the raw IQ file
    while (block_count+1)*block_size < len(iq_data):
    # while block_count < 50:
        print('Processing block ' + str(block_count))
        # FIXME When doing real time, use this:
        # iq_data = read_iq_data_from_input_stream()
        iq_data_block = iq_data[(block_count)*block_size:(block_count+1)*block_size]
    
        # TODO: Do RF Front end processing
        fm_demod = rf_fe_block.process(iq_data_block)
    
        # TODO: Do mono path
        mono_audio_data_block = mono_block.process(fm_demod)
        mono_audio_data = np.concatenate((mono_audio_data, mono_audio_data_block))

        # # TODO: Do stereo path
        stereo_audio_data_block = stereo_block.process(fm_demod)
        stereo_audio_data = np.concatenate((stereo_audio_data, stereo_audio_data_block))

        # TODO: Do radio data path
        if operating_mode == 0 or operating_mode == 2:
            RDS_msg_data, RDS_offset_data = RDS_block.process(fm_demod)
    
        # FIXME: When doing real time, output audio here
        
        if (operating_mode == 0 or operating_mode == 2) and block_count % 1 == 0:
            for i in range(len(RDS_msg_data)):
                print("block ", RDS_offset_data[i])
                RDS_block.app.get_message(RDS_offset_data[i], RDS_msg_data[i])

            RDS_block.app.print_attr()

        #if block_count == 3:
        #    exit()
    
        block_count += 1


    # calculate left and right channel
    left_channel_data = (mono_audio_data + stereo_audio_data) / 2
    right_channel_data = (mono_audio_data - stereo_audio_data) / 2
    dual_channel = np.array([np.int16(left_channel_data*32767), np.int16(right_channel_data*32767)]).T
    
    print('Finished processing all the blocks from the recorded I/Q samples')
    
    # write audio data to file
    # FIXME: Delete when doing real time processing
    # FIXME: Commented out for now until mono audio is done
    # generate wavefile for mono
    out_fname = "../data/fmMonoBlock.wav"
    wavfile.write(out_fname, int(mono_block.audio_Fs), np.int16((mono_audio_data/2)*32767))
    print("Written audio samples to \"" + out_fname + "\" in signed 16-bit format")

    # generate wavefile for left and right channel
    out_fname = "../data/fmStereoBlock.wav"
    wavfile.write(out_fname, int(mono_block.audio_Fs), dual_channel)
    print("Written audio samples to \"" + out_fname + "\" in signed 16-bit format")


if __name__ == "__main__":
    main()

