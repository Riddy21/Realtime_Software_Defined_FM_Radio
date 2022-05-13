/*
Comp Eng 3DY4 (Computer Systems Integration Project)
Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#ifndef DY4_FILTER_H
#define DY4_FILTER_H

// add headers as needed
#include <iostream>
#include <vector>
#include <cmath>

// declaration of a function prototypes
void impulseResponseLPF(float, float,
                        unsigned int,
                        std::vector<double> &);

void impulseResponseBPF(float, float, float,
                        unsigned int,
                        std::vector<double> &);

void impulseResponseRootRaisedCosine(float, unsigned int,
                                     std::vector<double> &);

void my_convolveFIR(std::vector<double> &,
                    const std::vector<double> &,
                    const std::vector<double> &,
                    std::vector<double> &,
                    const unsigned int,
                    const unsigned int);

void shiftSamples(std::vector<double> &, int);

void allPassFilt(std::vector<double> &,
                 std::vector<double> &,
                 int);

void fmPLL_stereo(std::vector<double> &,
                  float, float, float, float,
		          float, float*,
		          std::vector<double> &);

void fmPLL_rds(std::vector<double> &,
               float, float, float, float,
		       float, float*,
		       std::vector<double> &,
               std::vector<double> &);


#endif // DY4_FILTER_H
