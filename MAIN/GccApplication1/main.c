/*
 * GccApplication1.c
 *
 * Created: 1/10/2021 5:19:16 PM
 * Author : peter
 */ 
#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "i2cmaster.h"
#include "ds1621.h"
#include "lcd.h"
#include "usart.h"
#include "basic.h"


#define B1 0x3E		//4
#define B2 0x3D		//5
#define B3 0x3B		//6
#define B4 0x37		//7


uint8_t name_f = 0,w_f=0,start_flag, weight,name,rst_f=0;;									//if not start flag, new user
unsigned int name_addr=3,weight_addr=1,height_addr=2,timer;							//addresses
volatile long unsigned ms=0,s=0,m=0;


void steps(float *);
void intro_screen(void);

int main(void)
{	
	int name_array[5], flag=0, step=0,count=0;
	float g_mag=0,acc_x, acc_y, acc_z, a=0, v=0, v0=0, a0=0, x0=0, x=0,acc_mag, avg_mag=0;

	
	DDRD=0xFF;
	PORTD=0x00;
	DDRC=0xF0;
	PORTC=0x3F;
	register_st();
	i2c_init();
	LCD_init();
	MMA8451_init();
	
	//uart_init();
	//io_redirect();
	//intro_screen();
	
	weight=eeprom_read_byte((uint8_t *) weight_addr);		//weight from eeprom
	name= eeprom_read_byte((uint8_t *) 3);
	timer=millis();
	
	while(1)
	{
		/*LCD_set_cursor(0,0);
		printf("Hello ");
		for(int i=0;i<5;i++){
			printf("%c",name_array[i]);
		}
		LCD_set_cursor(0,1);*/
		LCD_set_cursor(0,0);
		printf("B1-GOAL");
		LCD_set_cursor(13,0);
		printf("W=%dkg",weight);
		LCD_set_cursor(0,1);
		printf("B2-Velocity");
		LCD_set_cursor(0,3);
		printf("B4-STOPWATCH");
		if(PINC==B1)							//step counter menu, maybe distance goal
		{
			LCD_clear();
			while(PINC!=B4)
			{	
				if(millis()-timer>100)
				{	
					timer=millis();
					steps(&g_mag);
					if (!flag)
					{
						if (g_mag>0.4){
							flag = 1;
						}
					}
					else
					{
						if (g_mag<0.3)
						{
							flag = 0;
							step++;
						}
					}
				}
				LCD_set_cursor(0,0);
				printf("Current Steps: %d\n",step);
				
				
			}
			LCD_clear();
			_delay_ms(200);
		}
		if(PINC==B2)
		{
			LCD_clear();
			while(PINC!=B4)
			{	
				
				if(millis()-timer>500)
				{
					acc_x = (3.1182*((adc_read(ADC_PIN0)*5.0)/1024)-5.1101)*GR_ACC_DK;
					acc_y = (3.0506*((adc_read(ADC_PIN1)*5.0)/1024)-4.9732)*GR_ACC_DK; //Equations to convert voltage levels to acceleration in g then to acceleration in m/s^2
					acc_z = (2.9112*((adc_read(ADC_PIN2)*5.0)/1024)-4.8236)*GR_ACC_DK;
					acc_mag = (sqrt(acc_x*acc_x+acc_y*acc_y+acc_z*acc_z))-GR_ACC_DK; // accel magnitude of all three axis minus gravitational acceleration
					if(acc_mag<0) abs(acc_mag);				// if accel magnitude is negative, it is now positive
					avg_mag+= acc_mag;						// avg magnitude adding up to be divided and reset later
					count++;
				}
				
				
				if(count==10)
				{					
					avg_mag /= 10;			// calculating average magnitude					
					if(avg_mag<1)
					{
						v=0;
						a=0;
					}		// If avg magnitude is below 1 m/s/s Velocity and Acceleration are reset
					else
					{
						a = avg_mag-a0;					// change in acceleration
						v = v0+a*0.5;					// Velocity equation, check time
					}
					
					x = x0+0.5*(v0+v)*0.5;		// Displacement equation
		
					a0 = a;
					v0 = v;		// Setting new initial values
					x0 = x;
					
					avg_mag=0;		// reset avg accel magnitude
					count=0;		// reset counter					
				}
				LCD_set_cursor(0,0);
				printf("a= %.1f v= %.1f x= %.1f",a,v,x);  // for testing purposes
			}
			LCD_clear();
			_delay_ms(200);
		}
		
		if(PINC==B4){						//STOPWATCH
			LCD_clear();
			_delay_ms(500);
			while(PINC!=B4)
			{
				LCD_set_cursor(0,0);
				printf("STOPWATCH-B4=BACK");
				LCD_set_cursor(6,1);
				printf("%02ld:%02ld:%02ld",m,s,ms/10);
				LCD_set_cursor(0,3);
				if(!rst_f && !ms && !s && !m) printf("START");
				else if(rst_f)printf("RESUME");
				if(ms){
					LCD_set_cursor(7,3);
					printf("STOP");
				}
				if(rst_f){
					LCD_set_cursor(13,3);
					printf("RESET");
				}
				if(PINC==B1){
					_delay_ms(100);
					start_stopwatch();
					rst_f=0;
					LCD_clear();
				}
				if(PINC==B2)
				{
					_delay_ms(100);
					stop_stopwatch();
					rst_f=1;
					LCD_clear();
					
				}
				if(PINC==B3 && rst_f){								//reset stopwatch
					_delay_ms(100);
					s=0;
					ms=0;
					m=0;
					rst_f=0;
					LCD_clear();
					
				}
			}
			LCD_clear();
			_delay_ms(200);
		}
	}
	
   
		
	
	return 0;
}

