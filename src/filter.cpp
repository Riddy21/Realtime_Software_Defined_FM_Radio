/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "filter.h"

// function to compute the impulse response "h" based on the sinc function
void impulseResponseLPF(float Fs, float Fc,
                        unsigned int num_taps,
                        std::vector<double> &h)
{
	// bring your own functionality
	// allocate memory for the impulse response
	h.assign(num_taps, 0.0);

	// the rest of the code in this function is to be completed by you
	// based on your understanding and the Python code from the first lab
	float norm_cutoff = Fc / (Fs / 2.0);

	for (unsigned int i = 0; i < num_taps; i++){
        if (i == ((float)num_taps - 1.0)/2.0){
            h[i] = norm_cutoff;
        } else {
			float x = PI * norm_cutoff * (i - ((float)num_taps - 1.0)/2.0);
			h[i] = norm_cutoff * std::sin(x) / x;
		}
		h[i] = h[i] * std::pow(std::sin(i * PI / (float)num_taps), 2.0);
	}
}

// function to generate the impulse response of band-pass filter
void impulseResponseBPF(float Fs, float Fb, float Fe,
                        unsigned int num_taps,
                        std::vector<double> &h)
{
	h.assign(num_taps, 0.0);

	float norm_center = ((Fe+Fb)/2.0) / (Fs/2.0);
	float norm_pass = (Fe-Fb) / (Fs/2.0);
	for (unsigned int i = 0; i<num_taps; i++){
		if(i == ((float)num_taps-1.0)/2.0){
			h[i] = norm_pass;
		} else {
			float x = PI * (norm_pass/2.0) * (i - ((float)num_taps-1.0)/2.0);
			h[i] = norm_pass * std::sin(x) / x;
		}
		h[i] = h[i] * std::cos(i*PI*norm_center);
		h[i] = h[i] * std::pow(std::sin(i*PI/(float)num_taps), 2.0);
	}
}

void impulseResponseRootRaisedCosine(float Fs, unsigned int num_taps,
                                     std::vector<double> &h){
    // duration for each symbol
    float T_symbol = 1.0/2375.0;
    // roll off factor
    float beta = 0.90;

    float t;

    // set size
    h.assign(num_taps, 0.0);

    for (int k=0; k < (int)num_taps; k++){
        t = ((float)k-(float)num_taps/2.0)/Fs;

        if (t == 0.0) {
            h[k] = 1.0 + beta * ((4.0/PI)-1.0);
        } else if (t == -T_symbol/(4.0*beta) || t == T_symbol/(4.0*beta)){
            h[k] = (beta/std::sqrt(2.0)) *
                    (((1.0+2.0/PI)*(std::sin(PI/(4.0*beta)))) +
                     ((1.0-2.0/PI)*(std::cos(PI/(4.0*beta)))));
        } else {
            h[k] = (std::sin(PI*t*(1.0-beta)/T_symbol) +
                    4.0*beta*(t/T_symbol)*std::cos(PI*t*(1.0+beta)/T_symbol))/
                    (PI*t*(1.0-(4.0*beta*t/T_symbol)*(4.0*beta*t/T_symbol))/T_symbol);
        }
    }
}

// function to compute the filtered output "y" by doing the convolution
// of the input data "x" with the impulse response "h"
void my_convolveFIR(std::vector<double> &filtered_x,
                    const std::vector<double> &x,
                    const std::vector<double> &h,
                    std::vector<double> &state,
                    const unsigned int up_decim,
                    const unsigned int down_decim){

    const int block_size = x.size();
    const int filter_size = h.size();

    
	// the rest of the code in this function is to be completed by you
	// based on your understanding and the Python code from the first lab
	for (int i = 0; i < filter_size; i += down_decim){
        filtered_x[i] = 0.0;
		for (int j = i%up_decim; j < filter_size; j += up_decim){
			if ((i-j) >= 0){
                //std::cerr << j <<  "boop\n" << std::flush;
				filtered_x[i] += (double)up_decim * h[j] * x[i-j];
			} else {
				filtered_x[i] += (double)up_decim * h[j] * state[state.size() + i-j];
			}
		}
	}
    
	// the rest of the code in this function is to be completed by you
	// based on your understanding and the Python code from the first lab
	for (int i = ((int)(filter_size/down_decim) + 1)*(down_decim); i < block_size; i += down_decim){
        filtered_x[i] = 0.0;
		for (int j = i%up_decim; j < filter_size; j += up_decim){
			filtered_x[i] += (double)up_decim * h[j] * x[i-j];
		}
	}

    // Save the address of the state as the previous block
    state = x;
}

