#ifndef DY4_APPLICATION_H
#define DY4_APPLICATION_H

#include "sampling.h"
#include "channel.h"
#include <vector>
#include <cmath>
#include "application.h"

class ApplicationLayer{
    public:
    	// parameters related to the application part
    	char program_type;
    	char program_service_group;
    	int program_service_name_addresses;

    	// vector related to the application part
        std::vector<int> program_id;
        std::vector<char> program_service_names;

        ApplicationLayer();
        void get_message(char,
                         std::vector<bool> &);
        
        void list_to_int(std::vector<bool> &,
                         int);
        
        void extract_program_id(std::vector<bool> &);

        void extract_program_type(std::vector<bool> &);

        void extract_program_service_name_address (std::vector<bool> &);

        void extract_program_service_group (std::vector<bool> &);

        void extract_program_service_name (std::vector<bool> &);
};

#endif