void intro_screen(){
	 int count_l=0,written=0,temp_w=40,temp_n=65,name_array[5];			//Array for names, letter count, if name was written
	 
	 while (PINC!=B4)
	 {	
		 if(!written && name_f){					//if not written and flag set, write to eeprom
			 LCD_clear();
			 LCD_set_cursor(0,0);
			 for(int i=0;i<5;i++){
				 printf("EEPROM WRITE");
				 eeprom_write_byte((uint8_t *)i+3,(uint8_t)name_array[i]);
				 _delay_ms(300);
				 LCD_clear();
			 }
			 written=1;
			 LCD_clear();
		 }
		 LCD_set_cursor(0,0);
		 if(name_f){
			 printf("Name:");
				 for(int i=0;i<5;i++){
					 printf("%c",name_array[i]);
				 }	 
			 
		 }
		 else printf("Name: NOT SET");
		 LCD_set_cursor(0,1);
		 if(w_f) printf("Weight: %dkg",weight);			//if flag set print w
		 else printf("Weight: Not Set"); 
		 LCD_set_cursor(0,2);
		 printf("SET:");
		 LCD_set_cursor(0,3);
		 printf("Weight|");
		 LCD_set_cursor(7,3);
		 printf("Height|");
		 LCD_set_cursor(14,3);
		 printf("Name");
		 
		 if(PINC==B1){
			 LCD_clear();
			 _delay_ms(100);
			 while(PINC!=B4){
				 LCD_set_cursor(0,0);
				 printf("Set Your Weight:");
				 LCD_set_cursor(0,1);
				 printf(" %02d",temp_w);
				 LCD_set_cursor(2,3);
				 printf("+");
				 LCD_set_cursor(6,3);
				 printf("-");
				 LCD_set_cursor(12,3);
				 printf("SET|");
				 LCD_set_cursor(16,3);
				 printf("BACK");
				 if(PINC==B1){
					 temp_w++;
					 _delay_ms(200);
				 }
				 if(PINC==B2&&temp_w){
					 temp_w--;
					 _delay_ms(200);
				 }
				 if(PINC==B3){
					 eeprom_write_byte((uint8_t *) 1,temp_w);					//EEPROM WRITE
					 LCD_clear();
					 LCD_set_cursor(0,0);
					 printf("EEPROM WRITE");
					 weight=temp_w;
					 w_f=1;
					 _delay_ms(300);
					 break;
				 }
			 }
			 temp_w=40;
			 LCD_clear();
			 _delay_ms(200);
		 }
		 
		 if(PINC==B3){
			 LCD_clear();
			 _delay_ms(100);
			 
			 while(PINC!=B4){
				  LCD_set_cursor(0,0);
				  printf("Set Your Name:");
				  LCD_set_cursor(0,1);
				  printf("%d. Letter: %c",count_l+1,temp_n);
				  LCD_set_cursor(2,3);
				  printf("+");
				  LCD_set_cursor(6,3);
				  printf("-");
				  LCD_set_cursor(12,3);
				  printf("SET|");
				  LCD_set_cursor(16,3);
				  printf("BACK");
				  
				  if(PINC==B1){
					  if(temp_n==90) temp_n=65;
					  else temp_n++;
					  _delay_ms(200);
				  }
				  if(PINC==B2&&temp_n){
					  if(temp_n==65) temp_n=90;
					  else temp_n--;
					  _delay_ms(200);
				  }
				  
				  if(PINC==B3){
					  if(!count_l) name_array[count_l]=temp_n;		//Capital letter for 1. 
					  else  name_array[count_l]=temp_n+32;			//lowercase for rest
					  LCD_set_cursor(0,2);
					  printf("%d",name_array[count_l]);
					  count_l++; 
					  if(count_l==5) break;
					  _delay_ms(250);
					
				  }
				}
			
			 name_f=1;
			 count_l=0;
			 temp_n=65;
			 LCD_clear();
			 _delay_ms(200); 
		 }
		 
	 } 
	 LCD_clear();
	 if(!w_f){
		LCD_set_cursor(0,2);
		printf("SELECT YOUR WEIGHT!");
		_delay_ms(1000);
		LCD_clear();
		//intro_screen();						What to do here>>>
	 }
	 //eeprom_write_byte((uint8_t *)0,1);
	
}

void steps(float *g_mag)
{
	int x0, y0, z0;
	float acd_x, acd_y, acd_z,g_max=0, g_min=10;
	
	get_data_accel(&x0, &y0, &z0);
	acd_x = x0/1024.0;
	acd_y = y0/1024.0;
	acd_z = z0/1024.0;
	*g_mag = (sqrt(acd_x*acd_x+acd_y*acd_y+acd_z*acd_z))-1;
	if(*g_mag<0) *g_mag *= (-1);
	if(*g_mag<g_min) g_min=*g_mag;
	if(*g_mag>g_max) g_max=*g_mag;
	

	printf("mag = %.2f\nmin = %.2f\nmax = %.2f\n", *g_mag, g_min, g_max);
	
}


ISR (TIMER0_COMPA_vect) // timer0 overflow interrupt
{
	//event to be executed every ms here
	if (ms<1000) ms++;
	else{
		ms=0;
		if(s<60) s++;
		else{
			s=0;
			m++;
		}
	}

}