/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#ifndef DY4_DY4_H
#define DY4_DY4_H

// some general and reusable stuff
// our beloved PI constant
#define PI 3.14159265358979323846

// although we use DFT (no FFT ... yet), the number of points for a
// Fourier transform is defined as NFFT (same as matplotlib)
#define NFFT 512
// design parameters
const float RF_FS[4] = {2400e3,2880e3,2400e3,2304e3};
const float RF_FC = 100e3;
const unsigned int RF_TAPS = 151;

const float IF_FS[4] = {240e3,288e3,240e3,256e3};

const float AUDIO_FS[4] = {48e3,48e3,44.1e3,44.1e3};
const unsigned int AUDIO_TAPS = 101;

const float SPS[4] = {11, 0 ,46, 0};

const unsigned int MAX_QUEUE_SIZE = 200;

const float MONO_FC = 16e3;
const float PILOT_FC1 = 18.5e3;
const float PILOT_FC2 = 19.5e3;
const float STEREO_FC1 = 22e3;
const float STEREO_FC2 = 54e3;
const float STEREO_CARRIER_FC = 16e3;
const float RDS_EXTRACTION_FC1 = 54e3;
const float RDS_EXTRACTION_FC2 = 60e3;
const float RDS_PLL_FC1 = 113.5e3;
const float RDS_PLL_FC2 = 114.5e3;
const float RDS_DEMOD_FC = 3e3;

#endif // DY4_DY4_H
