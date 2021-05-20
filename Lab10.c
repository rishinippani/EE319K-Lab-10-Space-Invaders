// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/16/2021 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "DAC.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
#define MS 8000000

// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x32;      // activate port B,F,E
  while((SYSCTL_PRGPIO_R&0x32) != 0x32){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x1F;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
	GPIO_PORTE_DIR_R |= 0x00;
	GPIO_PORTE_DEN_R |= 0x1E;
}

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x30;      // activate port F & E
  while((SYSCTL_PRGPIO_R&0x30) != 0x30){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 (built-in LED)
  GPIO_PORTF_PUR_R |= 0x10;
  GPIO_PORTF_DEN_R |=  0x1F;   // enable digital I/O on PF
	GPIO_PORTE_DIR_R |= 0x00;
	GPIO_PORTE_DEN_R |= 0x10;
}



//********************************************************************************

struct SpriteInformation {

	const uint8_t *ptr;
	uint8_t x;
	uint8_t y;
	uint8_t thresholdval;
	uint16_t spritecolor;

};

struct LaserInformation {

	const uint8_t *ptr;
	uint8_t x;
	uint8_t y;
	uint8_t vy;
	uint8_t thresholdval;
	uint16_t spritecolor;

};
	
typedef struct LaserInformation LaserLog;

LaserLog Lasers[1] = {

{Laser0, 0, 0, 3, 0, SSD1306_WHITE},

};

struct LaserInformation2 {

	const uint8_t *ptr;
	uint8_t x;
	uint8_t y;
	uint8_t vy;
	uint8_t thresholdval;
	uint16_t spritecolor;

};

typedef struct LaserInformation LaserLog2;

LaserLog2 EnemyLaser[1] = {

	{Laser1, 0, 0, 3, 0, SSD1306_WHITE},

};



typedef struct SpriteInformation SpriteLog;

SpriteLog Info[9] = {


	{PlayerShip0, 47, 63, 0, SSD1306_WHITE},
	{Bunker0, 53, 57, 0, SSD1306_WHITE},
	{Alien10pointA, 0, 9, 0, SSD1306_WHITE},
	{Alien10pointB, 15, 9, 0, SSD1306_WHITE},
	{Alien20pointA, 30, 9, 0, SSD1306_WHITE},
	{Alien20pointB, 45, 9, 0, SSD1306_WHITE},
	{Alien30pointA, 60, 9, 0, SSD1306_WHITE},
	{Alien30pointB, 75, 9, 0, SSD1306_WHITE},
	{AlienBossA, 94, 9, 0, SSD1306_WHITE},
	
};


int AlienPosition = 9;

void DrawScreen(void) {

		SSD1306_ClearBuffer();
		SSD1306_DrawBMP(Info[0].x, Info[0].y, Info[0].ptr, Info[0].thresholdval, Info[0].spritecolor);
		SSD1306_DrawBMP(Info[1].x, Info[1].y, Info[1].ptr, Info[1].thresholdval, Info[1].spritecolor);
		SSD1306_DrawBMP(Info[2].x, Info[2].y, Info[2].ptr, Info[2].thresholdval, Info[2].spritecolor);
		SSD1306_DrawBMP(Info[3].x, Info[3].y, Info[3].ptr, Info[3].thresholdval, Info[3].spritecolor);
		SSD1306_DrawBMP(Info[4].x, Info[4].y, Info[4].ptr, Info[4].thresholdval, Info[4].spritecolor);
		SSD1306_DrawBMP(Info[5].x, Info[5].y, Info[5].ptr, Info[5].thresholdval, Info[5].spritecolor);
		SSD1306_DrawBMP(Info[6].x, Info[6].y, Info[6].ptr, Info[6].thresholdval, Info[6].spritecolor);
		SSD1306_DrawBMP(Info[7].x, Info[7].y, Info[7].ptr, Info[7].thresholdval, Info[7].spritecolor);
		SSD1306_DrawBMP(Info[8].x, Info[8].y, Info[8].ptr, Info[8].thresholdval, Info[8].spritecolor);
		SSD1306_OutBuffer();
		SSD1306_SetCursor(0,0);
		LCD_OutDec(AlienPosition);

}



