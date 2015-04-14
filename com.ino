/*
 * STC1000+, improved firmware and Arduino based firmware uploader for the STC-1000 dual stage thermostat.
 *
 * Copyright 2014 Mats Staffansson
 *
 * This file is part of STC1000+.
 *
 * STC1000+ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * STC1000+ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with STC1000+.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Define STC-1000+ version number (XYY, X=major, YY=minor) and EEROM revision */
#define STC1000P_MAGIC_F		0x192C
#define STC1000P_MAGIC_C		0x26D3
#define STC1000P_VERSION		(100)
#define STC1000P_EEPROM_VERSION		(10)

/* Pin configuration */
#define ICSPCLK 9
#define ICSPDAT 8 
#define VDD1    6
#define VDD2    5
#define VDD3    4
#define nMCLR   3 

/* Delays */
#define TDLY()  delayMicroseconds(1)    /* 1.0us minimum */
#define TCKH()  delayMicroseconds(1)    /* 100ns minimum */
#define TCKL()  delayMicroseconds(1)    /* 100ns minimum */
#define TERAB() delay(5)    		    /* 5ms maximum */
#define TERAR() delay(3)    		    /* 2.5ms maximum */
#define TPINT() delay(5)    		    /* 5ms maximum */
#define TPEXT() delay(2)                /* 1.0ms minimum, 2.1ms maximum*/
#define TDIS()  delayMicroseconds(1)    /* 300ns minimum */
#define TENTS() delayMicroseconds(1)    /* 100ns minimum */
#define TENTH() delay(3)  		        /* 250us minimum */ /* Needs a lot more when powered by arduino */
#define TEXIT() delayMicroseconds(1)    /* 1us minimum */

/* Commands */
#define LOAD_CONFIGURATION                  0x00    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_PROGRAM_MEMORY        0x02    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_DATA_MEMORY           0x03    /* 0, data(14), zero(6), 0 */
#define READ_DATA_FROM_PROGRAM_MEMORY       0x04    /* 0, data(14), 0 */
#define READ_DATA_FROM_DATA_MEMORY          0x05    /* 0, data(14), zero(6), 0 */
#define INCREMENT_ADDRESS                   0x06    /* - */
#define RESET_ADDRESS                       0x16    /* - */
#define BEGIN_INTERNALLY_TIMED_PROGRAMMING  0x08    /* - */
#define BEGIN_EXTERNALLY_TIMED_PROGRAMMING  0x18    /* - */
#define END_EXTERNALLY_TIMED_PROGRAMMING    0x0A    /* - */
#define BULK_ERASE_PROGRAM_MEMORY           0x09    /* Internally Timed */
#define BULK_ERASE_DATA_MEMORY              0x0B    /* Internally Timed */
#define ROW_ERASE_PROGRAM_MEMORY            0x11    /* Internally Timed */


#define	COM_PIN	9	// ICSPCLK

#define COM_HANDSHAKE	0xC5
#define COM_ACK		0x9A

#define TPULSE()          __asm__("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"); 


void setup() {
	pinMode(ICSPCLK, INPUT);
	digitalWrite(ICSPCLK, LOW); // Disable pull-up
	pinMode(ICSPDAT, INPUT);
	digitalWrite(ICSPDAT, LOW); // Disable pull-up
	pinMode(nMCLR, INPUT);
	digitalWrite(nMCLR, LOW); // Disable pull-up

	Serial.begin(115200);

	delay(2);

	Serial.println("STC-1000+ communication sketch.");
	Serial.println("Copyright 2014 Mats Staffansson");
	Serial.println("");
	Serial.println("Send 'd' to check for STC-1000");

}

void tx_bit(unsigned const char data){
	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	if(!data){
		pinMode(COM_PIN, INPUT);
		digitalWrite(COM_PIN, LOW);
		delayMicroseconds(400);
	} else {
		delayMicroseconds(400);
		pinMode(COM_PIN, INPUT);
		digitalWrite(COM_PIN, LOW);
	}
	delayMicroseconds(100);
}

