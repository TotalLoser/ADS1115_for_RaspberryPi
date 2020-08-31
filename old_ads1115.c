//original format for reading from the ADS1115 ADC with raspberry pi
//use this if the new format doesn't work or is reading from the wrong channel
#include <stdio.h>
#include <unistd.h>

#include "old_ads1115.h"

int file_descriptor;
int ads_address = 0x48; //address of the ADC
uint8_t writeBuf[3]; //for writing data to the ADC
uint8_t readBuf[2]; //for reading data from the ADC
uint16_t raw_data_packet[2]; //a packet for storing data for both channels

const float VPS = 4.096 / 32768.0; //volts per step

int main() {
    //for 1256 ground station project, we only use channel 0 and 1 of the ADC

    // open device on /dev/i2c-1
    // the default on Raspberry Pi B
    if ((file_descriptor = open("/dev/i2c-1", O_RDWR)) < 0) {
        printf("Error: Couldn't open device! %d\n", file_descriptor);
        exit (1);
    }

    // connect to ads1115 as i2c slave
    if (ioctl(file_descriptor, I2C_SLAVE, ads_address) < 0) {
        printf("Error: Couldn't find device on address!\n");
        exit (1);
    }

    for (int i=0; i<=1000; i++) {
        prime_ads1115(0);
        raw_data_packet[0] = readReg(0);

        prime_ads1115(1);
        raw_data_packet[1] = readReg(1);
        // sleep(1); //delay not needed here since it's in the prime function
    }

    close(file_descriptor);

    return 0;
}

// void writeReg(uint8_t deviceAddress, uint8_t pointerRegister, uint8_t val1, uint8_t val2) {

// }

// uint16_t readReg(uint8_t deviceAddress, uint8_t pointerRegister) {
uint16_t readReg(int channel) {
    writeBuf[0] = 0;   // conversion register is 0
    if (write(file_descriptor, writeBuf, 1) != 1) {
        perror("Write register select");
        exit(-1);
    }

    // read 2 bytes
    if (read(file_descriptor, readBuf, 2) != 2) {
        perror("Read conversion");
        exit(-1);
    }

    // convert display results
    uint16_t result = readBuf[0] << 8 | readBuf[1];

    if (result < 0)   result = 0;

    float voltage = result * VPS; // convert to voltage

    printf("Channel %d Values: HEX 0x%02x DEC %d reading %4.3f volts\n",
            channel, result, result, voltage);

    return result;
}

void prime_ads1115(int channel) {
    //the channel argument takes 0 or 1 for AIN0 or AIN1

    //determine initial configuration and next channel config
    uint8_t init_config = BLANK_INIT_CONFIG | (channel << 4);
    int next_channel = !channel;
    uint8_t next_ch_config = BLANK_CHANNEL_CONFIG | (next_channel << 4);

    //place values in writeBuf to be written
    writeBuf[0] = 1; //1 tells the chip that we're writing to the config register
    writeBuf[1] = init_config; 
    writeBuf[2] = CONFIG_LSBS;

    if (write(file_descriptor, writeBuf, 3) != 3) { //write 3 bytes
    perror("Write to register 1");
    exit(-1);
    }

    sleep(1); //I think this waits either one second or until the signal is delivered
    //which ever comes first

    writeBuf[1] = next_ch_config; //writeBuf 0 and 2 stay the same

    if (write(file_descriptor, writeBuf, 3) != 3) { //write 3 bytes
    perror("Write to register 1");
    exit(-1);
    }

    sleep(1);

}


//what I did last time:
//for ch0
    //local init_config = BLANK_INIT_CONFIG = 0b01000001 
    //next channel = 1
    //next_ch_config = 0b01010001
//to do for ch1
    //init_config = 0b01010001
    //next ch = 0
    // next_ch_config = 0b01000001