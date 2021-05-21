/*
 * GccApplication1.c
 *
 * Created: 1/10/2021 5:19:16 PM
 * Author : peter
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif


#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "basic.h"
#include "i2cmaster.h"

volatile unsigned long mss;
void register_st(void)
{
  cli();
  ADMUX=(1<<REFS0);  //sets Vref to Vcc
  ADCSRA=(1<<ADPS2) | (1<<ADPS0) | (1<<ADEN); //enable adc
  OCR1A= 0xF9; //set the compare to 249 for 1 ms // OCRn = [ (clock_speed/prescaler) * time_in_seconds] - 1
  TIMSK1 |= (1 << OCIE1A); //Set the ISR COMPA vect

  TCCR0A|=(1<<WGM01); //Set the mode to CTC
  OCR0A= 0xF9; //set the compare to 249 for 1 ms // OCRn = [ (clock_speed/prescaler) * time_in_seconds] - 1
  TIMSK0 |= (1 << OCIE0A); //Set the ISR COMPA vect
  sei(); //enable interrupts
  TCCR1B|=(1<<WGM12)|(1<<CS10)|(1<<CS11); //Set the mode to CTC & set the prescaler to 64

}

uint16_t adc_read(uint8_t adc_channel)
{
	ADMUX &= 0xf0; // clears previously selected channel
	ADMUX |= adc_channel; // set the desired channel
	//start a conversion
	ADCSRA |= (1<<ADSC);
	// now wait for the conversion to complete
	while ( (ADCSRA & (1<<ADSC)) );
	ADMUX &= 0xf0; // clear any previously used channel, but keep internal reference

	// now we have the result, so we return it to the calling function as a 16 bit unsigned int
	return ADC;
}

ISR (TIMER1_COMPA_vect) // timer0 overflow interrupt
{
	//event to be executed every ms here
	mss++;

}



unsigned long millis()
{
	unsigned long m;
	m=mss;
	return m;

}

void MMA8451_init()
{
	i2c_write_reg(DIG_ACC_ADDR1, CTRL_REG1, 0x02);
	i2c_write_reg(DIG_ACC_ADDR1, CTRL_REG2, 0x02);
	i2c_write_reg(DIG_ACC_ADDR1, XYZ_DATA_CFG, 0x02);
	i2c_write_reg(DIG_ACC_ADDR1, F_SETUP, 0x00);
	i2c_write_reg(DIG_ACC_ADDR1, CTRL_REG1, 0x1B);
}

void get_data_accel(int *x, int *y, int *z)
{

	i2c_start(DIG_ACC_ADDR1_W);
	i2c_write(OUT_X_MSB);              //starts at msb x and autoincrements by 2
	i2c_start(DIG_ACC_ADDR1_R);

	*x = (i2c_readAck()<<8);   //two steps to have negative numbers
	*x = *x>>2;
	*y = (i2c_readAck()<<8);
	*y = *y>>2;
	*z = (i2c_readNak()<<8);
	*z = *z>>2;
	i2c_stop();
}


void i2c_write_reg(char device, char reg, char data)
{
	i2c_start(device + I2C_WRITE);
	i2c_write(reg);
	i2c_write(data);
	i2c_stop();
}

char i2c_read_reg(char device, char reg)
{
	i2c_start(device + I2C_WRITE);
	i2c_write(reg);
	i2c_start(device + I2C_READ);
	char data = i2c_readNak();
	i2c_stop();
	return data;
}

void start_stopwatch(void)
{
  TCCR0B|=(1<< CS01)|(1<<CS00); // set the prescaler to 64 and start stopwatch
}

void stop_stopwatch(void)
{
	TCCR0B&=~((1<< CS01)|(1<<CS00));		 //stop the timer
}