unsigned char rx_bit(){
	unsigned char data;

	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	pinMode(COM_PIN, INPUT);
	digitalWrite(COM_PIN, LOW);
	delayMicroseconds(200);
	data = digitalRead(COM_PIN);
	delayMicroseconds(300);

	return data;
}


void write_byte(unsigned const char data){
	unsigned char i;
	
	for(i=0;i<8;i++){
		tx_bit(((data << i) & 0x80));
	}
	delayMicroseconds(500);
}

unsigned char read_byte(){
	unsigned char i, data;
	
	for(i=0;i<8;i++){
		data <<= 1;
		if(rx_bit()){
			data |= 1;
		}
	}
	delayMicroseconds(500);

	return data;
}


bool write_address(const unsigned char address, const int data){
	write_byte(COM_HANDSHAKE);
	write_byte(address | 0x80);
	write_byte(((unsigned char)(data >> 8)));
	write_byte((unsigned char)data);
	write_byte((address | 0x80) ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data));
	return read_byte() == COM_ACK; 
}

bool read_address(const unsigned char address, int *data){
	unsigned char xorsum;
	unsigned char xorsum2;
	unsigned char ack;
        unsigned int mdata;
	write_byte(COM_HANDSHAKE);
	write_byte(address & 0x7f);
	mdata = read_byte();
	mdata = (mdata << 8) | read_byte();
	Serial.print("DATA -> ");
	Serial.println(mdata);
        *data = mdata;
	xorsum = read_byte();
	Serial.print(xorsum);
	Serial.print(" <- XOR -> ");
        xorsum2 = (address ^ ((unsigned char)(*data >> 8)) ^ ((unsigned char)*data));
	Serial.println(xorsum2);
        ack = read_byte();
	Serial.print(COM_ACK);
	Serial.print(" <- ACK -> ");
	Serial.println(ack);
	return ack == COM_ACK && xorsum == xorsum2; 
}


void loop() {
	int data;
        int count = 0;
        char a, b;

/*
       	pinMode(COM_PIN, OUTPUT);
       	digitalWrite(COM_PIN, HIGH);
        delayMicroseconds(7);
        pinMode(COM_PIN, INPUT);
        digitalWrite(COM_PIN, LOW);
        delayMicroseconds(200);
        a = digitalRead(COM_PIN);
        delayMicroseconds(300);
        b = digitalRead(COM_PIN);
        count++;
        if(a!=1 || b!=0){
          Serial.println(count);
          count=0;
        }
*/

	if (Serial.available() > 0) {
		char command = Serial.read();
		switch (command) {
		case 'u':
			lvp_entry();
			bulk_erase_program_memory();
			upload_hex_file_to_device();
			p_exit();
			break;
		case 'v':
			lvp_entry();
			bulk_erase_data_memory();
			upload_hex_file_to_device();
			write_magic(STC1000P_MAGIC_C);
			write_version(STC1000P_VERSION);
			p_exit();
			break;
		case 's':
			if(read_address(122, &data)){
				Serial.print("s:");
				Serial.print(data);
				Serial.println();
			} else {
				Serial.println("Error");
			}
			break;
		case 't':
			if(read_address(125, &data)){
				Serial.print("T:");
				Serial.print(data);
				Serial.println();
			} else {
				Serial.println("Error");
			}
			break;
		case 'h':
			write_byte(165);
			break;
		case 'g':
			write_byte(90);
			break;
		case 'j':
			write_byte(0);
			break;
		case 'k':
			write_byte(255);
			break;
                case 'q':
                        Serial.println("Sending pulse");
                  	pinMode(COM_PIN, OUTPUT);
                  	digitalWrite(COM_PIN, HIGH);
			delayMicroseconds(7);
          		pinMode(COM_PIN, INPUT);
          		digitalWrite(COM_PIN, LOW);
                        {
                            unsigned char a, b;
                            delayMicroseconds(200);
                            a = digitalRead(COM_PIN);
                            delayMicroseconds(300);
                            b = digitalRead(COM_PIN);
                            Serial.println(a);
                            Serial.println(b);
                        }
                        break;
		default:
			break;
		}
	}

}





