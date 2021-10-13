/*
 * GccApplication21.cpp
 *
 * Created: 10/10/2021 16:57:19
 * Author : tellSlater
 */ 


#define F_CPU   8000000
#define BUAD    9600
#define BRC     ((F_CPU/16/BUAD) - 1)

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

volatile uint8_t flips = 0;
volatile uint16_t secSleep = 0;
	

void inline setup()
{
	DDRB = 0x00;
	PORTB = 0x00;
	DDRB &= ~(1 << PINB1);											//I/O inputs
	PORTB |= 1 << PINB1;											//PULL UP RESISTOR for input
	TCCR0A |= (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);			//PWM
	TCCR0B |= 1 << CS02;											//PWM
	TIMSK0 |= 1 << TOIE0;											//Timer0 overflow interrupt
	MCUCR |= (1 << SM1) | (1 << SE);								//Sleep mode selection
}

bool sensorFlipped()												//Gets a new sensor sample and checks if it is flipped through rolling mean
{
	static uint8_t sensorIns = 0x00;
	static uint8_t i = 0;
	static bool sensorPos = 1;
	uint8_t sum = 0;
	
	if (PINB & (1 << PINB1))
	{
		sensorIns |= 1 << i;
	}
	else
	{
		sensorIns &= ~(1 << i);
	}
	i < 7? i++ : i=0;
	
	for (uint8_t j=0; j<8; ++j)
	{
		sum += (sensorIns >> j) & 0x01;
	}
	
	if ((sensorPos && sum<3) || (!sensorPos && sum>6))
	{
		sensorPos = !sensorPos;
		return true;
	}
	else
		return false;
}

inline void sleep()
{
	secSleep = 0;
	GIMSK |= 1 << INT0;			//Enable external interrupt for awakening
	sleep_mode();
	GIMSK &= ~(1 << INT0);		//Disable external interrupt after awakening
}

inline void rampUP()
{
	DDRB |= 1 << PINB0;
	while (OCR0A < 0xff)
	{
		OCR0A++;
		_delay_ms(16);
	}
}

inline void rampDOWN()
{
	while (OCR0A > 0x00)
	{
		OCR0A--;
		_delay_ms(16);
	}
	DDRB &= ~(1 << PINB0);
	sleep();
}


int main(void)
{
	setup();
	sei();
    while (1)
    {
	    _delay_ms(5);
		flips += sensorFlipped();
		if (flips > 6)
		{
			flips = 0;
			secSleep = 0;
			OCR0A ? rampDOWN() : rampUP();
		}
		if (secSleep > 120 || (secSleep > 10 && !OCR0A))
		{
			rampDOWN();
		}
    }
}

ISR (TIM0_OVF_vect) //Timer 0 overflow interrupt used for all the timing needs
{
	static uint8_t smallTimer = 0;
	smallTimer++;
	if (smallTimer > 122)
	{
		smallTimer = 0;
		secSleep++;
		if (flips > 0) flips --;
	}
}

ISR (INT0_vect) //External interrupt used to wake from sleep
{
}