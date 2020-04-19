/**
 * FROM 0x0000
 * TO   0x8000  (DEC 32768) (128 PAGES)
 */

const byte ASCIIHexList['F' - '0' + 1] = {
    0, /* '0' */
    1, /* '1' */
    2, /* '2' */
    3, /* '3' */
    4, /* '4' */
    5, /* '5' */
    6, /* '6' */
    7, /* '7' */
    8, /* '8' */
    9, /* '9' */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0xA, /* 'A' */
    0xB, /* 'B' */
    0xC, /* 'C' */
    0xD, /* 'D' */
    0xE, /* 'E' */
    0xF, /* 'F' */
};

#define ASCII2NIBBLE(Chr) (ASCIIHexList[Chr - '0'])

#define SHIFT_DATA     2
#define SHIFT_CLK     10
#define SHIFT_LATCH   11

#define WRITE_ENABLE  13  // WE

// HEX 0 1 2 3 4 5 6 7 8 9 A B C D E F
#define C_HELP      'H'     // Print on serial all commands with the description
#define C_START     '<'     // Start writing from address 0
#define C_END       '>'     // End writing
#define C_PRINT     'P'     // Print the page at address: P FFFF
#define C_CLEAR     'R'     // Erase all memory to 0x00

//                             MSB...................LSB
const static int data_pins[] = {12, 9, 8, 7, 6, 3, 4, 5};
#define DATA_LEN      (sizeof(data_pins) / sizeof(data_pins[0]))


typedef enum {
    MODE_WRITE,
    MODE_READ,
    MODE_NONE
} rw_mode_t;


typedef enum {
    S_WAIT,
    S_WRITE_IN_MEM,
    S_PRINT_PAGE
} state_t;


static state_t state = S_WAIT;              // Current state
static rw_mode_t current_mode = MODE_NONE;  // Current operation mode
static char HEX_buf[4];                     // Buffer for reading the hex from serial
static byte HEX_count = 0;                  // Counter of read HEX characters
static int write_counter = 0;               // Counter of bytes written in EEPROM
static int b = 0;                           // Last data read from serial
static byte last_byte = 0;                  // Last byte HEX formatted byte read from serial
static int ser_addres_count = 0;            // Counter used to read the a 4 HEX digit value
static int last_address = 0;                // Address read from serial


void setup() {
    // INIT SERIAL
    Serial.begin(115200);

    while (!Serial) ;

    // Put your setup code here, to run once:
    pinMode(SHIFT_DATA, OUTPUT);
    pinMode(SHIFT_CLK, OUTPUT);
    pinMode(SHIFT_LATCH, OUTPUT);
    digitalWrite(WRITE_ENABLE, HIGH);
    pinMode(WRITE_ENABLE, OUTPUT);
}


void loop() {
    // Read data from serial
    if (Serial.available() > 0) {
        // Read the incoming byte:
        b = Serial.read();

        // Search for command
        switch (b) {

        case C_START:   // Initialize the writing on memory status
            state = S_WRITE_IN_MEM;
            HEX_count = 0;
            write_counter = 0;
            return;

        case C_END:     // Finish the writing in memory status
            state = S_WAIT;
            HEX_count = 0;

            // Print the read data
            // char buf[10];
            // Serial.print("< ");

            // for (int i = 0; i < write_counter; ++i) {
            //     sprintf(buf, "%02X ", read_EEPROM(i));
            //     Serial.print(buf);
            // }
            return;

        case C_CLEAR:   // Erase memory to zero
            write_all(0x00);
            return;

        case C_PRINT:   // Initialize the status that print on serial the content of the selected page
            state = S_PRINT_PAGE;
            ser_addres_count = 0;
            last_address = 0;
            return;

        case C_HELP:    // Print command with description
            Serial.println("---------------------------");
            Serial.println("All commands accept only uppercase chars");
            Serial.println("All characters that are not hex or command will be ignored");
            Serial.println("---------------------------");
            Serial.println("'H' Print this message");
            Serial.println("'<' Start the writing. Ex: < AA AA AA ");
            Serial.println("'>' End the writing. Ex: AA AA AA >");
            Serial.println("'P' Print the selected memory page. Es: P0000 will print the page zero, P007F will print the page 127");
            Serial.println("'R' Will overwrite all the memory with 0x00");
            Serial.println("---------------------------");

        case '\n':
            return;
        }


        // Execute the current state
        switch (state) {
        case S_WAIT:                // Waiting for the next command
            break;

        case S_WRITE_IN_MEM:        // Write bytes on the serial into EEPROM

            if (read_HEX()) {
                write_EEPROM(write_counter, last_byte);
                ++write_counter;
            }

            break;

        case S_PRINT_PAGE:          // Print on serial a specified page 4 hex digits: FFFF
            if (read_address()) {
                Serial.println("---------------------------------------------------------");
                read_page(last_address);
                Serial.println("---------------------------------------------------------");
                state = S_WAIT;
            }

            break;

        default:
            Serial.println("Error 2");
            break;
        }
    }
}


