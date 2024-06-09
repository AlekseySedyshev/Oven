//Oven Project
#include "stm8s.h"
//PD3 - Down, PD2 - Right		PC5 - LEFT,		PC6 - UP, 	PC7 -Ok
//PD4 - Beep,	PC3 - Heater	PC4 - Analog Temp IN
//PA3 - LED,	I2C  PB4- SCL, PB5 - SDA
//PD5-TX , PD6 - RX (USART)
//=================SETUP==========================
#define		SWITCH_TIME			200U		//100ms

#define		FILTR_TIME			50U			//50ms
#define		ADC_ZERO				100U		//100UL		=0 deg temp
#define		PWM_PERIOD			50U			//50*4 = 200

#define		ADC_PERIOD			100U		//Scan ADC every 100 ms
#define		LCD_PERIOD			500U		//Update LCD Every 0,1 sek

#define SW_L 	(GPIOC->IDR & 0x20)	//PC5 - Left
#define SW_U 	(GPIOC->IDR & 0x40)	//PC6 - UP
#define SW_O	(GPIOC->IDR & 0x80)	//PC7 - Ok
#define SW_R	(GPIOD->IDR & 0x4)	//PD2 - Right
#define SW_D	(GPIOD->IDR & 0x8)	//PD3 - Down

//==========LCD================
#define 	LCD_Addr 		0x78
#define   COMM    		0x80			//0b10000000
#define   DATE    		0xC0			//0b11000000
#define   BURST   		0x40			//0b01000000
//=============================

#define ADC_PIN			0x10	//PC4
#define LED_PIN 		0x8		//PA3
#define HEAT_PIN 		0x8		//PC3
#define BUZZ_PIN 		0x10	//PD4

#define BUZZ_ON			GPIOD->ODR |= BUZZ_PIN;
#define BUZZ_OFF		GPIOD->ODR &=(~BUZZ_PIN);

uint8_t const TIM4_PSCR=0x07;
uint8_t const TIM4_PERIOD=124;

bool power_stat=0;	//1 - Heater On, 0 - Heater Off
bool usart_stat=0;	//1 - On, 0 - Off
bool pid_stat=0;	  //1 - On, 0 - Off
uint8_t act_conf=0; // Active config
//----------INTERRUPT GPIO STATUS------------
volatile bool press[5]=					{0,0,0,0,0};		//When button pressed - "1"
volatile bool release[5]=				{0,0,0,0,0};		//When button released - "1", after action = 0
volatile uint16_t press_time=0;									//Pressed button Value time
volatile uint16_t idle_timer=0;	

unsigned int TimingDelay;												//mSec counter
uint16_t	sec_time,adc_time=ADC_PERIOD/4,lcd_time=LCD_PERIOD/4;

volatile uint16_t temp,adc_out,temp_old=0;
uint8_t j=0,pwm=0,max_power=100,dif=0;
uint16_t time=0;

uint8_t menu=0,pos=0,pos1=0;pos2=0,stage=0;

//============SETUP==========================
const uint32_t usart_speed[]= {9600,19200,57600,115200};
uint8_t speed=1;	        //USART SPEED = 19200;
uint16_t conf_temp1[5];//={150,150,100,50,50};
uint16_t conf_temp2[5];//={215,225,100,50,50};
uint8_t conf_time1[5];//={60,60,60,60,60};
uint8_t conf_time2[5];//={30,30,30,30,30};
uint8_t conf_pwm[5];//={100,100,100,100,100};

uint8_t time1,time2,st2_timer=0,st4_timer=0;
uint16_t temp1,temp2;
bool sec_flag=0,lcd_flag=0;

//=========Functions and Procedures==========

