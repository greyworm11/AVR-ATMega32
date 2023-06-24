#define F_CPU 8000000UL
#define BAUD 19200L
#define TRUE 1
#define FALSE 0
#define MAX_BUFFER_SIZE 4
//*********************************************************************************************************************************************//
#include <avr/io.h>
#include <avr/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <math.h>
//*********************************************************************************************************************************************//
uint8_t SEGMENT[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x00};
uint8_t SEG_NUM[4] = {1, 2, 4, 8};
uint8_t data = 0;

uint8_t pause_flag = TRUE;
uint8_t segcounter = 0;
uint8_t voltage[4]= {10, 0, 0, 0}; // для вывода на семисегментный индикатор;
uint32_t adc_value = 0; // значение АЦП [0, 1024];
char high_adc = 0, low_adc = 0;

uint8_t rx_data = 0;
volatile uint8_t rx_flag = 0;
uint8_t buffer[MAX_BUFFER_SIZE];
uint8_t buffer_size = 0;
uint8_t buffer_complete = FALSE;

uint8_t t_uart = 0;
uint8_t t_adc = 0;
//*********************************************************************************************************************************************//
void UART_init()
{
	UBRRH = 0;
	UBRRL = 25; // baud = 19200;
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)|(1<<UPM1)|(1<<UPM0); //8 bit, 1 stop bit, odd parity
}
//*********************************************************************************************************************************************//
void UART_send(uint8_t data)
{
	while (!(UCSRA & (1 << UDRE)));
	UDR = data;
}
//*********************************************************************************************************************************************//
void UART_send_string(uint8_t* data)
{
	while(*data) UART_send(*data++);
}
//*********************************************************************************************************************************************//
uint8_t UART_get()
{
	while(!rx_flag);
	rx_flag = 0;
	return rx_data;
}
//*********************************************************************************************************************************************//
void ADC_init()
{
	ADCSRA |= (1 << ADEN) // Разрешение использования АЦП;
	|(1 << ADSC) // Запуск преобразования;
	|(1 << ADATE) // Непрерывный режим работы АЦП;
	/* |(1 << ADPS2) | (1 << ADPS1) */ | (1 << ADPS0) // Делитель 2;
	|(1 << ADIE); // Разрешение прерывания от АЦП;
	ADMUX |= (1 << ADLAR) | (1 << REFS1) | (1 << REFS0) | (1 << MUX2) | (1 << MUX0);
	//Внутренний Источник ОН 2,56в, ADLAR1 - в high младшие биты ,MUX2..0 - биты чтобы считывание шло с PORTA5;
}
//*********************************************************************************************************************************************//
void port_init()
{
	DDRA = 0xDF; // PA5 как вход для АЦП;
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRB = 0xFF;
	
	PORTD = 0x00;
	PORTC = 0x00;
	PORTA = 0x00;
	PORTB = 0x00;
}
//*********************************************************************************************************************************************//
ISR (ADC_vect)
{
	low_adc = ADCL;
	high_adc = ADCH;
	
	adc_value = (high_adc << 2);
	adc_value += (low_adc & 0b11000000) >> 6;
}
//*********************************************************************************************************************************************//
ISR (TIMER0_OVF_vect)
{
	PORTC = 0x00;
	_delay_us(1);
	TCNT0 = 0xDD;
	PORTA = SEG_NUM[segcounter];
	
	if(segcounter == 3)
	{
		PORTC = SEGMENT[voltage[0]];
	}
	else if(segcounter == 2)
	{
		PORTC = (SEGMENT[voltage[1]] | 0b10000000);
	}
	else if(segcounter == 1)
	{
		PORTC = SEGMENT[voltage[2]];
	}
	else if(segcounter == 0)
	{
		PORTC = SEGMENT[voltage[3]];
	}
	
	segcounter++;
	if(segcounter > 3) segcounter = 0;
}
//*********************************************************************************************************************************************//
void timer_init()
{
	// для таймера0;
	TIMSK = (1 << TOIE0);
	TCCR0 = (1 << CS02) | (0 << CS01) | (1 << CS00);
	TCNT0 = 0xB2;
	
	// для таймера1;
	TIMSK |= (1<<OCIE1A); // бит разрешения прерывания по совпадению;
	TCCR1B |= (1<<WGM12); // режим СТС (сброс по совпадению);
	OCR1AH = 0b00010000;
	OCR1AL = 0b10101010;
	TCCR1B |= (1 << CS12);
	
	
}
//******************************************************************************************//
void show_voltage()
{
	adc_value = adc_value / 2 - 7;
	voltage[1] = (adc_value / 100) % 10;
	voltage[2] = (adc_value % 100) / 10;
	voltage[3] = adc_value % 10;
}
//******************************************************************************************//
uint8_t is_digit(char buffer[])
{
	if(!(buffer[1] >= '0' && buffer[1] <= '2'))
	
	return TRUE;
}
//******************************************************************************************//
uint8_t get_time(char buffer[])
{	
	uint8_t res = 0;
	if(buffer_size == 4) // 3 разряда у числа;
	{
		res += (buffer[1] - '0') * 100;
		res += (buffer[2] - '0') * 10;
		res += (buffer[3] - '0');
	}
	else if(buffer_size == 3)
	{
		res += (buffer[1] - '0') * 10;
		res += (buffer[2] - '0');
	}
	else if(buffer_size == 2)
	{
		res += (buffer[1] - '0');
	}
	
	PORTB = res;
	
	return res;
}
//******************************************************************************************//
void proceed_buffer()
{
	if(buffer_size > MAX_BUFFER_SIZE) buffer[MAX_BUFFER_SIZE] = 0;
	
	if(buffer_size == 1)
	{
		if(buffer[0] == 's' && pause_flag == TRUE) // запуск осц, если он на паузе;
		{
			pause_flag = FALSE;
		}
		else if(buffer[0] == 'p' && pause_flag == FALSE) // остановка осц, если работает;
		{
			pause_flag = TRUE;
		}
	}
	else if(buffer_size > 1)
	{
		if(buffer[0] == 'o' && pause_flag == TRUE) // частота отправки данных на ПК [0, 255];
		{
			t_uart = buffer[1];
		}
		else if(buffer[0] == 'd' && pause_flag == TRUE) // частота дискретизации [0, 255];
		{
			t_adc = buffer[1];
			
			if(t_adc < 127)
			{
				OCR1AH = 0b00010000;
				OCR1AL = t_adc;
			}
			else
			{
				OCR1AH = 0b00001000;
				OCR1AL = ~t_adc;
			}
		}
	}
}
//******************************************************************************************//
int main()
{
	ADC_init();
	UART_init();
	timer_init();
	port_init();
	sei();
	int n;
	
	while(1)
	{
		if(buffer_complete == TRUE)
		{
			proceed_buffer();
			PORTB = 0;
			clean_buffer();
		}
		show_voltage();
	}
}
//*********************************************************************************************************************************************//
void clean_buffer()
{
	buffer[0] = 0;
	buffer_size = 0;
	buffer_complete = FALSE;
}
//*********************************************************************************************************************************************//
ISR(USART_RXC_vect)
{
	if(buffer_complete == TRUE) return;
	
	uint8_t ch = UDR;
	if(ch == '\n')
	{
		buffer_complete = TRUE;
		return;
	}
	else if(ch == '\r')
	{
		buffer[buffer_size] = 0;
		buffer_complete = TRUE;
		return;
	}
	else
	{
		buffer[buffer_size++] = ch;
		return;
	}
	
	if(buffer_size == MAX_BUFFER_SIZE)
	{
		buffer[buffer_size] = 0;
		buffer_complete = TRUE;
		return;
	}
}
//*********************************************************************************************************************************************//
ISR(TIMER1_COMPA_vect)
{
	if(pause_flag == TRUE) return;
	UART_send(voltage[1] + '0');
	UART_send(voltage[2] + '0');
	UART_send(voltage[3] + '0');
	UART_send('.');
}
//******************************************************************************************//
