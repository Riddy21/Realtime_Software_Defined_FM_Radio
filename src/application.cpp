#include "dy4.h"
#include "application.h"

ApplicationLayer::ApplicationLater (){
    program_id.clear();
    program_type = "Undefined";
    program_service_group = "not0A";
    program_service_name_addresses = -1;
    program_service_names.clear();
}

void ApplicationLayer::get_message(char block_type,
                                   std::vector<bool> &block_data){
    // Process message based on the RDS block type and data within
    // flip data around
    std::reverse(block_data.begin(), block_data.end());
    if (block_type == "A"){
        extract_program_id(block_data);
    } else if (block_type == "B"){
        extract_program_type(std::copy(block_data.begin()+5, block_data.begin() + 9));
        extract_program_service_group(block_data.begin()+11, block_data.end());
        extract_program_service_name_address(block_data.begin(),block_data.begin()+1);
    } else if (block_type == "D"){
        extract_program_service_name(block_data);
    }
}

void list_to_int(std::vector<bool> &block_data,
                 int data){
    data = 0;
    for (unsigned int i=block_data.size(); i>0; i--){
        data *= 2;
        data += (int)(block_data[i]);
    }
}

void extract_program_id(std::vector<bool> &block_data){
    // Extracts the program identification code from block
    // convert block data into character
    int data;
    list_to_int(block_data, data);

    if ((program_id.size() == 0) && (data != 12)){
        return;
    } else if ((program_id.size() == 0) && (data == 12)){
        program_id.push_back(std::hex << data);
    } else if ((program_id.size() > 1) && (program.size() < 4)){
        program_id.push_back(std::hex << data);
    } else {
        return;
    }
}

void extract_program_type(std::vector<bool> &block_data){
    // Extracts the program type from block
    int data;
    list_to_int(block_data, data);
    int data;
    if (data == 1){
        program_type = "News";
    } else if (data == 2){
        program_type = "Information";
    } else if (data == 3){
        program_type = "Sport";
    } else if (data == 4){
        program_type = "Talk";
    } else if (data == 5){
        program_type = "Rock";
    } else if (data == 6){
        program_type = "Classic rock";
    } else if (data == 7){
        program_type = "Adult Hits";
    } else if (data == 8){
        program_type = "Soft Rock";
    } else if (data == 9){
        program_type = "Top 40";
    } else if (data == 10){
        program_type = "Country Music";
    } else if (data == 11){
        program_type = "Oldies Music";
    } else if (data == 12){
        program_type = "Soft Music";
    } else if (data == 13){
        program_type = "Nostalgia";
    } else if (data == 14){
        program_type = "Jazz";
    } else if (data == 15){
        program_type = "Classical";
    } else if (data == 16){
        program_type = "Rhythm & Blues Music";
    } else if (data == 17){
        program_type = "Soft Rhythm & Blues Music";
    }
}



void extract_program_service_name_address (std::vector<bool> &block_data){
    list_to_int(block_data, program_service_name_addresses);
}

void extract_program_service_group (std::vector<bool> &block_data){
    // extract program service name from the block
    int data;
    list_to_int(block_data, data);

    if (data == 0){
        program_service_group = "A0";
    } else {
        program_service_group = "notA0";
    }
}

void extract_program_service_name (std::vector<bool> &block_data){
    // extract the data from offset D
    int data;
    list_to_int(block_data, data);

    if (program_service_group == "A0"){
        program_service_names[program_service_names.size()-1] += (char)(data);
        if (program_service_name_address == 0){
            program_service.push_back("");
        }
}
