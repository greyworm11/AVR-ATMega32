#define F_CPU 8000000
#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// режим работы;
#define LIGHTS 0
#define CHANGE 1

// изменяемый параметр;
#define HA_ON 1
#define BC_ON 0

unsigned char pwm_state = 0;
unsigned int adc_value; // значение с АЦП;
unsigned char mode; // режим работы;
unsigned char param; // какой параметр меняем;

unsigned char ba, bb, bc;
unsigned char ha, hb, hc;
unsigned char da, db, dc;
unsigned char pa, pb, pc;

unsigned char segcounter;

unsigned char HA, BC;

uint8_t seg_num[4] = {0b1, 0b10, 0b100, 0b1000}; // блок семисегментного индикатора;
//						0	  1     2    3     4     -
uint8_t ha_value[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x40};
//                      0    1		2	  3		4	  5		6	 7		8	 9		A	 B		C	  D		E	 F
uint8_t bc_value[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
//							h	  A.	b	C.
uint8_t params_value[] = {0x74, 0xF7, 0x7C, 0xB9};
// ************************************************************************* //
void ADC_init(void)
{
	ADCSRA |= (1 << ADEN) // Разрешение использования АЦП;
	|(1 << ADSC) // Запуск преобразования;
	|(1 << ADATE) // Непрерывный режим работы АЦП;
	|(1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) // Делитель 128 = 64 кГц;
	|(1 << ADIE); // Разрешение прерывания от АЦП;
	ADMUX |= (1 << ADLAR) | (1 << REFS1) | (1 << REFS0) | (1 << MUX2) | (1 << MUX0);
	//Внутренний Источник ОН 2,56в, ADLAR1 - в high младшие биты ,MUX2..0 - биты чтобы считывание шло с PORTA5;
}
// ************************************************************************* //
ISR (ADC_vect)
{
	adc_value = ADCH;
}
// ************************************************************************* //
ISR (INT0_vect)
{
	if(mode == LIGHTS)
		mode = CHANGE;
	else
		mode = LIGHTS;
}
// ************************************************************************* //
ISR (INT1_vect)
{
	if(param == HA_ON)
		param = BC_ON;
	else
		param = HA_ON;
}
// ************************************************************************* //
ISR(TIMER0_OVF_vect) // вывод на индикатор - прерывание таймера T0;
{
	TCNT0 = 0xDD;
	if(mode == CHANGE)
	{
		PORTA = seg_num[segcounter];
		if(segcounter == 3)
		{
			if(param == BC_ON)
			{
				PORTC = params_value[2]; // b;
			}
			else if(param == HA_ON)
			{
				PORTC = params_value[0]; // h;
			}
		}
		else if(segcounter == 2)
		{
			if(param == BC_ON)
			{
				PORTC = params_value[3]; // C. ;
			}
			else if(param == HA_ON)
			{
				PORTC = params_value[1]; // A. ;
			}
		}
		else if(segcounter == 1)
		{
			if(param == BC_ON)
			{
				if(BC < 16) // если одна цифра;
				{
					PORTC = 0; // пустая;
				}
				else // если 2 цифры;
				{
					PORTC = bc_value[(BC / 16)]; // старшая цифра;
				}
			}
			else if(param == HA_ON)
			{
				if(HA < 4) // если отрицательное;
				{
					PORTC = ha_value[5]; // - ;
				}
				else // если положительное;
				{
					PORTC = 0; // пустая;
				}
			}
		}
		else if(segcounter == 0)
		{
			if(param == BC_ON)
			{
				PORTC = bc_value[(BC % 16)];
			}
			else if(param == HA_ON)
			{
				if(HA < 4)
				{
					PORTC = ha_value[(4 - HA) % 5];
				}
				else
				{
					PORTC = ha_value[(HA - 4)];
				}
			}
		}
	}
	segcounter++;
	if(segcounter > 3) segcounter = 0;
}
// ************************************************************************* //
void port_init(void)
{
	DDRA = 0xDF; // 5 на вход, остальные на выход;
	PORTA |= (1 << PORTA5); // 1 на 5, подтягивающий;
	DDRB = 0xFF;
	DDRC = 0xFF; // вывод;
	DDRD |= (1 << PORTD4) | (1 << PORTD7); // ввод порт d, D4,D7 вывод для шима;
	PORTD |= (1 << PORTD2); // подтягивающий на прерывание int0 pd2;
	PORTD |= (1 << PORTD3); // подтягивающий на прерывание int1 pd3;
	
	ba = (ba << da) | (ba >> (8 - da));
	bb = (bb << db) | (bb >> (8 - db));
	bc = (bc << dc) | (bc >> (8 - dc));
	
	PORTA = ba;
	PORTB = bb;
	PORTC = bc;
	
}
// ************************************************************************* //
void params_init()
{
	HA = 1; BC = 3;
	ba = 1; bb = 5; bc = 3;
	ha = 1; hb = 2; hc = 4;
	pa = 2; pb = 3; pc = 4;
	da = 0; db = 1; dc = 0;
	
	adc_value = 0;
	mode = CHANGE;
	param = HA_ON;
	segcounter = 0;
}
// ************************************************************************* //
void lights()
{
	ba = (ba << (ha - 4)) | (ba >> (8 - (ha - 4)));
	bb = (bb << hb) | (bb >> (8 - hb));
	bc = (bc << hc) | (bc >> (8 - hc));
	
	_delay_ms(2000);
	PORTA = ba;
	_delay_ms(1000);
	PORTB = bb;
	_delay_ms(1000);
	PORTC = bc;
}
// ************************************************************************* //
void inter_init(void)
{
	GICR |= (1 << INT0) | (1 << INT1); // разрешение прерываний INT0 и INT1;
	MCUCR |= (1 << ISC01) | (1 << ISC11); // прерывание с 1 на 0;
	GIFR |= (1 << INT0) | (1 << INT1); // Предотвращение срабатывания INT0 и INT1 при включении прерываний;
}
// ************************************************************************* //
void PWM_init(void)
{
	TIMSK = (1 << TOIE0);
	TCCR0 = (1 << CS02) | (0 << CS01) | (1 << CS00);
	TCNT0 = 0xb2;
	
	// TCCR1B |= (1 << WGM12) | (1 << WGM10) | (1 << CS12) | (1 << CS11);

	TCCR2 |= (1 << CS21) | (1 << CS22) | (1 << WGM21) | (1 << WGM20) | (1 << COM21);
		// CS за частоту, WGM для режима ШИМ, COM21, 0 при совпадении 7;
	
	TCNT2=0x00; // значение таймера = 0;
	TCNT1L = 0x00;
	TCNT1H = 0x00;
	OCR2 = 0x00; // скважность шим (0 - это 0 %, а FF - 100 %);
	OCR1BH = 0x00;
	OCR1BL = 0x00;
	
	TCCR1A |= (2<<COM1A0)|(2<<COM1B0)|(1<<WGM10);
	TCCR1B |= (1<<WGM12)|(1<<CS10);
}
// ************************************************************************* //
void pwm(void)
{
	if(param == BC_ON) // меняем BC;
	{
		OCR2 = 0xFF * bc / 0xFF; // [0, 255];
	}
	else // меняем HA;
	
		OCR2 = 0xFF / 8 * (ha); // [-4, 4];
	}
	OCR1BL = 0xFF - OCR2;
}
// ************************************************************************* //
int main(void)
{
	params_init();
	ADC_init();
	inter_init();
	port_init();
	PWM_init();
	sei();
	
	while(1)
	{
		if(mode == LIGHTS)
		{
			lights();
		}
		else if(mode == CHANGE)
		{
			pwm();
			if(param == BC_ON)
			{
				bc = adc_value; // [0, 255];
			}
			else if(param == HA_ON)
			{
				ha = (adc_value / 31); // [0, 8]; чтобы в последующем получить значение ha нужно из него вычитать 4;
			}
			HA = ha;
			BC = bc;
		}
	}
}