unsigned char hex_nibble(unsigned char data) {
	data = toupper(data);
	return (data >= 'A' ? data - 'A' + 10 : data - '0') & 0xf;
}

unsigned char parse_hex() {
	unsigned char data;
	while (Serial.available() < 2)
		;
	data = hex_nibble(Serial.read()) << 4;
	data |= hex_nibble(Serial.read());

	return data;
}

unsigned char handle_hex_file_line(unsigned char bytecount,
		unsigned int address, unsigned char recordtype, unsigned char data[]) {
	static unsigned int device_address = 0;
	unsigned char i;

	if (recordtype == 1) {
		reset_address();
		device_address = 0;
		Serial.println("Programming done");
		return 1;
	} else if (recordtype == 04) {
		if (data[1] == 0) {
			Serial.println("Programming program memory");
			reset_address();
			device_address = 0;
		} else if (data[1] == 1) {
			Serial.println("Programming config memory");
			load_configuration(0);
			device_address = 0;
		}
	} else if (recordtype == 00) {
		if (address >= 0xE000) {
			if (device_address != ((address & 0x1FFF) >> 1)
					|| ((address & 0x1FFF) >> 1) == 0) {
				Serial.println("Resetting address for EEPROM");
				reset_address();
				device_address = 0;
			}

			if(device_address != ((address & 0x1FFF) >> 1)){
				Serial.print("Incrementing address to 0x");
				Serial.println(((address & 0x1FFF) >> 1), HEX);
				while (device_address != ((address & 0x1FFF) >> 1)) {
					increment_address();
					device_address++;
				}
			}

			Serial.print("Programming ");
			Serial.print(bytecount >> 1, DEC);
			Serial.print(" bytes at EEPROM address 0x");
			Serial.println((address & 0x1FFF) >> 1, HEX);

			for (i = 0; i < bytecount; i += 2) {
				unsigned char data_out = data[i];
				unsigned char data_in;
				load_data_for_data_memory(data_out);
				begin_internally_timed_programming();
				data_in = read_data_from_data_memory();
				if (data_in != data_out) {
					Serial.print("Validation failed for EEPROM address 0x");
					Serial.print(device_address, HEX);
					Serial.print(" wrote 0x");
					Serial.print(data_out, HEX);
					Serial.print(" but read back 0x");
					Serial.println(data_in, HEX);
					return 1;
				}
				increment_address();
				device_address++;
			}
		} else {
			if(device_address != (address >> 1)){
				Serial.print("Incrementing address to 0x");
				Serial.println((address >> 1), HEX);
				while (device_address != (address >> 1)) {
					increment_address();
					device_address++;
				}
			}

			Serial.print("Programming ");
			Serial.print(bytecount >> 1, DEC);
			Serial.print(" words at address 0x");
			Serial.println(address >> 1, HEX);

			for (i = 0; i < bytecount; i += 2) {
				unsigned int data_word_out = (((unsigned int) data[i + 1]) << 8)
						| data[i];
				unsigned int data_word_in;
				load_data_for_program_memory(data_word_out);
				begin_internally_timed_programming();
				data_word_in = read_data_from_program_memory();
				if (data_word_in != data_word_out) {
					Serial.print("Validation failed for address 0x");
					Serial.print(device_address, HEX);
					Serial.print(" wrote 0x");
					Serial.print(data_word_out, HEX);
					Serial.print(" but read back 0x");
					Serial.println(data_word_in, HEX);
					return 1;
				}
				increment_address();
				device_address++;
			}
		}
	}
	return 0;
}

void upload_hex_file_to_device() {
	unsigned char done = 0;

	Serial.println("Waiting for hex data...");

	while (!done) {
		unsigned char bytecount;
		unsigned int address;
		unsigned char recordtype;
		unsigned char data[16];
		unsigned char checksum;
		unsigned char i;
		unsigned char rchecksum;

		// Read start of line
		while (1) {
			while (Serial.available() < 1)
				;
			char rx = Serial.read();
			if (rx == ':') {
				break;
			}
		}

		// Read bytecount
		bytecount = parse_hex();
		checksum = bytecount;

		// Read address
		address = ((unsigned int) parse_hex()) << 8;
		address |= parse_hex();
		checksum += ((unsigned char) (address >> 8));
		checksum += ((unsigned char) (address));

		// Read recordtype
		recordtype = parse_hex();
		checksum += recordtype;

		for (i = 0; i < bytecount; i++) {
			data[i] = parse_hex();
			checksum += data[i];
		}

		// Read checksum
		rchecksum = parse_hex();
		checksum += rchecksum;

		if (checksum) {
			Serial.println("Checksum error!");
			break;
		}

		done = handle_hex_file_line(bytecount, address, recordtype, data);
	}
}