/**
 *  Parse the characters from serial into byte.
 *  Return true if found a valid 2 digit HEX number in serial, false otherwaise
 *  The result is stored in last_byte
 */
static bool read_HEX() {
    if ((b > '/' && b < ':') || (b > '@' && b < 'G')) {
        switch (HEX_count) {
        case 0:
            HEX_buf[0] = b;
            ++HEX_count;
            break;

        case 1:
            HEX_buf[1] = b;

            last_byte = hex_to_byte(HEX_buf);
            HEX_count = 0;
            return true;
        }
    } else {
        HEX_count = 0;

        Serial.print("Ignoring: ");
        Serial.println(b, DEC);
    }

    return false;
}


static bool read_address() {
    if (read_HEX()) {
        switch (ser_addres_count) {
        case 0:
            last_address = (last_byte << 8) & 0x0000FF00;
            ++ser_addres_count;
            break;

        case 1:
            last_address |= last_byte;
            ser_addres_count = 0;
            return true;

        default:
            Serial.println("Error read address out of bound");
            break;
        }
    }

    return false;
}


static void set_address(const int address, const bool out_enable) {
    shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (out_enable ? 0x00 : 0x80));
    shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

    digitalWrite(SHIFT_LATCH, LOW);
    digitalWrite(SHIFT_LATCH, HIGH);
    digitalWrite(SHIFT_LATCH, LOW);
}


static void set_mode(const rw_mode_t mode) {
    if (current_mode == mode) {
        return;
    }

    for (int i = 0; i < DATA_LEN; ++i) {
        int pin = data_pins[i];
        digitalWrite(pin, LOW);

        if (mode == MODE_WRITE) {
            pinMode(pin, OUTPUT);
        } else {
            pinMode(pin, INPUT);
        }
    }

    current_mode = mode;
}


static byte read_EEPROM(const int address) {
    byte res = 0;

    set_mode(MODE_READ);

    set_address(address, true);

    for (int i = 0; i < DATA_LEN; ++i) {
        res = (res << 1) + digitalRead(data_pins[i]);
    }

    return res;
}


static const void write_EEPROM(const int address, byte data) {

    set_mode(MODE_WRITE);

    set_address(address, false);

    for (int i = DATA_LEN - 1; i >= 0; --i) {
        digitalWrite(data_pins[i], data & 1);
        data = data >> 1;
    }

    delayMicroseconds(1);
    digitalWrite(WRITE_ENABLE, LOW);
    delayMicroseconds(1);
    digitalWrite(WRITE_ENABLE, HIGH);
    delay(10);
}


static const void disable_SDP() {
    // LOAD DATA AA TO ADDRESS 5555
    write_EEPROM(0x5555, 0xAA);
    // LOAD DATA 55 TO ADDRESS 2AAA
    write_EEPROM(0x2AAA, 0x55);
    // LOAD DATA 80 TO ADDRESS 5555
    write_EEPROM(0x5555, 0x80);
    // LOAD DATA AA TO ADDRESS 5555
    write_EEPROM(0x5555, 0xAA);
    // LOAD DATA 55 TO ADDRESS 2AAA
    write_EEPROM(0x2AAA, 0x55);
    // LOAD DATA 20 TO ADDRESS 5555
    write_EEPROM(0x5555, 0x20);

    // LOAD DATA XX TO ANY ADDRESS
    //write_EEPROM(0x0000, 0x69);
    //write_EEPROM(0x0001, 0x42);
}


static void read_page(const int page) {
    char buff[200];
    int offset = page * 256;

    for (int i = 0; i < 256; i += 16) {
        byte data_buf[16];

        for (int j = 0; j < 16; ++j) {
            data_buf[j] = read_EEPROM(i + j + offset);
        }

        sprintf(buff,
                "%04X   %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X",
                i + offset,
                data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5], data_buf[6],
                data_buf[7],
                data_buf[8], data_buf[9], data_buf[10], data_buf[11], data_buf[12], data_buf[13], data_buf[14],
                data_buf[15]);

        Serial.println(buff);
    }
}


static void read_all() {
    for (int i = 0; i < 128; ++i) {
        read_page(i);
        Serial.println();
    }
}


static void write_page(const int page, const byte data) {
    int offset = page * 256;

    for (int i = offset; i < 256 + offset; ++i) {
        write_EEPROM(i, data);
    }
}


static void write_all(const byte data) {
    for (int i = 0; i < 128; ++i) {
        // Serial.print("Writing page ");
        // Serial.println(i);
        write_page(i, data);
    }
}


byte hex_to_byte(const char bytes[2]) {
    return ((ASCII2NIBBLE((bytes[0])) << 4) | ASCII2NIBBLE((bytes[1])));
}


int hex_to_int16(const char bytes[4]) {
    return ((ASCII2NIBBLE((bytes[0])) << 12) | (ASCII2NIBBLE((bytes[1])) << 8) | (ASCII2NIBBLE((
                bytes[2])) << 4) | ASCII2NIBBLE((bytes[3])));
}
