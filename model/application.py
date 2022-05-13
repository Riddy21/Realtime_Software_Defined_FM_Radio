
class ApplicationLayer(object):
    def __init__(self):
        self.program_id = []
        self.program_type = "Undefined"
        self.program_service_group = "not0A"
        self.program_service_name_addresses = -1
        self.program_service_names = [""]

    def get_message(self, block_type, block_data):
        """Process message based on the RDS block type and data within"""
        # flip data around
        block_data = block_data[::-1]
        if block_type == "A":
            self.extract_program_id(block_data)
        elif block_type == "B":
            self.extract_program_type(block_data[5:10])
            self.extract_program_service_group(block_data[11:])
            self.extract_program_service_name_address(block_data[:2])
        elif block_type == "D":
            self.extract_program_service_name(block_data)

    def print_attr(self):
        print("program_id: ", self.program_id)
        print("program type: ", self.program_type)
        print("program service group: ", self.program_service_group)
        print("program service name addresses: ", self.program_service_name_addresses)
        print("program service names: ", self.program_service_names)
        
    def list_to_int(self, block_data):
        data = 0
        for item in block_data[::-1]:
            data *= 2
            data += int(item)
        return data

    def extract_program_id(self, block_data):
        """Extracts the program identification code from block"""
        # convert block data into character
        data = self.list_to_int(block_data)

        # Equivalent of C
        if len(self.program_id) == 0 and data != 12:
            return
        elif len(self.program_id) == 0 and data == 12:
            self.program_id.append(hex(data))
        elif len(self.program_id) > 1 and len(self.program_id) < 4:
            self.program_id.append(hex(data))
        else:
            return
    
    def extract_program_type(self, block_data):
        """Extracts the program type from block"""
        data = self.list_to_int(block_data)

        if data == 1:
            self.program_type = "News"
        elif data == 2:
            self.program_type = "Information"
        elif data == 3:
            self.program_type = "Sport"
        elif data == 4:
            self.program_type = "Talk"
        elif data == 5:
            self.program_type = "Rock"
        elif data == 6:
            self.program_type = "Classic rock"
        elif data == 7:
            self.program_type = "Adult Hits"
        elif data == 8:
            self.program_type = "Soft Rock"
        elif data == 9:
            self.program_type = "Top 40"
        elif data == 10:
            self.program_type = "Country Music"
        elif data == 11:
            self.program_type = "Oldies Music"
        elif data == 12:
            self.program_type = "Soft Music"
        elif data == 13:
            self.program_type = "Nostalgia"
        elif data == 14:
            self.program_type = "Jazz"
        elif data == 15:
            self.program_type = "Classical"
        elif data == 16:
            self.program_type = "Rhythm & Blues Music"
        elif data == 17:
            self.program_type = "Soft Rhythm & Blues Music"

    def extract_program_service_name_address(self, block_data):
        self.program_service_name_addresses = self.list_to_int(block_data)
    
    def extract_program_service_group(self, block_data):
        """extract program service name from the block"""
        data = self.list_to_int(block_data)

        if data == 0:
            self.program_service_group = "A0"
        else:
            self.program_service_group = "notA0"
    
    def extract_program_service_name(self, block_data):
        """extract the data from offset D"""
        data = self.list_to_int(block_data)
        if self.program_service_group == "A0":
            self.program_service_names[-1] += chr(data)
            if self.program_service_name_addresses == 0:
                self.program_service_names.append("")