#if 0
void upload_hex_from_progmem(PGM_P hexdata) {
	unsigned char done = 0;

	Serial.println("Programming hex data...");

	while (!done) {
		unsigned char bytecount;
		unsigned int address;
		unsigned char recordtype;
		unsigned char data[16];
		unsigned char checksum;
		unsigned char i;

		// Read bytecount
		bytecount = pgm_read_byte(hexdata++);
		checksum = bytecount;

		// Read address
		address = ((unsigned int) pgm_read_byte(hexdata++)) << 8;
		address |= pgm_read_byte(hexdata++);
		checksum += ((unsigned char) (address >> 8));
		checksum += ((unsigned char) (address));

		// Read recordtype
		recordtype = pgm_read_byte(hexdata++);
		checksum += recordtype;

		for (i = 0; i < bytecount; i++) {
			data[i] = pgm_read_byte(hexdata++);
			checksum += data[i];
		}

		// Read checksum
		i = pgm_read_byte(hexdata++);
		checksum += i;

		if (checksum) {
			Serial.println("Checksum error!");
			break;
		}

		done = handle_hex_file_line(bytecount, address, recordtype, data);
	}

}

#endif

/* low level bit transfer */
void write_bit(unsigned char bit) {
	digitalWrite(ICSPCLK, HIGH);
	digitalWrite(ICSPDAT, bit ? HIGH : LOW);
	TCKH();
	digitalWrite(ICSPCLK, LOW);
	TCKL();
	//  digitalWrite(ICSPDAT,LOW); // REM?
}

unsigned char read_bit() {
	unsigned char rv;

	digitalWrite(ICSPCLK, HIGH);
	TCKH();
	rv = (digitalRead(ICSPDAT) == HIGH);
	digitalWrite(ICSPCLK, LOW);
	TCKL();

	return rv;
}

/* Program/verify mode entry and exit */
void hvp_entry() {

	Serial.println("Enter high voltage programming mode");

	pinMode(ICSPCLK, OUTPUT);
	pinMode(VDD1, OUTPUT);
	pinMode(VDD2, OUTPUT);
	pinMode(VDD3, OUTPUT);
	pinMode(ICSPDAT, OUTPUT);
	// Set VPP to VIHH (9v)

	TENTS();
	digitalWrite(VDD1, HIGH);
	digitalWrite(VDD2, HIGH);
	digitalWrite(VDD3, HIGH);
	TENTH();
}

void lvp_entry() {
	unsigned long LVP_magic = 0b01001101010000110100100001010000;
	unsigned char i;

	Serial.println("Enter low voltage programming mode");

	pinMode(nMCLR, OUTPUT);
	pinMode(VDD1, OUTPUT);
	pinMode(VDD2, OUTPUT);
	pinMode(VDD3, OUTPUT);

	digitalWrite(nMCLR, HIGH);
	digitalWrite(VDD1, HIGH);
	digitalWrite(VDD2, HIGH);
	digitalWrite(VDD3, HIGH);
	TENTS();
	digitalWrite(nMCLR, LOW);
	pinMode(ICSPCLK, OUTPUT);
	pinMode(ICSPDAT, OUTPUT);
	TENTH();

	// Send "MCHP" backwards, to unlock LVP mode
	for (i = 0; i < 32; i++) {
		write_bit((LVP_magic >> i) & 1);
	}
	write_bit(0);
}

