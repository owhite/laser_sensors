#define SERIAL_SPEED   115200

#define I2C_SPEED 400000

#define JOY_SLAVE_ADDRESS          0x0A
#define FLOW_SLAVE_ADDRESS         0x0B
#define LOADCELL_SLAVE_ADDRESS     0x0C
#define RELAYDRIVER_SLAVE_ADDRESS  0x0D
#define TEST_SLAVE_ADDRESS         0x0E
#define OLED_ADDRESS               0x3C

#define PACKET_LEN       6
#define PACKET_START     '{'
#define PACKET_STOP      '}'

uint8_t outputBuf[PACKET_LEN]; 
uint8_t inputBuf[20]; 
uint8_t inputSize = 0;

#define LOADCELL_BAD_BIT 1
#define LOADCELL_STOP    1

uint8_t  masterCommand = 0;
uint32_t lastReceive   = 0;

#define CMD_DONOTHING  0x00
#define CMD_REPORT     0x0A
#define CMD_ACTIVATE   0x0B
#define CMD_DEACTIVATE 0x0C
#define CMD_ALARM      0x0D
#define CMD_SILENCE    0x0E
#define CMD_SETPIN     0x0F
#define CMD_SETPINS    0x10
#define CMD_SHUTDOWN   0x11