void DrawLasers(void) {

		SSD1306_ClearBuffer();
		SSD1306_DrawBMP(Info[0].x, Info[0].y, Info[0].ptr, Info[0].thresholdval, Info[0].spritecolor);
		SSD1306_DrawBMP(Info[1].x, Info[1].y, Info[1].ptr, Info[1].thresholdval, Info[1].spritecolor);
		SSD1306_DrawBMP(Info[2].x, Info[2].y, Info[2].ptr, Info[2].thresholdval, Info[2].spritecolor);
		SSD1306_DrawBMP(Info[3].x, Info[3].y, Info[3].ptr, Info[3].thresholdval, Info[3].spritecolor);
		SSD1306_DrawBMP(Info[4].x, Info[4].y, Info[4].ptr, Info[4].thresholdval, Info[4].spritecolor);
		SSD1306_DrawBMP(Info[5].x, Info[5].y, Info[5].ptr, Info[5].thresholdval, Info[5].spritecolor);
		SSD1306_DrawBMP(Info[6].x, Info[6].y, Info[6].ptr, Info[6].thresholdval, Info[6].spritecolor);
		SSD1306_DrawBMP(Info[7].x, Info[7].y, Info[7].ptr, Info[7].thresholdval, Info[7].spritecolor);
		SSD1306_DrawBMP(Info[8].x, Info[8].y, Info[8].ptr, Info[8].thresholdval, Info[8].spritecolor);
		SSD1306_DrawBMP(Lasers[0].x, Lasers[0].y, Lasers[0].ptr, Lasers[0].thresholdval, Lasers[0].spritecolor);
		SSD1306_DrawBMP(EnemyLaser[0].x, EnemyLaser[0].y, EnemyLaser[0].ptr, EnemyLaser[0].thresholdval, EnemyLaser[0].spritecolor);
		SSD1306_OutBuffer();
		SSD1306_SetCursor(0,0);
		LCD_OutDec(AlienPosition);

}


 
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

uint32_t Convert(uint32_t input){
// from lab 8
	int converted = 0;
	
	converted = (128*input)/4096;
	
	
	
	return converted;
}


void SysTick_Init(uint32_t period){
    
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = period - 1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (0x00FFFFFF & NVIC_SYS_PRI3_R) | 0x20000000;
	NVIC_ST_CTRL_R = 0x0007;
}