void p_exit() {

	Serial.println("Leaving programming mode");

	digitalWrite(nMCLR, LOW); // LVP mode
	digitalWrite(ICSPCLK, LOW);
	digitalWrite(ICSPDAT, LOW);
	digitalWrite(VDD1, LOW);
	digitalWrite(VDD2, LOW);
	digitalWrite(VDD3, LOW);
	TEXIT();

	pinMode(nMCLR, INPUT); // LVP mode
	pinMode(ICSPCLK, INPUT);
	pinMode(VDD1, INPUT);
	pinMode(VDD2, INPUT);
	pinMode(VDD3, INPUT);
	pinMode(ICSPDAT, INPUT);
}

/* low level command transfer */
void write_command(unsigned char command) {
	unsigned char i;

	for (i = 0; i < 5; i++) {
		write_bit((command >> i) & 1);
	}
	write_bit(0);
	TDLY();
}

void write_command_with_data(unsigned char command, unsigned int data) {
	unsigned char i;

	write_command(command);

	write_bit(0);
	for (i = 0; i < 14; i++) {
		write_bit((data >> i) & 1);
	}
	write_bit(0);
}

unsigned int read_command(unsigned char command) {
	unsigned char i;
	unsigned int data = 0;

	write_command(command);

	pinMode(ICSPDAT, INPUT);

	read_bit();
	for (i = 0; i < 14; i++) {
		data |= ((unsigned int) (read_bit() & 0x1) << i);
	}
	read_bit();

	pinMode(ICSPDAT, OUTPUT);

	return data;
}

/* high level commands */
void load_configuration(unsigned int data) {
	write_command_with_data(LOAD_CONFIGURATION, data);
}

void load_data_for_program_memory(unsigned int data) {
	write_command_with_data(LOAD_DATA_FOR_PROGRAM_MEMORY, data);
}

void load_data_for_data_memory(unsigned char data) {
	write_command_with_data(LOAD_DATA_FOR_DATA_MEMORY, data);
}

unsigned int read_data_from_program_memory() {
	return read_command(READ_DATA_FROM_PROGRAM_MEMORY);
}

unsigned char read_data_from_data_memory() {
	return (unsigned char) read_command(READ_DATA_FROM_DATA_MEMORY);
}

void increment_address() {
	write_command(INCREMENT_ADDRESS);
}

void reset_address() {
	write_command(RESET_ADDRESS);
}

void begin_internally_timed_programming() {
	write_command(BEGIN_INTERNALLY_TIMED_PROGRAMMING);
	TPINT();
}

void begin_externally_timed_programming() {
	write_command(BEGIN_EXTERNALLY_TIMED_PROGRAMMING);
	TPEXT();
}

void end_externally_timed_programming() {
	write_command(END_EXTERNALLY_TIMED_PROGRAMMING);
	TDIS();
}

void bulk_erase_program_memory() {
	Serial.println("Bulk erasing program memory");
	write_command(BULK_ERASE_PROGRAM_MEMORY);
	TERAB();
}

void bulk_erase_data_memory() {
	Serial.println("Bulk erasing data memory");
	write_command(BULK_ERASE_DATA_MEMORY);
	TERAB();
}

void row_erase_program_memory() {
	write_command(ROW_ERASE_PROGRAM_MEMORY);
	TERAR();
}

/* algorithms */
void bulk_erase_device() {
	Serial.println("Bulk erasing device");
	load_configuration(0);
	bulk_erase_program_memory();
	bulk_erase_data_memory();
}

void get_device_id(unsigned int *magic, unsigned int *version,
		unsigned int *deviceid) {
	lvp_entry();
	load_configuration(0);
	*magic = read_data_from_program_memory();
	increment_address();
	*version = read_data_from_program_memory();
	increment_address();

	increment_address();
	increment_address();
	increment_address();
	increment_address();

	*deviceid = read_data_from_program_memory();

	p_exit();
}

void write_magic(unsigned int data_word_out) {
	Serial.println("Writing magic.");
	load_configuration(0);
	load_data_for_program_memory(data_word_out);
	begin_internally_timed_programming();
}

void write_version(unsigned int data_word_out) {
	Serial.println("Writing version.");
	load_configuration(0);
	increment_address();
	load_data_for_program_memory(data_word_out);
	begin_internally_timed_programming();
}