void TimingDelayDec(void) 													{					// 1ms interrupt

	if (TimingDelay != 0x00) {TimingDelay--;}
	if (sec_time) { sec_time--;}
		else 				{	sec_time = 1000;sec_flag=1;GPIOA->ODR ^=8; 
								if (stage >0) {time++;}
								if (stage==2 && st2_timer>0) {st2_timer--;}	
								if (stage==4 && st4_timer>0) {st4_timer--;}
									
									idle_timer++;
								}
	if (adc_time) { adc_time--;}
		else 				{	adc_time = ADC_PERIOD;
									ADC1->CR1 |=ADC1_CR1_ADON; 	//«апустили сканирование ADC
								}	
	if (lcd_time) { lcd_time--;}
		else 				{	lcd_time = LCD_PERIOD;
									lcd_flag=1; 	//«апустили сканирование ADC
								}								
								
								
		if (!SW_L || !SW_R || !SW_U || !SW_D || !SW_O ) 
		{	press_time++;}	//Pressed button
	else {press_time=0;}	
}
void DelayMs(int Delay_time) 												{// ms Timer

	TimingDelay=Delay_time;
  while(TimingDelay!= 0x00);
}
//------------------------------LCD ------------------------------
const unsigned char init_lcd[]= 										{
0xA8,0x3F,0xD3,0x00,0x40,0xA1,0xC8,0xDA,0x12,
0x81,0x7F,0xA4,0xA6,0xD5,0x80,0x8D,0x14,0xAF
};
const unsigned char font[]= 												{// Font 8*5 
0x00, 0x00, 0x00, 0x00, 0x00 ,   // sp  32
0x00, 0x00, 0x2f, 0x00, 0x00 ,   // !   33
0x00, 0x07, 0x00, 0x07, 0x00 ,   // "   34
0x14, 0x7f, 0x14, 0x7f, 0x14 ,   // #   35
0x24, 0x2a, 0x7f, 0x2a, 0x12 ,   // $   36
0xc4, 0xc8, 0x10, 0x26, 0x46 ,   // %   37
0x36, 0x49, 0x55, 0x22, 0x50 ,   // &   38
0x00, 0x05, 0x03, 0x00, 0x00 ,   // '   39
0x00, 0x1c, 0x22, 0x41, 0x00 ,   // (   40
0x00, 0x41, 0x22, 0x1c, 0x00 ,   // )   41
0x14, 0x08, 0x3E, 0x08, 0x14 ,   // *   42
0x08, 0x08, 0x3E, 0x08, 0x08 ,   // +   43
0x00, 0x00, 0x50, 0x30, 0x00 ,   // ,   44
0x10, 0x10, 0x10, 0x10, 0x10 ,   // -   45
0x00, 0x60, 0x60, 0x00, 0x00 ,   // .   46
0x20, 0x10, 0x08, 0x04, 0x02 ,   // /   47
0x3E, 0x51, 0x49, 0x45, 0x3E ,   // 0   48
0x00, 0x42, 0x7F, 0x40, 0x00 ,   // 1   49
0x42, 0x61, 0x51, 0x49, 0x46 ,   // 2   50
0x21, 0x41, 0x45, 0x4B, 0x31 ,   // 3   51
0x18, 0x14, 0x12, 0x7F, 0x10 ,   // 4   52
0x27, 0x45, 0x45, 0x45, 0x39 ,   // 5   53
0x3C, 0x4A, 0x49, 0x49, 0x30 ,   // 6   54
0x01, 0x71, 0x09, 0x05, 0x03 ,   // 7   55
0x36, 0x49, 0x49, 0x49, 0x36 ,   // 8   56
0x06, 0x49, 0x49, 0x29, 0x1E ,   // 9   57
0x00, 0x36, 0x36, 0x00, 0x00 ,   // :   58
0x00, 0x56, 0x36, 0x00, 0x00 ,   // ;   59
0x08, 0x14, 0x22, 0x41, 0x00 ,   // <   60
0x14, 0x14, 0x14, 0x14, 0x14 ,   // =   61
0x00, 0x41, 0x22, 0x14, 0x08 ,   // >   62
0x02, 0x01, 0x51, 0x09, 0x06 ,   // ?   63
0x32, 0x49, 0x59, 0x51, 0x3E ,   // @   64
0x7E, 0x11, 0x11, 0x11, 0x7E ,   // A   65
0x7F, 0x49, 0x49, 0x49, 0x36 ,   // B   66
0x3E, 0x41, 0x41, 0x41, 0x22 ,   // C   67
0x7F, 0x41, 0x41, 0x22, 0x1C ,   // D   68
0x7F, 0x49, 0x49, 0x49, 0x41 ,   // E   69
0x7F, 0x09, 0x09, 0x09, 0x01 ,   // F   70
0x3E, 0x41, 0x49, 0x49, 0x7A ,   // G   71
0x7F, 0x08, 0x08, 0x08, 0x7F ,   // H   72
0x00, 0x41, 0x7F, 0x41, 0x00 ,   // I   73
0x20, 0x40, 0x41, 0x3F, 0x01 ,   // J   74
0x7F, 0x08, 0x14, 0x22, 0x41 ,   // K   75
0x7F, 0x40, 0x40, 0x40, 0x40 ,   // L   76
0x7F, 0x02, 0x0C, 0x02, 0x7F ,   // M   77
0x7F, 0x04, 0x08, 0x10, 0x7F ,   // N   78
0x3E, 0x41, 0x41, 0x41, 0x3E ,   // O   79
0x7F, 0x09, 0x09, 0x09, 0x06 ,   // P   80
0x3E, 0x41, 0x51, 0x21, 0x5E ,   // Q   81
0x7F, 0x09, 0x19, 0x29, 0x46 ,   // R   82
0x46, 0x49, 0x49, 0x49, 0x31 ,   // S   83
0x01, 0x01, 0x7F, 0x01, 0x01 ,   // T   84
0x3F, 0x40, 0x40, 0x40, 0x3F ,   // U   85
0x1F, 0x20, 0x40, 0x20, 0x1F ,   // V   86
0x3F, 0x40, 0x38, 0x40, 0x3F ,   // W   87
0x63, 0x14, 0x08, 0x14, 0x63 ,   // X   88
0x07, 0x08, 0x70, 0x08, 0x07 ,   // Y   89
0x61, 0x51, 0x49, 0x45, 0x43 ,   // Z   90
0x00, 0x7F, 0x41, 0x41, 0x00 ,   // [   91
0x55, 0x2A, 0x55, 0x2A, 0x55 ,   // 55  92
0x00, 0x41, 0x41, 0x7F, 0x00 ,   // ]   93
0x04, 0x02, 0x01, 0x02, 0x04 ,   // ^   94
0x40, 0x40, 0x40, 0x40, 0x40 ,   // _   95
0x00, 0x01, 0x02, 0x04, 0x00 ,   // '   96
0x20, 0x54, 0x54, 0x54, 0x78 ,   // a   97
0x7F, 0x48, 0x44, 0x44, 0x38 ,   // b   98
0x38, 0x44, 0x44, 0x44, 0x20 ,   // c   99
0x38, 0x44, 0x44, 0x48, 0x7F ,   // d   100
0x38, 0x54, 0x54, 0x54, 0x18 ,   // e   101
0x08, 0x7E, 0x09, 0x01, 0x02 ,   // f   102
0x0C, 0x52, 0x52, 0x52, 0x3E ,   // g   103
0x7F, 0x08, 0x04, 0x04, 0x78 ,   // h   104
0x00, 0x44, 0x7D, 0x40, 0x00 ,   // i   105
0x20, 0x40, 0x44, 0x3D, 0x00 ,   // j   106
0x7F, 0x10, 0x28, 0x44, 0x00 ,   // k   107
0x00, 0x41, 0x7F, 0x40, 0x00 ,   // l   108
0x7C, 0x04, 0x18, 0x04, 0x78 ,   // m   109
0x7C, 0x08, 0x04, 0x04, 0x78 ,   // n   110
0x38, 0x44, 0x44, 0x44, 0x38 ,   // o   111
0x7C, 0x14, 0x14, 0x14, 0x08 ,   // p   112
0x08, 0x14, 0x14, 0x18, 0x7C ,   // q   113
0x7C, 0x08, 0x04, 0x04, 0x08 ,   // r   114
0x48, 0x54, 0x54, 0x54, 0x20 ,   // s   115
0x04, 0x3F, 0x44, 0x40, 0x20 ,   // t   116
0x3C, 0x40, 0x40, 0x20, 0x7C ,   // u   117
0x1C, 0x20, 0x40, 0x20, 0x1C ,   // v   118
0x3C, 0x40, 0x30, 0x40, 0x3C ,   // w   119
0x44, 0x28, 0x10, 0x28, 0x44 ,   // x   120
0x0C, 0x50, 0x50, 0x50, 0x3C ,   // y   121
0x44, 0x64, 0x54, 0x4C, 0x44 ,   // z   122
//
0x00, 0x08, 0x36, 0x41, 0x00 ,   //7B- {
0x00, 0x00, 0x7f, 0x00, 0x00 ,   //7C- |
0x00, 0x41, 0x36, 0x08, 0x00 ,   //7D- }
0x04, 0x02, 0x04, 0x08, 0x04 ,   //7E- ~
0x00, 0x00, 0x00, 0x00, 0x00 ,   //7F- 
0x00, 0x00, 0x00, 0x00, 0x00 ,   //80- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //81- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //82- В
0x00, 0x00, 0x00, 0x00, 0x00 ,   //83- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //84- Д
0x00, 0x00, 0x00, 0x00, 0x00 ,   //85- Е
0x00, 0x00, 0x00, 0x00, 0x00 ,   //86- Ж
0x00, 0x00, 0x00, 0x00, 0x00 ,   //87- З
0x00, 0x00, 0x00, 0x00, 0x00 ,   //88- А
0x00, 0x00, 0x00, 0x00, 0x00 ,   //89- Й
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8A- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8B- Л
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8C- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8D- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8E- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //8F- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //90- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //91- С
0x00, 0x00, 0x00, 0x00, 0x00 ,   //92- Т
0x00, 0x00, 0x00, 0x00, 0x00 ,   // 93- У
0x00, 0x00, 0x00, 0x00, 0x00 ,   //94- Ф
0x00, 0x00, 0x00, 0x00, 0x00 ,   //95- Х
0x00, 0x00, 0x00, 0x00, 0x00 ,   //96- Ц
0x00, 0x00, 0x00, 0x00, 0x00 ,   //97- Ч
0x00, 0x00, 0x00, 0x00, 0x00 ,   //98- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //99- Щ 
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9A- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9B- Ы
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9C- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9D- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9E- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //9F- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A0- †
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A1- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A2- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A3- ?
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A4- §
0x00, 0x00, 0x00, 0x00, 0x00 ,   //A5- ?
0x00, 0x00, 0x36, 0x00, 0x00 ,   //A6- ¶
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //A7- І
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //A8- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //A9- ©
 0x3E, 0x49, 0x49, 0x49, 0x22 ,   //AA- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //AB- Ђ
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //AC- ђ
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //AD- ≠
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //AE- Ѓ
 0x44, 0x45, 0x7C, 0x45, 0x44 ,   //AF- ?
 0x06, 0x09, 0x09, 0x06, 0x00 ,   //B0- ∞
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B1- ±
 0x00, 0x41, 0x7F, 0x41, 0x00 ,   //B2- ?
 0x00, 0x44, 0x7D, 0x40, 0x00 ,   //B3- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B4- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B5- µ
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B6- ґ
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B7- Ј
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B8- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //B9- ?
 0x38, 0x54, 0x44, 0x28, 0x00 ,   //BA- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //BB- ї
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //BC- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //BD- ?
 0x00, 0x00, 0x00, 0x00, 0x00 ,   //BE- ?
 0x4A, 0x48, 0x7A, 0x40, 0x40 ,   //BF- ?                             
    
 0x7E, 0x11, 0x11, 0x11, 0x7E , // A        /*192*/
 0x7F, 0x49, 0x49, 0x49, 0x31 , // ?
 0x7F, 0x49, 0x49, 0x49, 0x36 , // B
 0x7F, 0x01, 0x01, 0x01, 0x03 , // ?
 0x60, 0x3E, 0x21, 0x21, 0x7F , // ?
 0x7F, 0x49, 0x49, 0x49, 0x41 , // E
 0x63, 0x14, 0x7F, 0x14, 0x63 , // ?
 0x22, 0x49, 0x49, 0x49, 0x36 , // ?
 0x7F, 0x10, 0x08, 0x04, 0x7F , // ?
 0x7F, 0x10, 0x09, 0x04, 0x7F , // ?
 0x7F, 0x08, 0x14, 0x22, 0x41 , // K
 0x7C, 0x02, 0x01, 0x01, 0x7F , // ?
 0x7F, 0x02, 0x0C, 0x02, 0x7F , // M
 0x7F, 0x08, 0x08, 0x08, 0x7F , // H
 0x3E, 0x41, 0x41, 0x41, 0x3E , // O
 0x7F, 0x01, 0x01, 0x01, 0x7F , // ?
 0x7F, 0x09, 0x09, 0x09, 0x06 , // P
 0x3E, 0x41, 0x41, 0x41, 0x22 , // C
 0x01, 0x01, 0x7F, 0x01, 0x01 , // T
 0x07, 0x48, 0x48, 0x48, 0x3F , // ?
 0x1C, 0x22, 0x7F, 0x22, 0x1C , // ?
 0x63, 0x14, 0x08, 0x14, 0x63 , // X
 0x3F, 0x20, 0x20, 0x3F, 0x60 , // ?
 0x07, 0x08, 0x08, 0x08, 0x7F , // ?
 0x7F, 0x40, 0x7F, 0x40, 0x7F , // ?
 0x3F, 0x20, 0x3F, 0x20, 0x7F , // ?
 0x01, 0x7F, 0x48, 0x48, 0x30 , // ?
 0x7F, 0x48, 0x48, 0x30, 0x7F , // ?
 0x7F, 0x48, 0x48, 0x48, 0x30 , // ?
 0x22, 0x49, 0x49, 0x49, 0x3E , // ?
 0x7F, 0x08, 0x3E, 0x41, 0x3E , // ?
 0x46, 0x29, 0x19, 0x09, 0x7F , // ?
 0x20, 0x54, 0x54, 0x54, 0x78 , // a
 0x78, 0x54, 0x54, 0x54, 0x20 , // b
 0x7C, 0x54, 0x54, 0x54, 0x28 , // ?
 0x7C, 0x04, 0x04, 0x04, 0x00 , // ?
 0x60, 0x38, 0x24, 0x38, 0x60 , // ?
 0x38, 0x54, 0x54, 0x54, 0x18 , // e
 0x6C, 0x10, 0x7C, 0x10, 0x6C , // ?
 0x28, 0x44, 0x54, 0x54, 0x28 , // ?
 0x3C, 0x40, 0x40, 0x20, 0x7C , // ?
 0x3C, 0x40, 0x42, 0x20, 0x7C , // ?
 0x7C, 0x10, 0x10, 0x28, 0x44 , // ?
 0x60, 0x10, 0x08, 0x04, 0x7C , // ?
 0x7C, 0x08, 0x10, 0x08, 0x7C , // ?
 0x7C, 0x10, 0x10, 0x10, 0x7C , // ?
 0x38, 0x44, 0x44, 0x44, 0x38 , // o
 0x7C, 0x04, 0x04, 0x04, 0x7C , // ?
 0x7C, 0x14, 0x14, 0x14, 0x08 , // p
 0x38, 0x44, 0x44, 0x44, 0x20 , // c
 0x04, 0x04, 0x7C, 0x04, 0x04 , // ?
 0x0C, 0x50, 0x50, 0x50, 0x3C , // y
 0x18, 0x24, 0x7C, 0x24, 0x18 , // ?
 0x44, 0x28, 0x10, 0x28, 0x44 , // x
 0x3C, 0x20, 0x20, 0x3C, 0x60 , // ?
 0x0C, 0x10, 0x10, 0x10, 0x7C , // ?
 0x7C, 0x40, 0x7C, 0x40, 0x7C , // ?
 0x3C, 0x20, 0x3C, 0x20, 0x7C , // ?
 0x04, 0x7C, 0x48, 0x48, 0x30 , // ?
 0x7C, 0x48, 0x30, 0x00, 0x7C , // ?
 0x7C, 0x48, 0x48, 0x48, 0x30 , // ?
 0x28, 0x44, 0x54, 0x54, 0x38 , // ?
 0x7C, 0x10, 0x38, 0x44, 0x38 , // ?
 0x58, 0x24, 0x24, 0x24, 0x7C  //  ?
 };
