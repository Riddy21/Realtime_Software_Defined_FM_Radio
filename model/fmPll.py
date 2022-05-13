#
# Comp Eng 3DY4 (Computer Systems Integration Project)
#
# Copyright by Nicola Nicolici
# Department of Electrical and Computer Engineering
# McMaster University
# Ontario, Canada
#

import numpy as np
import math

def fmPll(pllIn, freq, Fs, ncoScale = 1.0, phaseAdjust = 0.0, normBandwidth = 0.01, states = [1.0, 0.0, 0.0, 0.0, 0, 1.0], state_q = 0.0, is_RDS = False):

	"""
	pllIn 	 		array of floats
					input signal to the PLL (assume known frequency)

	freq 			float
					reference frequency to which the PLL locks

	Fs  			float
					sampling rate for the input/output signals

	ncoScale		float
					frequency scale factor for the NCO output

	phaseAdjust		float
					phase adjust to be added to the NCO output only
 
	normBandwidth	float
					normalized bandwidth for the loop filter
					(relative to the sampling rate)

	state 			to be added

	"""

	# scale factors for proportional/integrator terms
	# these scale factors were derived assuming the following:
	# damping factor of 0.707 (1 over square root of 2)
	# there is no oscillator gain and no phase detector gain
	Cp = 2.666
	Ci = 3.555

	# gain for the proportional term
	Kp = (normBandwidth)*Cp
	# gain for the integrator term
	Ki = (normBandwidth*normBandwidth)*Ci

	# output array for the NCO
	ncoOuti = np.empty(len(pllIn)+1)
	ncoOutq = np.empty(len(pllIn)+1)

	# initialize internal state from the input
	feedbackI = states[0]
	feedbackQ = states[1]
	integrator = states[2]
	phaseEst = states[3]
	trigOffset = states[4]
	ncoOuti[0] = states[5]
	ncoOutq[0] = state_q
	# note: state saving will be needed for block processing

	for k in range(len(pllIn)):

		# phase detector
		errorI = pllIn[k] * (+feedbackI)  # complex conjugate of the
		errorQ = pllIn[k] * (-feedbackQ)  # feedback complex exponential

		# four-quadrant arctangent discriminator for phase error detection
		errorD = math.atan2(errorQ, errorI)

		# loop filter
		integrator = integrator + Ki*errorD

		# update phase estimate
		phaseEst = phaseEst + Kp*errorD + integrator

		# internal oscillator
		trigOffset += 1
		trigArg = 2*math.pi*(freq/Fs)*(trigOffset) + phaseEst
		feedbackI = math.cos(trigArg)
		feedbackQ = math.sin(trigArg)
		ncoOuti[k+1] = math.cos(trigArg*ncoScale + phaseAdjust)
		ncoOutq[k+1] = math.cos(trigArg*ncoScale + phaseAdjust + math.pi/2)

  # state saving
	states = [feedbackI, feedbackQ, integrator, phaseEst, trigOffset, ncoOuti[-1]]
	state_q = ncoOutq[-1]

	# for stereo only the in-phase NCO component should be returned
	# for block processing you should also return the state
	if not is_RDS:
		return ncoOuti[:-1], states

	# for RDS quadrature part is also needed
	return ncoOuti[:-1], ncoOutq[:-1], states, state_q

if __name__ == "__main__":

	pass
