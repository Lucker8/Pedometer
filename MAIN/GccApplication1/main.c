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
#include "lcd.h"
#include "usart.h"
#include "basic.h"

#define B1 0xFB		//4				project buttons
#define B2 0xF7		//5
#define B3 0xEF		//6
#define B4 0xDF		//7


uint8_t name_f = 0,w_f=0,start_flag, weight,rst_f=0,height,h_f=0;								//if not start flag, new user
unsigned int name_addr=3,weight_addr=1,height_addr=2;						//addresses
unsigned long timer;
volatile long unsigned ms=0,s=0,m=0;

float array_sum(float *,int);
void steps(float *);
void intro_screen(void);

int main(void)
{
	unsigned int step_flag=0, step=0,count=0,goal_s=0,goal_t=10,k=0, reset_count=0;
	float g_mag=0,acc_x, acc_y, acc_z, v=0, v0=0, x0=0, x=0,acc_mag, avg_mag=0,step_length,step_d=0,avg_mag_a[10]={0};

	DDRD=0x00;		//project
	PORTD=0xFF;
	DDRC=0xF0;
	PORTC=0xF8;
	
	register_st();
	i2c_init();
	LCD_init();
	MMA8451_init();
	
	if(!eeprom_read_byte((uint8_t *)0))	intro_screen();
	LCD_set_cursor(1,1);
	printf("HELLO");
	for(int i=0;i<5;i++){
		LCD_set_cursor(7+i,1);
		printf("%c",eeprom_read_byte((uint8_t *) 3+i));
	}
	_delay_ms(1000);
	for(int i=0;i<15;i++)
	{
		LCD_set_cursor(1+i,1);
		printf(" ");
		_delay_ms(50);
	}

	weight=eeprom_read_byte((uint8_t *) weight_addr);		//weight from eeprom
	height=eeprom_read_byte((uint8_t *) height_addr);
	step_length=(height/100.0)*S_RATIO;			//conversion from cm to m, ratio calculated based on data
	timer=millis();

	while(1)
	{
		LCD_set_cursor(0,0);
		printf("B1-GOAL");
		LCD_set_cursor(13,0);
		printf("W=%dkg",weight);
		LCD_set_cursor(0,1);
		printf("B2-Time Lapse");
		LCD_set_cursor(13,1);
		printf("H=%dcm",height);
		LCD_set_cursor(0,2);
		printf("B3-STOPWATCH");
		LCD_set_cursor(0,3);
		printf("B4-Change info");
		if(PIND==B1)							// distance goal menu
		{
			int goal_f=0;
			LCD_clear();
			while(!goal_f)
			{
				LCD_set_cursor(0,0);
				printf("Set goal %d km ",goal_s);
				LCD_set_cursor(2,3);
				printf("+");
				LCD_set_cursor(6,3);
				printf("-");
				LCD_set_cursor(12,3);
				printf("SET|");
				LCD_set_cursor(16,3);
				printf("BACK");

				if(PIND==B1)
				{
					goal_s++;
					_delay_ms(200);

				}
				if(PIND==B2)
				{
					_delay_ms(200);
					LCD_set_cursor(0,2);
					if(goal_s==0) goal_s=0;
					else
					{
						goal_s--;
					}
				}
				if(PIND==B3)
				{
					goal_f=1;
					LCD_clear();
				}
				if(PIND==B4)
				{
					break;
					_delay_ms(150);
				}
			}

			while(PIND!=B4)				
			{
				if(millis()-timer>100)
				{
					timer=millis();
					steps(&g_mag);
					if (!step_flag)
					{
						if (g_mag>0.5)
						{
							step_flag = 1;
						}
					}
					else
					{
						if (g_mag<0.2)
						{
							step_flag = 0;
							step+=2;
							step_d+=step_length*2;		//adding a step length every step, *2 for the way it works
						}
					}
				}

				if(goal_s<=(step_d/1000))			//to km
				{
					LCD_clear();
					_delay_ms(100);
					while(PIND!=B4)
					{
						LCD_set_cursor(0,0);
						printf("Congratulations!");
						LCD_set_cursor(0,1);
						printf("Completed goal: %dkm",goal_s);
						LCD_set_cursor(0,2);
						printf("In: %d steps",step);
						LCD_set_cursor(0,3);
						printf("Press B4 to go back");
					}
					break;
				}
				LCD_set_cursor(0,0);
				printf("Current Steps: %u",step);
				LCD_set_cursor(0,1);
				printf("Goal : %dkm",goal_s);
				LCD_set_cursor(0,3);
				printf("Distance: %dm",(int) step_d);


			}
			goal_s=0;
			LCD_clear();
			_delay_ms(200);

		}
		if(PIND==B2)
		{
			int goal_f=0,g=0;
			LCD_clear();
			while(!goal_f)
			{
				LCD_set_cursor(0,0);
				printf("Set time %d min ",goal_t);
				LCD_set_cursor(2,3);
				printf("+");
				LCD_set_cursor(6,3);
				printf("-");
				LCD_set_cursor(12,3);
				printf("SET|");
				LCD_set_cursor(16,3);
				printf("BACK");
				if(PIND==B1)
				{
					goal_t++;
					_delay_ms(200);

				}
				if(PIND==B2)
				{
					_delay_ms(200);
					LCD_set_cursor(0,2);
					if(goal_t==0) goal_t=0;
					else goal_t--;

				}
				if(PIND==B3)
				{
					goal_f=1;
					g=goal_t;
					LCD_clear();
				}

				if(PIND==B4)
				{
					break;
					goal_t=10;
					_delay_ms(150);
				}
			}

			start_stopwatch();
			int s_new=s,ss=0;
			while(PIND!=B4)
			{
				if(s_new!=s)
				{
					s_new=s;
					if(ss==0){
						if(goal_t==0)
						{
							x=500;
							LCD_clear();
							LCD_set_cursor(0,0);
							printf("Time is up!");
							LCD_set_cursor(0,1);
							printf("Goal was %d min",g);
							LCD_set_cursor(0,2);
							printf("You traveled: %.1fm",x);
							_delay_ms(10000);
							break;
						}
						goal_t--;
						ss=59;
					}
					else ss--;
				}

				if(millis()-timer>50)
				{
					timer=millis();
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
					reset_count++;
					avg_mag /= 10;			// calculating average magnitude
					avg_mag_a[k]=avg_mag;		//calculating an avg for the last 5 sec
					k++;
					if(k==10) k=0;

					if (reset_count>=4) //resets initial velocity every 2nd second to avoid drift
					{
						v0=0;
						reset_count=0;
					}

					if(avg_mag<1)
					{
						avg_mag=0;
						v=0;
					}		// If avg magnitude is below 1 m/s/s Velocity and Acceleration are reset
					else v = v0+avg_mag*0.5;					// Velocity equation, check time


					x = x0+0.5*(v0+v)*0.5;		// Displacement equation

					v0 = v;		// Setting new initial values
					x0 = x;

					LCD_set_cursor(0,0);
					printf("Avg mag : %.1f ",avg_mag);
					LCD_set_cursor(0,2);
					printf("Velocity: %.1f ",v);
					LCD_set_cursor(0,3);
					printf("Distance: %.1f ",x);

					avg_mag=0;		// reset avg accel magnitude
					count=0;		// reset counter
				}
				LCD_set_cursor(15,0);
				printf("%02d:%02d",goal_t,ss);
				LCD_set_cursor(0,1);
				printf("per 5s  : %.1f ",(array_sum(avg_mag_a,10))/10);

			}
			LCD_clear();
			goal_t=10;
			_delay_ms(200);
		}

		if(PIND==B3){						//STOPWATCH
			LCD_clear();
			while(PIND!=B4)
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
				if(PIND==B1){
					_delay_ms(100);
					start_stopwatch();
					rst_f=0;
					LCD_clear();
				}
				if(PIND==B2)
				{
					_delay_ms(100);
					stop_stopwatch();
					rst_f=1;
					LCD_clear();

				}
				if(PIND==B3 && rst_f){								//reset stopwatch
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
		if(PIND==B4)
		{
			LCD_clear();
			_delay_ms(400);
			while(PIND!=B4)
			{
				LCD_set_cursor(0,0);
				printf("B1-Change data");
				LCD_set_cursor(0,1);
				printf("B2-Reset");
				if(PIND==B1)
				{
					intro_screen();
					_delay_ms(200);
				}
				if(PIND==B2)
				{
					LCD_clear();
					LCD_set_cursor(0,0);
					eeprom_write_byte((uint8_t *)0,0);
					printf("Reset successful!");
					LCD_set_cursor(0,1);
					printf("Please restart");
					LCD_set_cursor(0,2);
					printf("the device");
					_delay_ms(2000);
					break;
				}
			}
			LCD_clear();
			_delay_ms(400);
		}
	}




	return 0;
}

void intro_screen(){
	 int count_l=0,written=0,temp_w=40,temp_n=65,temp_h=160,name_array[5];			//Array for names, letter count, if name was written
	 if(eeprom_read_byte((uint8_t *)0))
	 {
		 LCD_clear();
		 _delay_ms(200);
		 written=1;
	 }
	 while (PIND!=B4)
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
		 if(h_f) printf("Height: %dcm",height);		//if flag set print h
		 else printf("Height: Not Set");
		 LCD_set_cursor(0,3);
		 printf("1Weight");
		 LCD_set_cursor(7,3);
		 printf("2Height");
		 LCD_set_cursor(14,3);
		 printf("3Name");

		 if(PIND==B1){
			 LCD_clear();
			 _delay_ms(100);
			 while(PIND!=B4){
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
				 if(PIND==B1){
					 temp_w++;
					 _delay_ms(200);
				 }
				 if(PIND==B2&&temp_w){
					 temp_w--;
					 _delay_ms(200);
				 }
				 if(PIND==B3){
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

		 if(PIND==B2)
		 {
			 LCD_clear();
			 _delay_ms(100);
			 while(PIND!=B4)
			 {
				 LCD_set_cursor(0,0);
				 printf("Set Your Height:");
				 LCD_set_cursor(0,1);
				 printf(" %02d",temp_h);
				 LCD_set_cursor(2,3);
				 printf("+");
				 LCD_set_cursor(6,3);
				 printf("-");
				 LCD_set_cursor(12,3);
				 printf("SET|");
				 LCD_set_cursor(16,3);
				 printf("BACK");
				 if(PIND==B1)
				 {
					 temp_h++;
					 _delay_ms(200);
				 }
				 if(PIND==B2&&temp_h)
				 {
					 temp_h--;
					 _delay_ms(200);
				 }
				 if(PIND==B3)
				 {
					 eeprom_write_byte((uint8_t *) 2,temp_h);					//EEPROM WRITE
					 LCD_clear();
					 LCD_set_cursor(0,0);
					 printf("EEPROM WRITE");
					 height=temp_h;
					 h_f=1;
					 _delay_ms(300);
					 break;
				 }
			 }
			 temp_h=160;
			 LCD_clear();
			 _delay_ms(200);
		 }


		 if(PIND==B3)
		 {
			 LCD_clear();
			 _delay_ms(100);

			 while(PIND!=B4){
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

				  if(PIND==B1){
					  if(temp_n==90) temp_n=65;
					  else temp_n++;
					  _delay_ms(100);
				  }
				  if(PIND==B2&&temp_n){
					  if(temp_n==65) temp_n=90;
					  else temp_n--;
					  _delay_ms(100);
				  }

				  if(PIND==B3){
					  if(!count_l) name_array[count_l]=temp_n;		//Capital letter for 1.
					  else  name_array[count_l]=temp_n+32;			//lowercase for rest
					  LCD_set_cursor(0,2);
					  printf("%d",name_array[count_l]);
					  count_l++;
					  if(count_l==5) break;

					  _delay_ms(250);

				  }
				}
			 eeprom_write_byte((uint8_t *)0,0);
			 name_f=1;
			 written=0;
			 count_l=0;
			 temp_n=65;
			 LCD_clear();
			 _delay_ms(200);
		 }

	 }
	 LCD_clear();
	 if(!h_f){
		LCD_set_cursor(0,2);
		printf("NO HEIGHT SELECTED!");		//height and name are very important
		_delay_ms(1000);
		LCD_clear();
	 }
	 if(!name_f)
	 {
		 LCD_set_cursor(0,2);
		 printf("NO NAME SELECTED!");
		 _delay_ms(1000);
		 LCD_clear();
	 }
	 eeprom_write_byte((uint8_t *)0,1);
	 LCD_set_cursor(0,1);
	 printf("Data set, thx");
	 _delay_ms(1000);
	 LCD_clear();

}

void steps(float *g_mag)
{
	int x0, y0, z0;
	float acd_x, acd_y, acd_z;

	get_data_accel(&x0, &y0, &z0);
	acd_x = x0/1024.0;
	acd_y = y0/1024.0;
	acd_z = z0/1024.0;
	*g_mag = (sqrt(acd_x*acd_x+acd_y*acd_y+acd_z*acd_z))-1;
	if(*g_mag<0) *g_mag *= (-1);

}

float array_sum(float arr[],int n)
{
	int sum=0;
	for(int i=0;i<n;i++)
	{
		sum+=arr[i];
	}

	return sum;
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