void I2C_write(unsigned char byte_send)							{//WRITE
	 while(!(I2C->SR1 & I2C_SR1_TXE)){}; 
	 I2C->DR = byte_send;
	}
void I2C_Start(void)    														{//Start
	 while(I2C->SR3 & I2C_SR3_BUSY){}; //I2C_BUSY
	 I2C->CR2 |= I2C_CR2_START;
	 while(!(I2C->SR1 & I2C_SR1_SB)){}; // Start - Generated
	 I2C->DR = LCD_Addr;
	 while(!(I2C->SR1 & I2C_SR1_ADDR)){}; // ACK - Received
	 while(!(I2C->SR3 & I2C_SR3_MSL)){}; // Master mode
	 
}
void I2C_Stop(void) 																{//Stop
	 while(!(I2C->SR1 & I2C_SR1_TXE)){}; // TX buf Empty
	 while(!(I2C->SR1 & I2C_SR1_BTF)){}; // TX buf Empty
	  I2C->CR2 |= I2C_CR2_STOP;
	 while(I2C->SR3 & I2C_SR3_BUSY){};
 }
void CMD(unsigned char byte) 												{
 I2C_write(COMM);
 I2C_write(byte);
 }
void LCD_Init(void) 																{// INIT LCD T191
uint8_t i;
I2C_Start();
CMD(0xAE);
for (i=0;i<=sizeof(init_lcd);i++)
	{CMD(init_lcd[i]);}
I2C_Stop();
	
}
void LCD_Gotoxy ( uint8_t x, uint8_t y )						{
       I2C_Start();
				if (x>0xf) {CMD(0x10 |(x>>4));}
			 else {CMD(0x10);}
			 CMD(0x00 | (x&0xf));
			 CMD(0xb0 | y);  																							//Y - 0...7
       I2C_Stop();
    }