int main6(void){uint32_t time=0;
  DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();   
  Random_Init(1);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  EnableInterrupts();
  Delay100ms(20);
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(47, 63, PlayerShip0, 0, SSD1306_WHITE); // player ship bottom
  SSD1306_DrawBMP(53, 55, Bunker0, 0, SSD1306_WHITE);

  SSD1306_DrawBMP(0, 9, Alien10pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(10,9, Alien10pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(30, 9, Alien20pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(70, 9, Alien20pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(90, 9, Alien30pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(50, 9, AlienBossA, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  Delay100ms(30);

  SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("GAME OVER");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Nice try,");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("Earthling!");
  SSD1306_SetCursor(2, 4);
  while(1){
    Delay100ms(10);
    SSD1306_SetCursor(19,0);
    SSD1306_OutUDec2(time);
    time++;
    PF1 ^= 0x02;
  }
}


uint32_t ADCData = 0;
int ConvertedX;
int TravelCounter = 0;
int DownFlag = 0;
int i;
int k = 0;
int LaserFlag = 0;
int LowThresh = 0;
int HighThresh = 20;
int m;
int n;
int EnemyFlag;
int MailStatus;
int LowPlayerThresh;
int HighPlayerThresh;
int LossFlag = 0;
int CollisionCounter = 0;
int English = 1;
int Spanish = 0;
int SetLanguage;


uint32_t Movement(void) {

	ADCData = ADC_In();
	Info[0].x = Convert(ADCData);
			
			if (Info[0].x > 110) {
			
			Info[0].x = 110;
			}

		return Info[0].x;
	}

	



	
int main (void) {
    DisableInterrupts();
		TExaS_Init(&LogicAnalyzerTask);
		ADC_Init();
		Random_Init(1);
		Profile_Init(); // PB5,PB4,PF3,PF2,PF1
		DAC_Init();
		
			
		Random_Init(1);
    Random_Init(NVIC_ST_CURRENT_R);
	

    SSD1306_Init(SSD1306_SWITCHCAPVCC);
		SysTick_Init(MS);
		
    SSD1306_OutClear();
    SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
    SSD1306_OutBuffer();
    Delay100ms(20);
		SSD1306_OutClear();  
		SSD1306_SetCursor(0, 0);
		SSD1306_OutString("Choose your Language:");
		SSD1306_SetCursor(1, 1);
		SSD1306_OutString("English (GREEN)");
		SSD1306_SetCursor(1, 2);
		SSD1306_OutString("Spanish (BLUE)");
		SSD1306_SetCursor(0, 4);
		SSD1306_OutString("*IF ALIENS REACH 56 ");
		SSD1306_SetCursor(0, 5);
		SSD1306_OutString("OR ABOVE YOU LOSE*");	
		SSD1306_SetCursor(0, 6);
		SSD1306_OutString("(WATCH TOP-LEFT)");
		
		
		while (((GPIO_PORTE_DATA_R & 0x10) == 0) && ((GPIO_PORTE_DATA_R & 0x08) == 0)){};
			
		if (GPIO_PORTE_DATA_R == 16) {
		
		SetLanguage = 1;
		
		
		}	
		
		if (GPIO_PORTE_DATA_R == 8) {
		
		SetLanguage = 0;
		
		}
		
	
	
		SSD1306_ClearBuffer();
		Delay100ms(30);
    
		EnableInterrupts();
	
    while(1){

		DrawScreen();
			
			
		for (k=0;k<7;k++) {
		
		
			for (i=2;i<9;i++) {
			
			Info[i].x += 2;
			
			DrawScreen();
				
			
			}
		
		}

		for (i = 2; i< 9; i++) {
		
				Info[i].y += 3;
			
			
		if ((Info[2].y >= 56 & Info[2].y <= 63) || (Info[3].y >= 56 & Info[3].y <= 63) || (Info[4].y >= 56 & Info[4].y <= 63) || (Info[5].y >= 56 & Info[5].y <= 63) || (Info[6].y >= 56 & Info[6].y <= 63) || (Info[7].y >= 56 & Info[7].y <= 63) || (Info[8].y >= 56 & Info[8].y <= 63)) {
		
			LossFlag = 1;
			SSD1306_OutClear();
			DisableInterrupts();
		
			while (LossFlag == 1) {
		
			if (SetLanguage == 1) {
		
			DisableInterrupts();
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("GAME OVER");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("Nice try,");
			SSD1306_SetCursor(1, 3);
			SSD1306_OutString("Earthling!");
			SSD1306_SetCursor(2, 5);
			SSD1306_OutString("*PRESS RESET TO");
			SSD1306_SetCursor(2, 6);
			SSD1306_OutString("TRY AGAIN*");
				
			}
			
			if (SetLanguage == 0) {
			
			DisableInterrupts();
			SSD1306_SetCursor(1, 0);
			SSD1306_OutString("JUEGO TERMINADO");
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("BUEN INTENTO,");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("TERRICOLA!");
			SSD1306_SetCursor(2, 4);
			SSD1306_OutString("*PRESIONE");
			SSD1306_SetCursor(2, 5);
			SSD1306_OutString("RESTABLECER");
			SSD1306_SetCursor(2, 6);
			SSD1306_OutString("PARA VOLVER");	
			SSD1306_SetCursor(2, 7);
			SSD1306_OutString("A INTENTARLO*");
			
			
			}
				
		}	
		
		
	}  	
			
			
			
			
		}
		
		AlienPosition += 3;
		
		DrawScreen();
		
		
		for (k=0;k<7;k++) {
		
		
			for (i=2;i<9;i++) {
			
			Info[i].x -= 2;
			
			DrawScreen();
			
			}
			
		
		}
		
		for (i = 2; i< 9; i++) {
		
		Info[i].y += 3;
			
			
		if ((Info[2].y >= 56 & Info[2].y <= 63) || (Info[3].y >= 56 & Info[3].y <= 63) || (Info[4].y >= 56 & Info[4].y <= 63) || (Info[5].y >= 56 & Info[5].y <= 63) || (Info[6].y >= 56 & Info[6].y <= 63) || (Info[7].y >= 56 & Info[7].y <= 63) || (Info[8].y >= 56 & Info[8].y <= 63)) {
		
			DisableInterrupts();
			LossFlag = 1;
			SSD1306_OutClear();
		
			while (LossFlag == 1) {
		
		if (SetLanguage == 1) {
		
			DisableInterrupts();
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("GAME OVER");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("Nice try,");
			SSD1306_SetCursor(1, 3);
			SSD1306_OutString("Earthling!");
			SSD1306_SetCursor(2, 5);
			SSD1306_OutString("*PRESS RESET TO");
			SSD1306_SetCursor(2, 6);
			SSD1306_OutString("TRY AGAIN*");
				
			}
			
			if (SetLanguage == 0) {
			
			DisableInterrupts();
			SSD1306_SetCursor(1, 0);
			SSD1306_OutString("JUEGO TERMINADO");
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("BUEN INTENTO,");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("TERRICOLA!");
			SSD1306_SetCursor(2, 4);
			SSD1306_OutString("*PRESIONE");
			SSD1306_SetCursor(2, 5);
			SSD1306_OutString("RESTABLECER");
			SSD1306_SetCursor(2, 6);
			SSD1306_OutString("PARA VOLVER");	
			SSD1306_SetCursor(2, 7);
			SSD1306_OutString("A INTENTARLO*");
			
			
			}
			
			
		}
			
		DrawScreen();
		
		
		
		}  	
			
			
		}
		
		AlienPosition += 3;
		
		
	

	}




}

//COLLISION CHECKER FOR EACH ALIEN

void CollsionAlien10A (void) {

	LowThresh = Info[2].x;
	HighThresh = Info[2].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[2].y + 1)) {
	
		Info[2].ptr = BigExplosion1;
		Info[2].x = 128;
		Info[2].y = 72;
		CollisionCounter+= 1;
		GetTrigger(1);
		LaserFlag = 0;
		
	
	}

}

void CollsionAlien10B (void) {

	LowThresh = Info[3].x;
	HighThresh = Info[3].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[3].y + 1)) {
	
		Info[3].ptr = BigExplosion1;
		Info[3].x = 128;
		Info[3].y = 72;
		CollisionCounter+= 1;
			GetTrigger(1);
		LaserFlag = 0;
	}

}

void CollsionAlien20A (void) {

	LowThresh = Info[4].x;
	HighThresh = Info[4].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[4].y + 1)) {
	
		Info[4].ptr = BigExplosion1;
		Info[4].x = 128;
		Info[4].y = 72;
		CollisionCounter+= 1;
			GetTrigger(1);
		LaserFlag = 0;
		
	}

}

