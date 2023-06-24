#include <avr/io.h>
#define F_CPU 8000000UL //8MHz
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_BUFFER_SIZE 100
#define FALSE 0
#define TRUE 1

uint8_t receive = 0;
uint8_t rx_data = 0;
volatile uint8_t rx_flag = 0;

unsigned char buffer[MAX_BUFFER_SIZE] = { 0 };
unsigned char buffer_size = 0;
unsigned char buffer_complete = FALSE;

char s[MAX_BUFFER_SIZE] = {0x00}; // строка s;
unsigned char s_size = 0; // длина строки s;
unsigned char number_xor[MAX_BUFFER_SIZE] = {0x00}; // беззнаковое число для выполнения поразрядного исключающего ИЛИ;
char extra_string[MAX_BUFFER_SIZE] = {0x00}; // дописываемая строка;
char delete_string[MAX_BUFFER_SIZE] = {0x00}; // удаляемая строка;
unsigned char number_index[MAX_BUFFER_SIZE] = {0x00}; // беззнаковое число для вывода символа строки s с данным номером;

void UARTInit(void)
{
	UBRRH = 0;
	UBRRL = 16; //baud rate 28800
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0)|(1<<UPM1)|(1<<UPM0); //8 bit, 1 stop bit, odd parity
}

void UARTSend(uint8_t data)
{
	while(!(UCSRA & (1<<UDRE)));
	UDR = data;
}

void UARTSendString(char str[])
{
	register char i = 0;
	while(str[i])
		UARTSend(str[i++]);
}

unsigned char UARTGet()
{
	while(!rx_flag);
	rx_flag = 0;
	return rx_data;
}

void inter_init()
{
	GICR |= (1 << INT0) | (1 << INT1); // разрешение прерываний INT0 и INT1;
	MCUCR |= (1 << ISC01) | (1 << ISC11); // прерывание с 1 на 0;
	GIFR |= (1 << INT0) | (1 << INT1); // Предотвращение срабатывания INT0 и INT1 при включении прерываний;
}

//******************************************************************************************************//
void shift_string(char *str, int size)
{
	for(int i = 1; i < size; ++i)
	{
		str[i - 1] = str[i];
	}
	str[size - 1] = 0x00;
}
//******************************************************************************************************//
void process_xor(char s[], char y[])
{
	int y_len = strlen(y);
	if(y_len > 2)
	return;
	unsigned char v = 0;
	if(y_len == 2)
	{
		if(y[0] >= 'A' && y[0] <= 'F')
		v += (y[0] - 'A' + 10) * 16;
		else
		v += (y[0] - '0') * 16;
		if(y[1] >= 'A' && y[0] <= 'F')
		v += (y[1] - 'A' + 10);
		else
		v += (y[1] - '0');
	}
	else if(y_len == 1)
	{
		if(y[0] >= 'A' && y[0] <= 'F')
		v += (y[0] - 'A' + 10);
		else
		v += (y[0] - '0');
	}
	else
	{
		return;
	}
	for(int i = 0; i < s_size; ++i)
	{
		s[i] = (s[i] ^ v);
	}
}
//******************************************************************************************************//
void process_strcat(char s[], char e[])
{
	int e_size = strlen(e);
	if(e == 0) return;
	int shift = 0;
	if(s_size + e_size >= 100)
	{
		shift = (s_size + e_size) - 100;
		for(int i = e_size - shift; i < e_size; ++i)
		{
			e[i] = 0x00;
		}
		e_size -= shift;
	}
	strcat(s, e);
	s_size += e_size;
}
//******************************************************************************************************//
void process_delete(char s[], char d[])
{
	int len = strlen(d);
	char *p, *q;
	
	while ((p = strstr(s, d)))
	{
		q = p + len; // адрес следующего символа после подстроки;
		while ((*p++ = *q++)) {} // сдвигаем все символы строки влево;
	}
}
//******************************************************************************************************//
char process_index(char s[], char i[])
{
	int v = 0;
	int i_size = strlen(i);
	char sym; char h, l;
	
	if(i_size == 2)
	{
		v += (i[0] - '0') * 10;
		v += (i[1] - '0');
		
		if(v < s_size)
		return s[v];
		else
		return 0;
	}
	else if(i_size == 1)
	{
		v += (i[0] - '0');
		if(v < s_size)
		return s[v];
		else
		return 0;
	}
	else
	{
		return 0;
	}
}
//******************************************************************************************************//
void process(void)
{
	if(buffer[0] == '*') // инициализация строки s;
	{
		shift_string(buffer, buffer_size);
		buffer_size--;
		strcpy(s, buffer);
		s_size = buffer_size;
		
		UARTSendString(s);
		UARTSend('\r');
	}
	else if(buffer[0] == '^' && s_size > 0) // значение для xor;
	{
		shift_string(buffer, buffer_size);
		buffer_size--;
		strcpy(number_xor, buffer);
		
		process_xor(s, number_xor);
		UARTSendString(s);
		UARTSend('\r');
	}
	else if(buffer[0] == '+' && s_size > 0) // дописываемая строка;
	{
		shift_string(buffer, buffer_size);
		buffer_size--;
		strcpy(extra_string, buffer);
		
		process_strcat(s, extra_string);
		
		UARTSendString(s);
		UARTSend('\r');
		
	}
	else if(buffer[0] == '-' && s_size > 0)
	{
		shift_string(buffer, buffer_size);
		buffer_size--;
		strcpy(delete_string, buffer);
		
		process_delete(s, delete_string);
		
		UARTSendString(s);
		UARTSend('\r');
	}
	else if(buffer[0] == '?' && s_size > 0)
	{
		shift_string(buffer, buffer_size);
		buffer_size--;
		strcpy(number_index, buffer);
		
		char num = 0x00;
		num = process_index(s, number_index);
		
		char hex_num[5] = {'0', 'x', 0x00, 0x00, 0x00};
		char h, l;
		h = num / 16;
		l = num % 16;
		if(h < 10)
		{
			hex_num[2] = h + '0';
		}
		else if(h >= 10)
		{
			hex_num[2] = (h - 10) + 'A';
		}
		
		if(l < 10)
		{
			hex_num[3] = l + '0';
		}
		else if(l >= 10)
		{
			hex_num[3] = (l - 10) + 'A';
		}
		
		UARTSendString(hex_num);
		UARTSend('\r');
	}
}
//******************************************************************************************************//

int main(void)
{
	inter_init();
	sei();
	UARTInit();
	while(1)
	{
		if(buffer_complete == TRUE)
		{
			process();
			clean_buffer();
		}
	}
}

void clean_buffer()
{
	buffer[0] = 0;
	buffer_size = 0;
	buffer_complete = FALSE;
}

ISR(USART_RXC_vect)
{
	if(buffer_complete == TRUE)
		return;
	uint8_t ch = UDR;
	if(ch == '\n')
	{
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
		// UARTSend(ch);
		return;
	}
	
	if(buffer_size == MAX_BUFFER_SIZE)
	{
		buffer[buffer_size] = 0;
		buffer_complete = TRUE;
		return;
	}
}

ISR (INT0_vect)
{
	UARTSendString(s);
	UARTSend('\n');
}

ISR (INT1_vect)
{
	clean_buffer();
	s[0] = 0;
	s_size = 0;
}