void LCD_mode(unsigned char mode)										{ 
      I2C_Start();
       if(mode > 0)
       {CMD(0xA7);}
      else
       {CMD(0xA6);}
       I2C_Stop();
      }
void LCD_Char(char flash_o, uint8_t mode)						{// Print Symbol
      	 unsigned char i,ch; 
         I2C_Start();
         I2C_write(BURST);
          for(i=0; i<5; i++)
      	  {
      	    ch = font[(flash_o - 32)*5+i];
      	     if(mode) { I2C_write(~ch); }
      			 else  	 { I2C_write(ch);}
      		 if(i == 4)
      			  {
      			    if(mode) { I2C_write(0xff); }
      				  else   {	 I2C_write(0x00); }
      			  }
      	   }
         I2C_Stop();
          }
void LCD_PrintStr(char *s, uint8_t mode)						{// Print String
          while (*s)
        {
          LCD_Char(*s, mode);
          s++;
        }
      }
void LCD_Clear(void)      													{// Clear LCD
    	 unsigned char k, kk;
	for(kk=0;kk<8;kk++) 
	{
		I2C_Start();
		CMD(0x20);
		CMD(0x00);
		I2C_write(BURST);
		for(k=0;k<128;k++) I2C_write(0x00);	//LSB ??????, MSB ?????
		I2C_Stop();
	}
}       
void LCD_ClearStr(uint8_t y, uint8_t qnt )					{
uint8_t q,k;
	LCD_Gotoxy ( 0, y );
for (q=y;q<(qnt+y);q++){
		I2C_Start();
		CMD(0x20);
		CMD(0x00);
		CMD(0x10);
	  CMD(0xb0|q);
		I2C_write(BURST);
		for(k=0;k<128;k++) I2C_write(0x00);
		I2C_Stop();
}	
}
void LCD_PrintDec(long value,uint8_t mode) 					{// Print Dec
	
	char i=1,d=0;
	unsigned char text[10];
	do 
  { 
    if (value >=10)  {
				d = value % 10; 																				// 
				text[i] = d + '0'; 																			// ASCII 
				value /= 10; 																						// 
			}
		else 
			{	text[i] = value + '0';
				value=0;
			}
 		i++;
  }
	while(value); 
	i--;			
	do
	{	LCD_Char(text[i], mode);
		i--;
	}
	while(i);
}
//==================font 16x12======================
const unsigned char  dig1216[]  = 									{//digits big font

 0,0,248,254,6,131,195,99,51,30,254,248,0,0,7,31,27,49,48,48,48,24,31,7,      // 0
 0,0,0,12,12,14,255,255,0,0,0,0,0,0,0,48,48,48,63,63,48,48,48,0,              // 1
 0,0,28,30,7,3,131,195,227,119,62,28,0,0,56,60,62,55,51,49,48,48,48,48,       // 2
 0,0,12,14,7,195,195,195,195,231,126,60,0,0,12,28,56,48,48,48,48,57,31,14,    // 3
 0,0,192,224,112,56,28,14,255,255,0,0,0,0,7,7,6,6,6,6,63,63,6,6,              // 4
 0,0,120,127,103,99,99,99,99,227,195,131,0,0,12,28,56,48,48,48,48,56,31,15,   // 5
 0,0,192,240,248,220,206,199,195,195,128,0,0,0,15,31,57,48,48,48,48,57,31,15, // 6
 0,0,3,3,3,3,3,195,243,63,15,3,0,0,0,0,48,60,15,3,0,0,0,0,                    // 7
 0,0,0,188,254,231,195,195,231,254,188,0,0,0,15,31,57,48,48,48,48,57,31,15,   // 8
 0,0,60,126,231,195,195,195,195,231,254,252,0,0,0,48,48,48,56,28,14,7,3,0,    // 9
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,56,56,0,0,0,0,													// .
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0															// Space
	};