void shiftSamples(std::vector<double> &vec, int shift){
    std::move(vec.begin(), vec.end() - shift, vec.begin() + shift);
    std::fill(vec.begin(), vec.begin() + shift, 0.0);
}

// FIXME: Not tested
void allPassFilt(std::vector<double> &vec,
                 std::vector<double> &state,
                 int shift){

    vec.insert(vec.begin(), state.end() - shift, state.end());
    state.assign(vec.end() - shift, vec.end());
    vec.resize(vec.size() - shift);
}

void fmPLL_stereo(std::vector<double> &pllIn,
			float freq,
			float Fs,
			float ncoScale,
			float phaseAdjust,
			float normBandwidth,
			float state[6],
			std::vector<double> &ncoOut){
	float Cp = 2.666;
	float Ci = 3.555;

	// gain for the proportional term
	float Kp = normBandwidth*Cp;
	// gian for the integrator term
	float Ki = normBandwidth*normBandwidth*Ci;
	// initialize internal state from the input
	float *feedbackI = &state[0];
	float *feedbackQ = &state[1];
	float *integrator = &state[2];
	float *phaseEst = &state[3];
	float *trigOffset = &state[4];

	ncoOut[0] = (double)state[5];

	for(unsigned int k=0; k<pllIn.size(); k++){
		// phase detector
		float errorI = pllIn[k] * (+*feedbackI);
		float errorQ = pllIn[k] * (-*feedbackQ);
		// four-quadrant arctangent discriminator for phase error detection
		float errorD = std::atan2(errorQ, errorI);
		// loop filter
		*integrator = *integrator + Ki*errorD;
		// update phase estimate
		*phaseEst = *phaseEst + Kp*errorD + *integrator;
		// internal oscillator
		*trigOffset += 1.0;
		float trigArg = 2.0 * PI * (freq/Fs) * *trigOffset + *phaseEst;
		*feedbackI = std::cos(trigArg);
		*feedbackQ = std::sin(trigArg);
        if (k < (pllIn.size() - 1)){
		    ncoOut[k+1] = std::cos(trigArg*ncoScale + phaseAdjust);
        }
        else
		    state[5] = (float)std::cos(trigArg*ncoScale + phaseAdjust);
	}
}

void fmPLL_rds(std::vector<double> &pllIn,
			float freq,
			float Fs,
			float ncoScale,
			float phaseAdjust,
			float normBandwidth,
			float state[7],
			std::vector<double> &ncoOuti,
			std::vector<double> &ncoOutq){

	float Cp = 2.666;
	float Ci = 3.555;

	// gain for the proportional term
	float Kp = normBandwidth*Cp;
	// gian for the integrator term
	float Ki = normBandwidth*normBandwidth*Ci;
	// initialize internal state from the input
	float *feedbackI = &state[0];
	float *feedbackQ = &state[1];
	float *integrator = &state[2];
	float *phaseEst = &state[3];
	float *trigOffset = &state[4];

	ncoOuti[0] = (double)state[5];
	ncoOutq[0] = (double)state[6];

	for(unsigned int k=0; k<pllIn.size(); k++){
		// phase detector
		float errorI = pllIn[k] * (+*feedbackI);
		float errorQ = pllIn[k] * (-*feedbackQ);
		// four-quadrant arctangent discriminator for phase error detection
		float errorD = std::atan2(errorQ, errorI);
		// loop filter
		*integrator = *integrator + Ki*errorD;
		// update phase estimate
		*phaseEst = *phaseEst + Kp*errorD + *integrator;
		// internal oscillator
		*trigOffset += 1.0;
		float trigArg = 2.0 * PI * (freq/Fs) * *trigOffset + *phaseEst;
		*feedbackI = std::cos(trigArg);
		*feedbackQ = std::sin(trigArg);
        if (k < (pllIn.size() - 1)){
		    ncoOuti[k+1] = std::cos(trigArg*ncoScale + phaseAdjust);
		    ncoOutq[k+1] = std::cos(trigArg*ncoScale + phaseAdjust + PI/2.0);
        }
        else {
		    state[5] = (float)std::cos(trigArg*ncoScale + phaseAdjust);
		    state[6] = (float)std::cos(trigArg*ncoScale + phaseAdjust + PI/2.0);
        }
	}

}

