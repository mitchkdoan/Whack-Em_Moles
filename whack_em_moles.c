/*	Partner 1 Name & E-mail: Mitchell Doan; mdoan005@ucr.edu
 *	Lab Section: 28
 *	Assignment: Custom Project  
 *	Exercise Description: [2 Player Whack a Mole]
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.c"

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void transmit_data(unsigned char data) {
	unsigned char i;	// for each bit of data
	for (i = 0; i < 8; ++i) {	// Set SRCLR to 1 allowing data to be set
		PORTC = 0x08;	// Also clear SRCLK in preparation of sending data
		PORTC |= ((data >> i) & 0x01);	// set SER = next bit of data to be sent.
		PORTC |= 0x04;	// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
	}
	PORTC |= 0x02;	// end for each bit of data
	PORTC = 0x00;	// set RCLK = 1. Rising edge copies data from the "Shift" register to the "Storage"
	// clears all lines in preparation of a new transmission
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

unsigned char generateMole() {
	unsigned char temp;
	temp = (rand() % 8);
	return temp;
}

void LCD_Custom_Char (unsigned char loc, unsigned char *msg)
{
	unsigned char i;
	if(loc<8)
	{
		LCD_WriteCommand (0x40 + (loc*8));	/* Command 0x40 and onwards forces the device to point CGRAM address */
		for(i=0;i<8;i++)	/* Write 8 byte for generation of 1 character */
		LCD_WriteData(msg[i]);
	}
}

char customChar[] = {
	0x1F,
	0x1F,
	0x1F,
	0x04,
	0x04,
	0x04,
	0x04,
	0x00
};

char customChar2[] = {
  0x00,
  0x00,
  0x00,
  0x07,
  0x07,
  0x1F,
  0x07,
  0x07
};

char customChar3[] = {
	0x0E,
	0x1F,
	0x15,
	0x1F,
	0x1F,
	0x11,
	0x11,
	0x1F
};

char customChar4[] = {
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x0E,
	0x1F,
	0x15
};

	enum States {Start, Init, Welcome, Mode, DisplayHigh, Wait, Pop, Update1, Update2, Update3, Winner, Incorrect, NextLevel, Restart} state;
	void Tick();
	char Score1 = 0;
	char Score2 = 0;
	unsigned char hole;
	unsigned char counter;
	unsigned char waitCounter;
	unsigned char Level;
	unsigned char highScore = 0;
	unsigned char temp;
	unsigned char temp1;
	unsigned char temp2;
	unsigned char finalLights = 0x01;

int main(void)
{	DDRA = 0x00; PORTA = 0xFF; // Set Port A to input
	DDRB = 0x00; PORTB = 0xFF; // Set Port B to input
	DDRC = 0xFF; PORTC = 0x00; // Set Port C to output
	DDRD = 0xFF; PORTD = 0x00;
	state = Start;
	TimerSet(200);
	TimerOn();
	LCD_init();
	LCD_Custom_Char(0, customChar);
	LCD_Custom_Char(1, customChar2);
	LCD_Custom_Char(2, customChar3);
	LCD_Custom_Char(3, customChar4);
	//eeprom 
	if(eeprom_read_byte((uint8_t*)1) == 255) {
		eeprom_update_byte((uint8_t*)1, (uint8_t) 0);
	} 
		
	highScore = eeprom_read_byte((uint8_t*)1);
	
	srand(time(0));
	
	while(1) {
		
		Tick();// User code (i.e. synchSM calls)

		while (!TimerFlag);	// Wait 1 sec
		TimerFlag = 0;

	}
	return 0;
}