void LCD_PrintBig(long value, char xx,char yy) 			{// Print Dec
	
    char i=1, d=0, j;
    unsigned char text[10];
		
		do 
  { 
    if (value >=10)  {
				d = value % 10; 																	// 
				text[i] = d; 																			//ASCII 
				value /= 10; 																			//
			}
		else 
			{	text[i] = value;
				value=0;
			}
 		i++;
  }
	while(value);  
i--;d=i;	
//--------------------------------------------------------------------
			I2C_Start();
			if (xx>0xf) {CMD(0x10 |(xx>>4));}
			 else {CMD(0x10);}
			 CMD(0x00 | (xx&0xf));
			 CMD(0xb0 | yy);  																							//Y - 0...7
       I2C_Stop();
	do	{	
		I2C_Start();I2C_write(BURST);
    for(j=0; j<12; j++)   	{
      	I2C_write(dig1216[(text[i])*24+j]);
      }
    I2C_Stop();
		i--;
	}
	while(i);
			 I2C_Start();
				if (xx>0xf) {CMD(0x10 |(xx>>4));}
			 else {CMD(0x10);}
			 CMD(0x00 | (xx&0xf));
			 CMD(0xb0 | yy+1);  																							//Y - 0...7
       I2C_Stop();
	
	do	{
		 I2C_Start();I2C_write(BURST);
     for(j=12; j<24; j++) {
       I2C_write(dig1216[(text[d])*24+j]);
		 }
		 I2C_Stop();
		d--;
	}
	while(d);
	
}
void speed_cfg (void)																{
	UART1->CR2 &=(~UART1_CR2_REN)&(~UART1_CR2_TEN);		//RX\TX off
switch (speed)	{
	case 9600:   {UART1->BRR1 =0x68;UART1->BRR2 =0x03;} break;	//9600
	case 19200:  {UART1->BRR1 =0x34;UART1->BRR2 =0x01;} break;	//19200
	case 57600:  {UART1->BRR1 =0x11;UART1->BRR2 =0x06;} break;	//57600
	case 115200: {UART1->BRR1 =0x08;UART1->BRR2 =0x0B;} break;	//115200
	default:     {UART1->BRR1 =0x34;UART1->BRR2 =0x01;} break;	//19200
  }
	 UART1->CR2 |= UART1_CR2_REN | UART1_CR2_TEN; // RX\TX On
}
void SaveCFG (uint8_t cnf) 										      {   // Save EEPROM 
FLASH->DUKR=0xAE;															            //Enter First Unlock code
FLASH->DUKR=0x56;															            //Enter Second Unlock code
while(!(FLASH->IAPSR & FLASH_IAPSR_DUL)){};		            //waiting when unlock EEP_Write

if (cnf<5)                                            {   //Config 0...4 write
FLASH->CR2 |=FLASH_CR2_WPRG;                              //Word write
FLASH->NCR2 &=(~FLASH_NCR2_NWPRG);
*(NEAR uint8_t*)(0x004000+cnf*8) = conf_pwm[cnf] & 0xFF;			  //Write PWM Data
*(NEAR uint16_t*)(0x004001+cnf*8) = conf_temp1[cnf] & 0xFFFF;	  //Write Temp1
*(NEAR uint8_t*)(0x004003+cnf*8) = conf_time1[cnf] & 0xFF;			//Write Time1
while((!FLASH->IAPSR & FLASH_IAPSR_EOP)){};		                //Waiting while writing

FLASH->CR2 |=FLASH_CR2_WPRG;                                  //Word write
FLASH->NCR2 &=(~FLASH_NCR2_NWPRG);
*(NEAR uint16_t*)(0x004004+cnf*8) = conf_temp2[cnf] & 0xFFFF;   //Write Temp2_high
*(NEAR uint8_t*)(0x004006+cnf*8) = conf_time2[cnf] & 0xFF;			//Write Time2
*(NEAR uint8_t*)(0x004007+cnf*8) = 0xFF;			
while((!FLASH->IAPSR & FLASH_IAPSR_EOP)){};		                //Waiting while writing
}

else {
  FLASH->CR2 |=FLASH_CR2_WPRG;                                  //Word write
  FLASH->NCR2 &=(~FLASH_NCR2_NWPRG);
  *(NEAR uint8_t*)0x004030 = speed & 0xFF;			          //Write USART speed
  *(NEAR uint8_t*)0x004031 = usart_stat & 0xFF;			            //Write Max power
  *(NEAR uint8_t*)0x004032 = max_power & 0xFF;			            //Write Max power
  *(NEAR uint8_t*)0x004033 = pid_stat & 0xFF;			              //Write Max power
		while((!FLASH->IAPSR & FLASH_IAPSR_EOP)){};	
	
	*(NEAR uint8_t*)0x004040 = act_conf & 0xFF;			              //Active config
		while((!FLASH->IAPSR & FLASH_IAPSR_EOP)){};		                //Waiting while writing
}

FLASH->NCR2 |=FLASH_NCR2_NWPRG;
FLASH->IAPSR &=(~FLASH_IAPSR_DUL); 											//Write Protect
}
void LoadCFG (void) 										      			{   // Save EEPROM 
uint8_t cnf;
for (cnf=0;cnf<5;cnf++)                             {   //Config 0...4 read
	conf_pwm[cnf]		=	*(NEAR uint8_t*)(0x004000+cnf*8);			//Write PWM Data
	conf_temp1[cnf] = *(NEAR uint16_t*)(0x004001+cnf*8);	  //Write Temp1
	conf_time1[cnf]	=	*(NEAR uint8_t*)(0x004003+cnf*8);			//Write Time1
	conf_temp2[cnf]	=	*(NEAR uint16_t*)(0x004004+cnf*8);   //Write Temp2_high
	conf_time2[cnf]	=	*(NEAR uint8_t*)(0x004006+cnf*8);			//Write Time2
	}
speed				=  *(NEAR uint8_t*)0x004030;			     	     //Write USART speed
usart_stat	=  *(NEAR uint8_t*)0x004031 &0x1;			        //Write Max power
max_power 	=  *(NEAR uint8_t*)0x004032;			            //Write Max power
pid_stat		=  *(NEAR uint8_t*)0x004033 &0x1;		          //Write Max power
act_conf		=	 *(NEAR uint8_t*)0x004040;			              //Active config
}
void cfg	(uint8_t cnf)															{
LoadCFG();
pwm=		conf_pwm[cnf]		;			
temp1=	conf_temp1[cnf];	 
time1=	conf_time1[cnf];		
temp2=	conf_temp2[cnf];   
time2=	conf_time2[cnf];
speed_cfg();
time=0;
}

void initial(void)																	{//Init GPIO & Timer
		//-------------«апуск в режиме HSI------------------------
	  CLK->ICKR |= CLK_ICKR_HSIEN; //внутренний генератор на 16MHz
	  CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV); //делитель вырубить
		
///*
//------------ онфигурируем HSE -------------------------	
		CLK->SWCR |=CLK_SWCR_SWEN;
		CLK->SWR = 0xB4;
		CLK->ECKR |= CLK_ECKR_HSEEN;
	//-----------Ќастраиваем аварийное переключение на HSI---	
		CLK->CSSR |= CLK_CSSR_CSSEN;
		CLK->CSSR |= CLK_CSSR_CSSDIE; //прерывание включено 	
