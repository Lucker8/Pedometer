/*
 * GccApplication1.c
 *
 * Created: 1/10/2021 5:19:16 PM
 * Author : peter
 */ 
#define F_CPU 16000000UL

#include <avr/eeprom.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "i2cmaster.h"
#include "ds1621.h"
#include "lcd.h"
#include "usart.h"
#include <avr/interrupt.h>

#define B1 0x3E		//4
#define B2 0x3D		//5
#define B3 0x3B		//6
#define B4 0x37		//7


uint8_t name_f = 0,w_f=0,start_flag, weight,name;									//if not start flag, new user
unsigned int name_addr=3,weight_addr=1,height_addr=2,temp_w=40,temp_n=65;							//addresses
void intro_screen(void);
void print_name(void);
void read_name(void);
int main(void)
{
	//uart_init();
	//io_redirect();
	DDRD=0xFF;
	PORTD=0x00;
	DDRC=0xF0;
	PORTC=0x3F;
	i2c_init();
	LCD_init();
	intro_screen();
	int name_array[5];
	
	weight=eeprom_read_byte((uint8_t *) weight_addr);
	name= eeprom_read_byte((uint8_t *) 3);
	
	while(1){
		LCD_set_cursor(0,0);
		printf("Hello ");
		for(int i=0;i<5;i++){
			printf("%c",name_array[i]);
		}
		LCD_set_cursor(0,1);
		printf("You weigh: %dkg",weight);
	}
	
   
		
	
	return 0;
}

void intro_screen(){
	 int name_array[5], count_l=0,written=0;			//Array for names, letter count, if name was written
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
	 eeprom_write_byte((uint8_t *)0,1);
	
}