void CollsionAlien20B (void) {

	LowThresh = Info[5].x;
	HighThresh = Info[5].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[4].y + 1)) {
	
		Info[5].ptr = BigExplosion1;
		Info[5].x = 128;
		Info[5].y = 72;
		CollisionCounter+= 1;
			GetTrigger(1);
		LaserFlag = 0;
	}

}

void CollsionAlien30A (void) {

	LowThresh = Info[6].x;
	HighThresh = Info[6].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[5].y + 1)) {
	
		Info[6].ptr = BigExplosion1;
		Info[6].x = 128;
		Info[6].y = 72;
		CollisionCounter+= 1;
			GetTrigger(1);
		LaserFlag = 0;
	}

}

void CollsionAlien30B (void) {

	LowThresh = Info[7].x;
	HighThresh = Info[7].x + 12;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[6].y + 1)) {
	
		Info[7].ptr = BigExplosion1;
		Info[7].x = 128;
		Info[7].y = 72;
		CollisionCounter+= 1;
		GetTrigger(1);
		LaserFlag = 0;
	}

}

void CollsionAlienBossA (void) {

	LowThresh = Info[8].x;
	HighThresh = Info[8].x + 16;
	
	if ((Lasers[0].x >= LowThresh && Lasers[0].x <= HighThresh) && (Lasers[0].y <= Info[7].y + 1)) {
	
		Info[8].ptr = BigExplosion1;
		Info[8].x = 128;
		Info[8].y = 72;
		CollisionCounter+= 1;
		GetTrigger(1);
		LaserFlag = 0;
	}

}