//*/
//=============TIM4============	
		TIM4->PSCR = TIM4_PSCR;
		TIM4->ARR = TIM4_PERIOD;
		TIM4->SR1 &=~TIM4_SR1_UIF; 	
		TIM4->IER	|= TIM4_IER_UIE; 
		TIM4->CR1 |= TIM4_CR1_CEN; // Start tim4
		_asm("rim");
 //===========TIM1=================		
		GPIOC->DDR |= HEAT_PIN; //Output Push pull
		GPIOC->CR1 |= HEAT_PIN;
		GPIOC->CR2 |= HEAT_PIN; 
	//================200√ц=====================
		TIM1->PSCRH = 0x9C;	TIM1->PSCRL = 0x40;				// делим 8000000 на 40000 получаем 200 гц.
		TIM1->ARRH = 0x00;	TIM1->ARRL = (PWM_PERIOD*4);
		TIM1->CCR3H=0;			TIM1->CCR3L=0;					// 4 - 20mS
		
		TIM1->CNTRH=0;	TIM1->CNTRL=0;								//—брос счетчика
		TIM1->CCMR3 |= 0b110<<4 | TIM1_CCMR_OCxPE;		//------
		TIM1->CCER2 |= TIM1_CCER2_CC3E;	
		TIM1->CR1 |= TIM1_CR1_CMS;												//CMS = 11 UP and Down 
		TIM1->BKR |=TIM1_BKR_MOE;
		TIM1->CR1 |= TIM1_CR1_CEN; 											// запустить таймер
		TIM1->EGR |= TIM1_EGR_UG;
		//-----------BUTTONTS ----------
		GPIOC->DDR &=(~0xE0);GPIOC->CR1 |=0xE0;GPIOC->CR2 |=0xE0; //PC 5,6,7		Input,Floating,Interrupt
		GPIOD->DDR &=(~0x0C);GPIOD->CR1 |=0x0C;GPIOD->CR2 |=0x0C; //PD 2,3		 	Input,Floating,Interrupt
		
		_asm("sim");
		  EXTI->CR1 |= (0b11 << 4); //Rise\Fall interrupt GPIOC
			EXTI->CR1 |= (0b11 << 6); //Rise\Fall interrupt GPIOD
		_asm("rim");
			
//------------------LED--------------------		
    GPIOA->DDR |= LED_PIN;		//Output Push pull
		GPIOA->CR1 |= LED_PIN;
		GPIOA->CR2 |= LED_PIN; 	
//------------------BUZZER-----------------		
		GPIOD->DDR |= BUZZ_PIN;		//Output Push pull
		GPIOD->CR1 |= BUZZ_PIN;
		GPIOD->CR2 |= BUZZ_PIN; GPIOD->ODR &=(~BUZZ_PIN);	

//--------------------ADC------------------
		GPIOC->DDR &=(~ADC_PIN);
		GPIOC->CR1 &=(~ADC_PIN);
		GPIOC->CR2 &=(~ADC_PIN);
		ADC1->CSR |= 0x02; 									//2й канал
		ADC1->CSR |= ADC1_CSR_EOCIE; 				//ѕрерывание по окончанию конвертации.
		ADC1->CR1 |= 0x40; 									//Fadc = Fmaster/8 = 2MHz
		ADC1->CR2 = 0x08; 									// Right alignement
		ADC1->TDRL =0x4; 										//отключить триггер шмидта на 2м канале	
