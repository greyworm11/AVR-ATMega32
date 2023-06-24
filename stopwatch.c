
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#define F_CPU 8000000UL // ???????;
#define TRUE 1
#define FALSE 0

unsigned char znak[10] = {0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001,
						  0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};

unsigned char minutes = 0;
volatile uint8_t seconds = 0;

unsigned char pause_flag = TRUE;

unsigned char segcounter = 0;
unsigned char low_sec = 0;
unsigned char high_sec = 0;
unsigned char low_min = 0;
unsigned char high_min = 0;

uint8_t seg_num[4] = {1, 2, 4, 8};

#define MAX_SAVED_VALUE_COUNT 10

unsigned char eeprom_memory[MAX_SAVED_VALUE_COUNT + 1];

unsigned char saved_value_count = 0;
unsigned char current_value_index = 0;
unsigned char saved_sek_value[MAX_SAVED_VALUE_COUNT];


/* ????????? ?????????? INT0 ? INT1 */
void inter_init()
{
	GICR = 0b11000000;
	MCUCR = 0b00001010;
}


/* ????????? ?????????? ?? T0 ? T1
	T0 - ?? ????????????
	T1 - ?? ??????????
*/
void timer_init()
{
	TCCR1B |= (1 << WGM12); // ????????? ?????? ???????? ? CTC (?????????????? ????);
	TIMSK |= (1 << OCIE1A); // ?????????? ?????????? ?? ?????????? ? ????????? OCR1A;
	TCCR1B |= ((1 << CS10) | (1 << CS12)); // ????????? ???????? ?? 1024;
	OCR1A = 970; // ????????? ????????, ??? ??????? ????????????? ?????????? ????? 1 ???;
}

ISR(TIMER1_COMPA_vect)
{
	seconds++;
	
	if(seconds > 9999)
		seconds = 0;
}

ISR (INT0_vect)
{
	if(pause_flag == FALSE)
	{
		pause_flag = TRUE;
		return;
	}
	else
	{
		pause_flag = FALSE;
		return;
	}
}


ISR (INT1_vect)
{
	seconds = 0;
	pause_flag = TRUE;
}


void read_epprom()
{
	saved_value_count = eeprom_read_byte(&eeprom_memory[0]);
	for (int i = 0; i < saved_value_count; ++i)
	{
		saved_sek_value[i] = eeprom_read_byte(&eeprom_memory[i + 1]);
	}
}


void write_epprom()
{
	eeprom_write_byte(&eeprom_memory[0], saved_value_count);
	for (int i = 0; i < saved_value_count; ++i)
	{
		eeprom_write_byte(&eeprom_memory[i + 1], saved_sek_value[i]);
	}
}


int main(void)
{
	timer_init();
	inter_init();
	
	DDRA = 0xFF;
	DDRC = 0xFF;
	
	PORTD = 0b00001111;
	// DDRD = 0b11110000;
	
	sei(); // global interrupts enable
	seconds = 0;
	pause_flag = FALSE; // CHANGE
	unsigned int prev_seconds = 0;
	unsigned char is_prev_saved = FALSE;
	
    while (1) 
    {
		if(!(PIND & 0b00000001) && pause_flag == FALSE)
		{
			saved_sek_value[saved_value_count] = seconds;
			saved_value_count++;
			if(saved_value_count > MAX_SAVED_VALUE_COUNT)
				saved_value_count = 0;
			write_epprom();
		}
		else if(!(PIND & 0b00000010) && pause_flag == TRUE)
		{
			read_epprom();
			seconds = saved_sek_value[current_value_index];
			current_value_index++;
			if(current_value_index > saved_value_count)
				current_value_index = 0;
			prev_seconds = seconds;
		}
		else if(pause_flag == TRUE && (PIND & 0b00000010))
		{
			if(is_prev_saved == FALSE)
			{
				prev_seconds = seconds;
				is_prev_saved = TRUE;
			}
			seconds = prev_seconds;
		}
		else
			is_prev_saved = FALSE;
		////////////////////////////////////////////////////////////////////
		// seconds to minutes and seconds
		unsigned char case_3 = seconds % 60 % 10;
		unsigned char case_2 = (seconds - case_3) % 60 / 10;
		unsigned char case_1 = (seconds - (case_2 * 10 + case_3)) / 60 % 10;
		unsigned char case_0 = seconds / 600;
		low_sec = case_3; high_sec = case_2;
		low_min = case_1; high_min = case_0;
		
		PORTC = 0x00;
		PORTA = seg_num[segcounter];
		if(segcounter == 3)
		{
			PORTC = ~znak[high_min];
		}
		else if(segcounter == 2)
		{
			PORTC = (~znak[low_min]) | 0b10000000;
		}
		else if(segcounter == 1)
		{
			PORTC = ~znak[high_sec];
		}
		else if(segcounter == 0)
		{
			PORTC = ~znak[low_sec];
		}
		segcounter++;
		if(segcounter > 3) segcounter = 0;
    }
}

