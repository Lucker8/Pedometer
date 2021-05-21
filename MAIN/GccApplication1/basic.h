#ifndef BASIC_H_
#define BASIC_H_

#define ADC_PIN0 0
#define ADC_PIN1 1
#define ADC_PIN2 2
#define ADC_PIN3 3
#define GR_ACC_DK 9.81584
#define DIG_ACC_ADDR1_R 0x3B
#define DIG_ACC_ADDR1_W 0x3A
#define DIG_ACC_ADDR1 0x1D
#define OUT_X_MSB 0x01
#define F_SETUP 0x09
#define XYZ_DATA_CFG 0x0E
#define CTRL_REG1 0x2A
#define CTRL_REG2 0x2B


//gets raw data from mma4851
void get_data_accel(int *x, int *y, int *z);
//initialises mma4851
void MMA8451_init();
//write a reg through i2c
void i2c_write_reg(char device, char reg, char data);
//read
char i2c_read_reg(char device, char reg);
//read adc
uint16_t adc_read(uint8_t adc_channel);
//setup registers
void register_st(void);
//start time measurement
unsigned long millis();
void start_stopwatch(void);
void stop_stopwatch(void);
void print_name(void);
void read_name(void);




#endif