//-------I2C ENABLE----------------
		GPIOB->DDR |=0x30;
		GPIOB->CR1 &=(~0x30);
		GPIOB->CR2 |=0x30;
		
		I2C->FREQR 	= 16;  					//16MHz	
		I2C->CCRL		=	80; 					//100 kHz		
		I2C->CCRH		=	0;
		I2C->TRISER	=	17;
		I2C->CR1 |= I2C_CR1_PE;

		//--------------------USART--------------------
		GPIOD->DDR &=(~0x40)| 0x20; //PD5 - Tx, PD6 - RX
		GPIOD->CR1 |=  0x60; 				//Pull Up & push_pull
		GPIOD->CR2 &=(~0x40) | 0x20; 			//Without IRQ & High Speed
		
		UART1->CR1 &=(~0xff);UART1->CR2 &=(~0xff);
		UART1->CR3 &=(~0xff);UART1->CR4 &=(~0xff);
		speed_cfg();
		//UART1->CR2 |= UART1_CR2_REN | UART1_CR2_TEN; // включение передачи и приема
}
void lcd_update(void)																{
uint8_t y,pos_flag=(1<<pos);  //меню первого экрана 
uint8_t pos1_flag=(1<<pos1); //меню второго экрана
uint8_t pos2_flag=(1<<pos2);	// меню настроек

if(!stage){
	
	switch (menu) {
	case 0: {					//menu=0 - ¬ыбор профил€
		for(y=0;y<5;y++) {	
			LCD_Gotoxy (1,y+1);LCD_PrintDec(y+1,(pos_flag & 1<<y));LCD_PrintStr(" ",(pos_flag & 1<<(y))); LCD_PrintDec(conf_pwm[y]*2,(pos_flag & 1<<y));
			LCD_PrintStr(" ",(pos_flag & 1<<y));
			LCD_PrintDec(conf_temp1[y],(pos_flag & 1<<y));LCD_PrintStr("/",(pos_flag & 1<<y));LCD_PrintDec(conf_time1[y],(pos_flag & 1<<y));
			LCD_PrintStr(", ",(pos_flag & 1<<y));LCD_PrintDec(conf_temp2[y],(pos_flag & 1<<y));
			LCD_PrintStr("/",(pos_flag & 1<<y));LCD_PrintDec(conf_time2[y],(pos_flag & 1<<y));
		}
		LCD_Gotoxy (1,6);LCD_PrintStr("Settings",(pos_flag & 1<<5));
	}	break;          //menu=0
	case 1:	{					//menu=1	Ќастройки профил€
	LCD_Gotoxy (1,0);LCD_PrintStr("Start",(pos1_flag & 1<<0));LCD_Gotoxy (64,0);LCD_PrintStr("Config #",0);LCD_PrintDec(pos,0);
	LCD_Gotoxy (1,1);LCD_PrintStr("t1= ",(pos1_flag & 1<<1));LCD_PrintDec(conf_temp1[pos],(pos1_flag & 1<<1));LCD_PrintStr(" ∞C  ",(pos1_flag & 1<<1));
	LCD_Gotoxy (1,2);LCD_PrintStr("S1= ",(pos1_flag & 1<<2));LCD_PrintDec(conf_time1[pos],(pos1_flag & 1<<2));LCD_PrintStr("s. ",(pos1_flag & 1<<2));
	LCD_Gotoxy (1,3);LCD_PrintStr("t2= ",(pos1_flag & 1<<3));LCD_PrintDec(conf_temp2[pos],(pos1_flag & 1<<3));LCD_PrintStr(" ∞C  ",(pos1_flag & 1<<3));
	LCD_Gotoxy (1,4);LCD_PrintStr("S2= ",(pos1_flag & 1<<4));LCD_PrintDec(conf_time2[pos],(pos1_flag & 1<<4));LCD_PrintStr("s. ",(pos1_flag & 1<<4));
	LCD_Gotoxy (1,5);LCD_PrintStr("P=  ",(pos1_flag & 1<<5));LCD_PrintDec(conf_pwm[pos]*2,(pos1_flag & 1<<5));LCD_PrintStr(" %  ",(pos1_flag & 1<<5));
	LCD_Gotoxy (1,6);LCD_PrintStr("<-Exit<- | ->Save->",(pos1_flag & 1<<6));
	} break; 
	case 2:	{					//menu=1	Ќастройки параметров
	LCD_Gotoxy (1,0);LCD_PrintStr("USART",(pos2_flag & 1<<0));
	if (!usart_stat){ LCD_PrintStr(" - Off ",0);}
	else            {	LCD_PrintStr(" - On  ",0);}
	LCD_Gotoxy (1,1);LCD_PrintStr("Speed",(pos2_flag & 1<<1));LCD_PrintStr(" = ",0);LCD_PrintDec(usart_speed[speed],(pos1_flag & 1<<1));LCD_PrintStr("    ",0);
	LCD_Gotoxy (1,2);LCD_PrintStr("Max power",(pos2_flag & 1<<2));LCD_PrintStr(" = ",0);LCD_PrintDec(max_power*2,0);LCD_PrintStr(" %    ",0);
	LCD_Gotoxy (1,3);LCD_PrintStr("PID",(pos2_flag & 1<<3));
	if (!pid_stat)  {LCD_PrintStr(" - Off ",0);}
	else            {LCD_PrintStr(" - On  ",0);}
	LCD_Gotoxy (1,4);LCD_PrintStr("<-Exit<- | ->Save->",(pos2_flag & 1<<4));
	
	LCD_Gotoxy (1,5);LCD_PrintStr("---------------------",0);
	LCD_Gotoxy (1,6);LCD_PrintStr("ADC: ",0); LCD_PrintDec(adc_out,0);
  if (adc_out<100 && adc_out>=10) {LCD_PrintStr("  ",0);}
  if (adc_out<1000 && adc_out>=100) {LCD_PrintStr(" ",0);}
	} break; 
	default: break;
}

LCD_Gotoxy (1,7);LCD_PrintStr("t= ",0);LCD_PrintDec(temp,0);LCD_PrintStr(" ∞C",0);
if (temp<10 && temp>=0)   {LCD_PrintStr("  ",0);}
if (temp<100 && temp>=10) {LCD_PrintStr(" ",0);}
LCD_Gotoxy (60,7);LCD_PrintStr("Time ",0);LCD_PrintDec(time,0);LCD_PrintStr(" s",0);
if (temp<10 && time>=0)   {LCD_PrintStr("  ",0);}
if (temp<100 && time>=10) {LCD_PrintStr(" ",0);}
}

else {	//stage >0
LCD_Gotoxy (20,0);LCD_PrintStr("Config #",0);LCD_PrintDec(pos,0);
LCD_Gotoxy (100,0);
if (TIM1->CCR3L > 0) {LCD_PrintStr("On",1);LCD_PrintStr(" ",0);}
	else {LCD_PrintStr("Off",1);}
//LCD_ClearStr(1, 1 );
LCD_Gotoxy (1,2);LCD_PrintStr("t =",0);LCD_PrintBig(temp,30,1); LCD_PrintStr(" ∞C  " ,0);
LCD_Gotoxy (1,4);LCD_PrintStr("s =",0);LCD_PrintBig(time,30,3); LCD_PrintStr(" s    " ,0);
LCD_Gotoxy (1,6);LCD_PrintStr("dt= ",0); LCD_PrintDec(dif,0); LCD_PrintStr(" ∞C/s ",0);
LCD_Gotoxy (66,6);
if (stage==2) {LCD_PrintStr("S ",0);LCD_PrintDec(st2_timer,0);}
if (stage==4) {LCD_PrintStr("S ",0);LCD_PrintDec(st4_timer,0);}
if (stage!=2 || stage!=4) {LCD_PrintStr("    ",0);}
LCD_Gotoxy (1,7);
switch (stage) { 
  //case 0:	{LCD_PrintStr("   ¬ыключено   ",0);} break;
  case 1:	{LCD_PrintStr("     PreHeat     ",0);} break;
  case 2:	{LCD_PrintStr("      Soak       ",0);} break;
  case 3:	{LCD_PrintStr("     Reflow      ",0);} break;
  case 4:	{LCD_PrintStr(" Top temp. pause ",0);} break;
  case 5:	{LCD_PrintStr("     CoolDown    ",0);} break;
  default: {} break;
  }
}
}
void action (void)																	{

if (!power_stat) { TIM1->CCR3L = 0;}

switch (stage) {
		case 1: {	if (temp<temp1) {TIM1->CCR3L = pwm*4;}
								else {TIM1->CCR3L = 0; stage=2;st2_timer=time1;
											BUZZ_ON;DelayMs(10);BUZZ_OFF;
								}
				}
		break;
		case 2: {	if(st2_timer) {
									if (temp<temp1) {TIM1->CCR3L = pwm*4;}
									if (temp>temp1) {TIM1->CCR3L = 0;}
								}
						else {	TIM1->CCR3L = pwm*4;stage=3;}
					}
		break;
		case 3: {			if (temp<temp2) {TIM1->CCR3L = pwm*4;}
									else {TIM1->CCR3L = 0;stage=4;st4_timer=time2;
												BUZZ_ON;DelayMs(10);BUZZ_OFF;}
						}
		break;
		case 4: {	if(st4_timer) {
									if (temp<temp2) {TIM1->CCR3L = pwm*4;}
									if (temp>temp2) {TIM1->CCR3L = 0;}
								}
						else {	TIM1->CCR3L = 0;stage=5;
									BUZZ_ON;DelayMs(50);BUZZ_OFF;DelayMs(50);
									BUZZ_ON;DelayMs(50);BUZZ_OFF;DelayMs(50);
									BUZZ_ON;DelayMs(50);BUZZ_OFF;DelayMs(50);
						}
					}
		break;
		
		case 5: {			TIM1->CCR3L = 0;
									if (temp<101) {stage=0;LCD_Clear();
									BUZZ_ON;DelayMs(500);BUZZ_OFF;}
						}
		break;
	default: {} break;
		

}
}
void kbd_scan (void)																{//Activity 
	if (press[2] && press_time>FILTR_TIME)	{	//Ok 
			press_time=0;lcd_flag=1;
			if (!menu && press[2]) { press[2]=0;
							if (pos==5) {menu=2;pos2=0;LCD_Clear();}		//settings
							else { menu=1;pos1=0;LCD_Clear();}					//config adjust
							}	
			if (menu==1 && !pos1 && press[2] && !stage) {press[2]=0; stage=1;LCD_Clear();cfg(pos);}	
	}	
	if (press[1] && press_time>FILTR_TIME)	{	//Up
			press_time=0;	press[1]=0;lcd_flag=1;
		switch (menu) {
			case 0: {	if(pos>0)		pos--;} 	break;
			case 1: {	if(pos1>0)	pos1--;} 	break;
			case 2: {	if(pos2>0)	pos2--;} 	break;
			default: break; 
		}
	}		
	if (press[4] && press_time>FILTR_TIME ) {	//Down
			press_time=0;	press[4]=0;lcd_flag=1;
			switch (menu) {
			case 0: {	if(pos<5)	pos++;	}		break;
			case 1: {	if(pos1<6)	pos1++;	}	break;
			case 2: {	if(pos2<4)	pos2++;	}	break;
			default: break; 
		}	
	}
	if (press[0] && press_time>FILTR_TIME) 	{	//LEFT
			press_time=0;lcd_flag=1;
			//press[0]=0;
			if (!menu)	{menu=0;stage=0;}	
			if (menu==1){
					switch (pos1) {
						case 0: {menu=0;stage=0;LCD_Clear();}	break;
						case 1: {if (conf_temp1[pos]>0) conf_temp1[pos]--;}	break;
						case 2: {if (conf_time1[pos]>0) conf_time1[pos]--;}	break;
						case 3: {if (conf_temp2[pos]>0) conf_temp2[pos]--;}	break;
						case 4: {if (conf_time2[pos]>0) conf_time2[pos]--;}	break;
						case 5: {if (conf_pwm[pos]*2>0) conf_pwm[pos]--;}	break;
						case 6: {menu=0;LCD_Clear();}	break;
							default: 	break;
					}
				}	
			if (menu==2){
						switch (pos2) {
						case 0: {usart_stat=0;}	break;
						case 1: {if (speed>0) speed--;}	break;
						case 2: {if (max_power*2>0) max_power--;}	break;
						case 3: {pid_stat=0;}	break;
						case 4: {menu=0; LCD_Clear();}	break;
						default: 	break;
						}	
			
			}	
				
			}
	if (press[3] && press_time>FILTR_TIME) 	{	//Right
		press_time=0;lcd_flag=1;
		//press[3]=0;
			if (!menu && press[3] && !stage) { 	press[3]=0;
										if (pos==5) {menu=2;pos2=0;LCD_Clear();}		//settings
										else { menu=1;pos1=0;LCD_Clear();	}				//config adjust
									}																						// End of menu 0
			if (menu==1 && press[3]) {		
					switch (pos1) {
						case 0: {stage=1;LCD_Clear();cfg(pos);}	break;
						case 1: {if (conf_temp1[pos]<=170) conf_temp1[pos]++;}	break;
						case 2: {if (conf_time1[pos]<=90) conf_time1[pos]++;}	break;
						case 3: {if (conf_temp2[pos]<=300) conf_temp2[pos]++;}	break;
						case 4: {if (conf_time2[pos]<=60) conf_time2[pos]++;}	break;
						case 5: {if (conf_pwm[pos]*2<max_power*2) conf_pwm[pos]++;}	break;
						case 6: {SaveCFG(pos);}	break;
							default: 	break;
					}
				}	
			if (menu==2 && press[3]) {
					switch (pos2) {
						case 0: {usart_stat=1;}	break;
						case 1: {if (speed<3) speed++;}	break;
						case 2: {if (max_power*2<100) max_power++;}	break;
						case 3: {pid_stat=1;}	break;
						case 4: {SaveCFG(pos);}	break;
						default: 	break;
						}	
					}
		
		}
	
if (idle_timer > 5)  { press_time=0;
		}
}
//==============USART OUT DATA=============
void print_char(char bb) 														{// ¬ывод символа в UART

  while(!(UART1->SR & UART1_SR_TXE)); //ожидаем освобождени€ UART
	UART1->DR = bb;
}
void print_str(char *str_,bool ln) 									{// ¬ывод строки в UART

  while (*str_) 
  {
    print_char(*str_);
    str_++;
   }
 if (ln) 
  {
    print_char(0x0D);
		print_char(0x0A);
  }
}
void UART_Printf(unsigned long value)								{// ¬ывод числа  в UART

	char i=1,d=0;
	unsigned char text[12];
	do 
  { 
    if (value >=10)
			{ d = value % 10; // остаток от делени€
				text[i] = d + '0'; // вз€ть ASCII символ и вставить в текст
				value /= 10; // "дес€тичный сдвиг вправо" -- деление на 10
			}
		else 
			{	text[i] = value + '0';
				value=0;
			}
 		i++;
  }
	while(value); 
	i--;			
	do
	{	print_char(text[i]);
		i--;
	}
	while(i);
}
void SendToUart(void)  															{// ќтправка даных в UART

	print_char(0x3A);																		//Start
	print_str(",",0);UART_Printf(time);						//Time
	print_str(",",0);UART_Printf(temp);						//Temp
	print_str(",",0);UART_Printf(dif);						//dif Temp
	print_str(",",0);UART_Printf(TIM1->CCR3L/2);					//Power
	print_str("",1);
}
 
//=====================-MAIN-=======================
int main(void) 		{
initial();
DelayMs(100);LCD_Init();
LCD_Clear();
LCD_Gotoxy (24,0);LCD_PrintStr(" Dawinch Oven ",1);
LoadCFG();

while(1){ //=========== main cycle=================

kbd_scan();
action();
if (sec_flag && stage>0) {
			if (temp<temp_old) {dif=temp_old-temp;}
			if (temp>=temp_old) {dif=temp-temp_old;}
			temp_old=temp;
			if(usart_stat) {SendToUart();}
			} 
if (lcd_flag) {lcd_update();}
lcd_flag=0;								//drop the lcd_flag
sec_flag=0;								//drop the sec_flag
} 												// end of mine cycle
} 

#ifdef USE_FULL_ASSERT

void assert_failed(u8* file, u32 line)
{ 
 //printf("Wrong parameters value: file %s on line %d\r\n", file, (int) line);	
  while (1)
  {
  }
}
#endif

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
