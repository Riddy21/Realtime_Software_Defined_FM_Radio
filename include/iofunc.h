/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#ifndef DY4_IOFUNC_H
#define DY4_IOFUNC_H

// add headers as needed
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <complex>

// declaration of a function prototypes
void printRealVector(const std::vector<float> &);

void printComplexVector(const std::vector<std::complex<float> > &);

void readBinData(const std::string, std::vector<float> &);

void writeBinData(const std::string, const std::vector<float> &);

void readRawData(const std::string, std::vector<float> &);

void readStdinBlockData(unsigned int, std::vector<float> &);

void writeMonoStdoutBlockData(const std::vector<double> &);

void writeStereoStdoutBlockData(const std::vector<double> &, const std::vector<double> &);

#endif // DY4_IOFUNC_H
