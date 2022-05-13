/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "iofunc.h"

// some basic functions for printing information from vectors
// or to read from/write to binary files in 32-bit float format
void printRealVector(const std::vector<float> &x)
{
	std::cout << "Printing float vector of size " << x.size() << "\n";
	for (unsigned int i = 0; i < x.size(); i++)
		std::cout << x[i] << " ";
	std::cout << "\n";
}

void printComplexVector(const std::vector<std::complex<float>> &X)
{
	std::cout << "Printing complex vector of size " << X.size() << "\n";
	for (unsigned int i = 0; i < X.size(); i++)
		std::cout << X[i] << " ";
	std::cout << "\n";
}

// assumes data in the raw binary file is in 32-bit float format
void readBinData(const std::string in_fname, std::vector<float> &bin_data)
{
	std::ifstream fdin(in_fname, std::ios::binary);
	if(!fdin) {
		std::cout << "File " << in_fname << " not found ... exiting\n";
		exit(1);
	} else {
		std::cout << "Reading raw binary from \"" << in_fname << "\"\n";
	}
	fdin.seekg(0, std::ios::end);
	const unsigned int num_samples = fdin.tellg() / sizeof(uint8_t);

	bin_data.resize(num_samples);
	fdin.seekg(0, std::ios::beg);
	fdin.read(reinterpret_cast<char*>(&bin_data[0]), num_samples*sizeof(float));
	fdin.close();
}

// assumes data in the raw binary file is 32-bit float format
void writeBinData(const std::string out_fname, const std::vector<float> &bin_data)
{
	std::cout << "Writing raw binary to \"" << out_fname << "\"\n";
	std::ofstream fdout(out_fname, std::ios::binary);
	for (unsigned int i=0; i<bin_data.size(); i++) {
		fdout.write(reinterpret_cast<const char*>(&bin_data[i]),\
								sizeof(bin_data[i]));
	}
	fdout.close();
}

void readRawData(const std::string in_fname, std::vector<float> &raw_data){
	std::ifstream myData(in_fname, std::ios::binary);
	raw_data.clear();
	char buf[sizeof(uint8_t)];
	while(myData.read(buf, sizeof(uint8_t))){
		raw_data.push_back(((float)(uint8_t)*buf - (float)128.0)/(float)128.0);
	}
	myData.close();	
}

void readStdinBlockData(unsigned int num_samples,
        std::vector<float> &block_data){
    std::vector<char> raw_data(num_samples);
    std::cin.read(reinterpret_cast<char*>(&raw_data[0]), num_samples*sizeof(char));
    for (int k=0; k<(int)num_samples; k++) {
        //normalizes from range -1 to 1
        block_data[k] = float(((unsigned char)raw_data[k]-128)/128.0);
    }
}

void writeMonoStdoutBlockData(const std::vector<double> &processed_data){
    std::vector<short int> audio_data(processed_data.size());
    for (unsigned int k=0; k<processed_data.size(); k++) {
        if (std::isnan(processed_data[k])){
            audio_data[k] = 0;
        } else {
            audio_data[k] = static_cast<short int>(processed_data[k] * 16384);
        }
    } 
    fwrite(&audio_data[0], sizeof(short int), audio_data.size(), stdout);
}

void writeStereoStdoutBlockData(const std::vector<double> &processed_data_l,
                                const std::vector<double> &processed_data_r){
    std::vector<short int> audio_data(processed_data_l.size()*2);
    for (unsigned int k=0; k<processed_data_l.size()*2; k++) {
        if (k%2 == 0){
            if (std::isnan(processed_data_l[k])){
                audio_data[k] = 0;
            } else {
                audio_data[k] = static_cast<short int>(processed_data_l[k/2] * 16384);
            }
        } else {
            if (std::isnan(processed_data_r[k])){
                audio_data[k] = 0;
            } else {
                audio_data[k] = static_cast<short int>(processed_data_r[(k-1)/2] * 16384);
            }

        }
    } 
    fwrite(&audio_data[0], sizeof(short int), audio_data.size(), stdout);
}