void PlayerCollision (void) {

	LowPlayerThresh = Info[0].x;
	HighPlayerThresh = Info[0].x + 15;
	
	if ((EnemyLaser[0].x >= LowPlayerThresh && EnemyLaser[0].x <= HighPlayerThresh) && (EnemyLaser[0].y >= 57)) {
	
	Info[0].ptr = BigExplosion1;
	LossFlag = 1;
	
	
	
	
	} 


}













//COLLISION CHECKER FOR EACH ALIENT



// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

void SysTick_Handler(void){ 
  	
	
	if (CollisionCounter != 7) {
		
	
	ADCData = ADC_In();
	Info[0].x = Convert(ADCData);
			
			if (Info[0].x > 110) {
			
			Info[0].x = 110;
			}
			
	DrawScreen();

	if ((GPIO_PORTE_DATA_R & 0x04) != 0) {
	
		if (LaserFlag == 0) {
		
		Lasers[0].x = Info[0].x + 7; //x + 7
		Lasers[0].y = Info[0].y - 20; //y - 20
		LaserFlag = 1;
		GetTrigger(0);
		
			
		}	
			

		//while (Lasers[0].y != 9) {
		
		Lasers[0].y -= Lasers[0].vy;
		DrawLasers();
	
		//}

		if (Lasers[0].y < AlienPosition) {
		
			LaserFlag = 0;
			
		
		}
		
		
		CollsionAlien10A();
		CollsionAlien10B();
		CollsionAlien20A();
		CollsionAlien20B();
		CollsionAlien30A();
		CollsionAlien30B();
		CollsionAlienBossA();
		
	
	

}
	
}
	
	if (CollisionCounter == 7) {
	
		DisableInterrupts();
		
		SSD1306_OutClear();
	
		while(1) {
			
		if (SetLanguage == 1) {
			
		DisableInterrupts();	
		SSD1306_SetCursor(0, 1);
		SSD1306_OutString("Good job!");
		SSD1306_SetCursor(0, 2);
		SSD1306_OutString("We've won, for now...");
		SSD1306_SetCursor(2, 5);
		SSD1306_OutString("*PRESS RESET TO");
		SSD1306_SetCursor(2, 6);
		SSD1306_OutString("DO IT AGAIN*");
		
		}
		
		if (SetLanguage == 0) {
		
		DisableInterrupts();	
		SSD1306_SetCursor(0, 0);
		SSD1306_OutString("Buen trabajo!");
		SSD1306_SetCursor(0, 1);
		SSD1306_OutString("Salvamos el ");
		SSD1306_SetCursor(0, 2);
		SSD1306_OutString("Mundo,");
		SSD1306_SetCursor(0, 3);
		SSD1306_OutString("por ahora...");
		SSD1306_SetCursor(2, 4);
		SSD1306_OutString("*PRESIONE");
		SSD1306_SetCursor(2, 5);
		SSD1306_OutString("RESTABLECER");
		SSD1306_SetCursor(2, 6);
		SSD1306_OutString("PARA VOLVER");	
		SSD1306_SetCursor(2, 7);
		SSD1306_OutString("A INTENTARLO*");
		
		
		
		}
			
		}
		
	
	}

}