void Tick() {
	switch(state){ //Transitions
		case Start:
			state = Init;
			break;
			
		case Init:	
			 state = Welcome;
			 break;
			 
		case Welcome:
			 if (counter > 25) {
				 counter = 0;
				 LCD_ClearScreen();
				 state = Mode;
				 break;
			 }
			 else {		
				state = Welcome;
				break;
			}

		case DisplayHigh:
			 if (((~PINA) & 0x02) == 0x02) {
				 LCD_ClearScreen();
				 state = Mode;
				 break;
			 }
			 else {		
				state = DisplayHigh;
				break;
			}
			
		case Mode:
			 if (((~PINA) & 0x01) == 0x01) {
	 			state = Wait;
	 			break;
			}
			
			else if(((~PINA) & 0x02) == 0x02) {
				LCD_ClearScreen();
				state = DisplayHigh;
				break;
			}
			
			else {
				state = Mode;
				break;
			}

		case Wait:
			if(Level >= 12) {
				counter = 0;
				LCD_ClearScreen();
				state = Winner;
				break;
			}
			else if (waitCounter > 5) {
				state = Pop;
				break;
			}
			else {
				state = Wait;
				break;
			}
			
		case Pop:	
			if (Level <= 3) {
				if ((((~PINA) & 0xFF) == hole) && (((~PINB) & 0xFF) == hole)) {
					state = Update3;
					break;
				}
				else if (((~PINA) & 0xFF) == hole) {
					state = Update1;
					break;
				}
				else if (((~PINB) & 0xFF) == hole) {
					state = Update2;
					break;
				}
				else if (counter > 30) {
					state = Incorrect;
					break;
				}
				else{
					state = Pop;
					break;
				}
			}
			else if (Level <= 6) {
				if ((((~PINA) & 0xFF) == hole) && (((~PINB) & 0xFF) == hole)) {
					state = Update3;
					break;
				}
				else if (((~PINA) & 0xFF) == hole) {
					state = Update1;
					break;
				}
				else if (((~PINB) & 0xFF) == hole) {
					state = Update2;
					break;
				}
				else if (counter > 15) {
					state = Incorrect;
					break;
				}
				else{
					state = Pop;
					break;
				}				
			}
			else {
				if ((((~PINA) & 0xFF) == hole) && (((~PINB) & 0xFF) == hole)) {
					state = Update3;
					break;
				}
				else if (((~PINA) & 0xFF) == hole) {
					state = Update1;
					break;
				}
				else if (((~PINB) & 0xFF) == hole) {
					state = Update2;
					break;
				}
				else if (counter > 8) {
					state = Incorrect;
					break;
				}
				else{
					state = Pop;
					break;
				}
			}
			
		case Update1:
			state = NextLevel;
			break;	
					
		case Update2:
			state = NextLevel;
			break;

		case Update3:
			state = NextLevel;
			break;
		
		case Winner:
			if(counter > 40) {
				counter = 0;
				LCD_ClearScreen();
				state = Restart;
				break;
			}
			else {
				state = Winner;
				break;
			}
			
		case Incorrect:
			state = NextLevel;
			break;
			
		case NextLevel:
			state = Wait;
			break;
			
		case Restart:
			if (((~PINA) & 0x01) == 0x01) {
				counter = 0;
				LCD_ClearScreen();
				Level = 0;
				Score1 = 0;
				Score2 = 0;
				state = Mode;
			}
			else {
				state = Restart;
			}
	}
	
	switch(state){ //Actions
		case Start:
			break;
			
		case Init:
			hole = 0x01;
			transmit_data(0x00);
			Score1 = 0;
			Score2 = 0;
			counter = 0;
			waitCounter = 0;
			Level = 0;
			break;
			
		case Welcome:
			LCD_DisplayString(1, "Welcome To");
			LCD_DisplayString(17, "Whack'Em Moles");
			if ((counter % 4 == 0) || (counter % 4 == 1)){
				LCD_Cursor(16);
				LCD_WriteData(0);
				LCD_Cursor(32);
				LCD_WriteData(2);
			}
			else {
				LCD_Cursor(16);
				LCD_WriteData(1);
				LCD_Cursor(32);
				LCD_WriteData(3);
			}
			LCD_Cursor(33);
			counter = counter + 1;
			break;

		case DisplayHigh:
			LCD_DisplayString(1, "High Score: ");
			LCD_Cursor(13);
			if (highScore > 9) {
				temp1 = highScore / 10;
				temp2 = highScore % 10;
				LCD_WriteData(temp1 + '0');
				LCD_Cursor(14);
				LCD_WriteData(temp2 + '0');
			}
			else {
				LCD_WriteData((highScore) + '0');
			}
			LCD_DisplayString(17, "Back: A2");
			LCD_Cursor(33);
			break;
			
		case Mode:
			//LCD_DisplayString(1, "Single Player");
			//LCD_DisplayString(17, "Multiplayer");
			//LCD_Cursor(33);
			transmit_data(0x00);
			LCD_DisplayString(1, "Start Game: A1");
			LCD_DisplayString(17, "High Score: A2");	
			LCD_Cursor(33);
			break;
		
		case Wait:
			LCD_ClearScreen();
			transmit_data(0x00);
			LCD_DisplayString(1, "Player 1: ");
			LCD_Cursor(11);
			if(Score1 > 9) {
				temp1 = Score1 / 10;
				temp2 = Score2 % 10;
				LCD_WriteData(temp1 + '0');
				LCD_Cursor(12);
				LCD_WriteData(temp2 + '0');
			}
			else {
				LCD_WriteData((Score1) + '0');
			}
			LCD_DisplayString(17, "Player 2: ");
			LCD_Cursor(27);
			if(Score2 > 9) {
				temp1 = Score2 / 10;
				temp2 = Score2 % 10;
				LCD_WriteData(temp1 + '0');
				LCD_Cursor(28);
				LCD_WriteData(temp2 + '0');
			}
			else {
				LCD_WriteData((Score2) + '0');
			}
			waitCounter = waitCounter + 1;
			LCD_Cursor(33);
			break;
			
		case Pop:
			transmit_data(hole);
			counter = counter + 1;
			waitCounter = 0;
			break;
			
		case Update1:
			transmit_data(0x00);
			if (Level <= 4) {
			    Score1 = Score1 + 1;
			}
			else if(Level <= 8) {
				Score1 = Score1 + 2;
			}
			else {
			    Score1 = Score1 + 3;
			}
			break;
		
		case Update2:
			transmit_data(0x00);
			if (Level <= 4) {
			    Score2 = Score2 + 1;
			}
			else if(Level <= 8) {
			    Score2 = Score2 + 2;
			}
			else {
			    Score2 = Score2 + 3;
			}
			break;
			
		case Update3:
			transmit_data(0x00);
			if (Level <= 4) {
			    Score1 = Score1 + 1;
			    Score2 = Score2 + 1;
			}
			else if(Level <= 8) {
			    Score1 = Score1 + 2;
			    Score2 = Score2 + 2;
			}
			else {
			    Score1 = Score1 + 3;
			    Score2 = Score2 + 3;
			}
			break;
			
		case Winner:
			if (counter > 0) {
				if (counter % 2 == 0) {
					if (Score1 > Score2) {
						transmit_data(0xF0);
					}
					else if (Score2 > Score1) {	
						transmit_data(0x0F);
					}
					else {
						transmit_data(0xFF);
					}
				}
				else {
					transmit_data(0x00);
				}
				counter = counter + 1;
				break;
			}
			if(Score1 > Score2) {
				transmit_data(0xF0);
				LCD_DisplayString(1, "Winner Player 1!");
				if(Score1 > highScore) {
					highScore = Score1;
					eeprom_update_byte((uint8_t*)1, (uint8_t)highScore);
					LCD_DisplayString(17, "New High: ");
					LCD_Cursor(27);
					if (highScore > 9) {
						temp1 = highScore / 10;
						temp2 = highScore % 10;
						LCD_WriteData(temp1 + '0');
						LCD_Cursor(28);
						LCD_WriteData(temp2 + '0');
					}
					else {
						LCD_WriteData((highScore) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
				else {
					LCD_DisplayString(17, "Score: " );
					LCD_Cursor(24);
					if (Score1 > 9) {
						temp1 = Score1 / 10;
						temp2 = Score1 % 10;
						LCD_WriteData((temp1) + '0');
						LCD_Cursor(25);
						LCD_WriteData((temp2) + '0');
					}
					else {
						LCD_WriteData((Score1) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
			}
			else if (Score2 > Score1) {
				transmit_data(0x0F);
				LCD_DisplayString(1, "Winner Player 2!");
				if(Score2 > highScore) {
					highScore = Score2;
					eeprom_update_byte((uint8_t*)1, (uint8_t)highScore);
					LCD_DisplayString(17, "New High: ");
					LCD_Cursor(27);
					if (highScore > 9) {
						temp1 = highScore / 10;
						temp2 = highScore % 10;
						LCD_WriteData((temp1) + '0');
						LCD_Cursor(28);
						LCD_WriteData((temp2) + '0');
					}
					else {
						LCD_WriteData((Score2) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
				else {
					LCD_DisplayString(17, "Score: " );
					LCD_Cursor(24);
					if (Score2 > 9) {
						temp1 = Score2 / 10;
						temp2 = Score2 % 10;
						LCD_WriteData((temp1) + '0');
						LCD_Cursor(25);
						LCD_WriteData((temp2) + '0');
					}
					else {
						LCD_WriteData((Score2) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
			}
			else {
				transmit_data(0xF0);
				LCD_DisplayString(1, "Winner: Tie!");
				if(Score1 > highScore) {
					highScore = Score1;
					eeprom_update_byte((uint8_t*)1, (uint8_t)highScore);
					LCD_DisplayString(17, "New High: ");
					LCD_Cursor(27);
					if (highScore > 9) {
						temp1 = highScore / 10;
						temp2 = highScore % 10;
						LCD_WriteData((temp1) + '0');
						LCD_Cursor(28);
						LCD_WriteData((temp2) + '0');
					}
					else {
						LCD_WriteData((highScore) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
				else {
					LCD_DisplayString(17, "Score: " );
					LCD_Cursor(24);
					if (Score1 > 9) {
						temp1 = Score1 / 10;
						temp2 = Score1 % 10;
						LCD_WriteData((temp1) + '0');
						LCD_Cursor(25);
						LCD_WriteData((temp2) + '0');
					}
					else {
						LCD_WriteData((Score1) + '0');
					}
					LCD_Cursor(33);
					counter = counter + 1;
					break;
				}
			}

			
		case Incorrect:
			transmit_data(0x00);
			counter = 0;
			break;
		
		case NextLevel:
			transmit_data(0x00);
			Level = Level + 1;
			if (Level < 5) {
				temp = generateMole();		
				hole = 0x01 << temp;
				break;
			}
			else if (Level < 9) {
				temp = generateMole();
				hole = 0x01 << temp;
				temp = generateMole();
				while (temp == hole) {
				    temp = generateMole();
				}
				hole = (hole | temp);
				break;
			}
			else {
				temp = generateMole();
				hole = 0x01 << temp;
				temp = generateMole();
				/*while (temp == hole) {
				    temp = generateMole();
				}*/
				hole = (hole | temp);
				temp = generateMole();
				/*while ((hole | temp) == hole) {
				    temp = generateMole();
				}*/
				hole = (hole | temp);
				break;
			}
		
		case Restart:
			LCD_DisplayString(1, "To Play Again:"); 
			LCD_DisplayString(17, "Press A1");
			LCD_Cursor(33);
			transmit_data(finalLights);
			if (finalLights == 0x80) {
				(finalLights = 0x08);
			}
			else if (finalLights == 0x01) {
				finalLights = 0x10;
			}
			else {
				if ((finalLights <= 0x08)) {
					finalLights = finalLights >> 1;
				} 
				else {
					finalLights = finalLights << 1;
				}
			}
			break;
	}
}