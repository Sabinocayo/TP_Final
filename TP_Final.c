/*
===============================================================================
 Name        : TP_Final.c
 Author      : $(CAYO Sabino....FERNANDEZ Juan Ignacio)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_systick.h"

#define LCD_D4     		23
#define LCD_D5     		24
#define LCD_D6     		25
#define LCD_D7     		26

#define LCD_RS     		0
#define LCD_RW     		1
#define LCD_EN     		2

#define BANIO  			4
#define PIEZA  			5
#define COCINA 	 		6
#define LIVING  		7

#define SENSOR_BANIO  	5
#define SENSOR_PIEZA 	6
#define SENSOR_COCINA  	7
#define SENSOR_LIVING  	8

#define LUZ_BANIO		6
#define LUZ_PIEZA		7
#define LUZ_COCINA		8
#define LUZ_LIVING		9
#define LED_ROJO		22

#define PIN_ALARMA  	0
#define ACTIVAR     	1
#define DESACTIVAR  	0

void Retardo(int);
void Mandar_Nibble(char);
void LCD_CmdWrite(char);
void LCD_DataWrite(char);
void Conf_LCD(void);
void Conf_TIMER(void);
void Conf_ADC(void);
void Conf_UART2(void);
void Conf_INTGPIO(void);
void Conf_EINT0(void);
void Conf_EINT1(void);
void Conf_LUCES(void);
void Conf_Alarma(void);
void Alarma(uint32_t tmp); 								// Equivalente al Conf_SysTick
void POWER_OFF(void);
void POWER_ON(void);
void UART_Recibir(void);
void Tocan_Timbre(void);

char Primera_Frase[] = {"Temperatura"};
char Cocina[] = {"Cocina: "};
char  Pieza[] = {"Pieza: "};
char Living[] = {"Living: "};

uint32_t Decena=0;						// Variables para la conversion del ADC a temperatura
uint32_t Unidad=0;						//
uint32_t Decimal=0;						//
uint32_t Temperatura=0;					//
uint32_t ADC_Value=0;					//
uint32_t Habitacion= PIEZA;           	// Sensa primero esta temperatura

uint8_t TOCAN_TIMBRE = 'Z';   			// Identificacion de cada Byte a transmitir
uint8_t ALARMA_FALLIDA = 'X';  			//
uint8_t ALARMA_EXITOSA = 'Y';   		//
uint8_t INTRUSO_EN_BANIO = 'P';			//
uint8_t INTRUSO_EN_PIEZA = 'Q';			//
uint8_t INTRUSO_EN_COCINA = 'R';		//
uint8_t INTRUSO_EN_LIVING = 'S';		//
uint8_t Password[4] ="1234";			// Clave de desbloqueo
uint8_t	Power=0;						// 0-> Apagado; 1->Encendido
uint8_t	Habilitada=0;					// 0-> Alarma desactivada ; 1-> Alarma Activada
uint8_t	Sonido=0;						// 0-> No suena ; 1-> Suena

int main(void) {
	SystemInit();
	static uint8_t	last_status = 0;
	Conf_Alarma();
	Conf_LUCES();
	Conf_TIMER();
	Conf_ADC();
	Conf_UART2();
	//Conf_INTGPIO();
	Conf_EINT0();
	//Conf_EINT1();                                   	// General ON/OFF Power
	Conf_LCD();

	//POWER_OFF();                                        // Power OFF
	while(1) {
		if(Sonido != last_status){
			last_status = Sonido;
			Alarma(Sonido);
		}
	}
    return 0 ;
}

void Conf_ADC(void){
	PINSEL_CFG_Type PinCfg;
		PinCfg.Portnum = 1;                            // AD0.4
		PinCfg.Pinnum  = 30;
		PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
		PinCfg.Funcnum = 3;
		PinCfg.OpenDrain = 0;
	PINSEL_ConfigPin(&PinCfg);
		PinCfg.Pinnum = 31;                            // AD0.5
	PINSEL_ConfigPin(&PinCfg);
		PinCfg.Portnum = 0;                            // AD0.6
		PinCfg.Pinnum = 3;
		PinCfg.Funcnum = 2;
	PINSEL_ConfigPin(&PinCfg);
		PinCfg.Pinnum = 2;                             // AD0.7
	PINSEL_ConfigPin(&PinCfg);

	ADC_Init(LPC_ADC,50000);     				       // Rate de 50KHz para inicializarlo
	ADC_BurstCmd(LPC_ADC, DISABLE);					   // Desactivamos modo BURST
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_5, ENABLE);	   // Habilitamos el canal 5
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);		   // Convierte cada ves que interrumpe el Match 01
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING); // Convierte en el flanco de subida del Match 01
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN4, ENABLE);      // Habilita interrupciones en canal 4 a 7
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN5, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN6, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN7, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADGINTEN, DISABLE);     // Desactivar la global int
	NVIC_EnableIRQ(ADC_IRQn);                          // Habilita interrupciones
	NVIC_SetPriority(ADC_IRQn,7);
	return;
}

void ADC_IRQHandler(void){
		if(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_4, ADC_DATA_DONE)==1){
			ADC_Value=ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_4);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_4, DISABLE);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_5, ENABLE);
			Habitacion = BANIO;
		}
		else if(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_5, ADC_DATA_DONE)==1){
			ADC_Value=ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_5);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_5, DISABLE);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_6, ENABLE);
			Habitacion = PIEZA;
		}
		else if(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_6, ADC_DATA_DONE)==1){
			ADC_Value=ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_6);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_6, DISABLE);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_7, ENABLE);
			Habitacion = COCINA;
		}
		else if(ADC_ChannelGetStatus(LPC_ADC,ADC_CHANNEL_7, ADC_DATA_DONE)==1){
			ADC_Value=ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_7);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_7, DISABLE);
			ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_4, ENABLE);
			Habitacion = LIVING;
		}
		// AJUSTAR ESTO A 3300 EN LA ENTREGA FINAL
		Temperatura= ADC_Value*1650;                    // Voltaje = (ADC_Value/4096)*3.3V
		Temperatura= Temperatura/4096;                  // T° = (Voltaje/10mV por °C)*10
		Decena=  Temperatura/100;
		Unidad=  (Temperatura%100)/10;
		Decimal= (Temperatura%100)%10;
		Conf_LCD();
	    return;
}

void Conf_TIMER(void){
	TIM_TIMERCFG_Type TIM_ConfigStruct;
		TIM_ConfigStruct.PrescaleOption = 0; 				// 0 ticks, 1 valor en uS
		TIM_ConfigStruct.PrescaleValue = 5000-1;            // 5000-1
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ConfigStructInit(TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	TIM_MATCHCFG_Type TIM_MatchConfigStruct;
		TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE; // o HIGH, LOW
		TIM_MatchConfigStruct.IntOnMatch = DISABLE;
		TIM_MatchConfigStruct.MatchChannel = 1;                // Match 0.1
		TIM_MatchConfigStruct.MatchValue = 30000-1;       // 300000-1
		TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
		TIM_MatchConfigStruct.StopOnMatch = DISABLE;
	TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
	TIM_Cmd(LPC_TIM0, ENABLE);
	return;
}

void Conf_UART2(void){
	PINSEL_CFG_Type PinCfg;                          	// P0.10 como TXD2
		PinCfg.Portnum = 0;
		PinCfg.Pinnum = 10;
		PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
		PinCfg.Funcnum = 1;
		PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&PinCfg);
		PinCfg.Pinnum = 11;                           	// P0.11 como RXD2
		PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;    	// Pines de RXD en este modo!
	PINSEL_ConfigPin(&PinCfg);

	UART_CFG_Type UART_ConfigStruct;
		UART_ConfigStruct.Baud_rate = 9600;
		UART_ConfigStruct.Parity = UART_PARITY_NONE;
		UART_ConfigStruct.Databits = UART_DATABIT_8;
		UART_ConfigStruct.Stopbits = UART_STOPBIT_1;
	UART_Init(LPC_UART2, &UART_ConfigStruct);
	UART_FIFO_CFG_Type FIFOCfg;
		FIFOCfg.FIFO_DMAMode = DISABLE;
		FIFOCfg.FIFO_Level = UART_FIFO_TRGLEV0;	     	// 0: 1 character, 8 bits
		FIFOCfg.FIFO_ResetRxBuf = ENABLE;
		FIFOCfg.FIFO_ResetTxBuf = ENABLE;
	UART_FIFOConfig(LPC_UART2, &FIFOCfg);
	UART_TxCmd(LPC_UART2, ENABLE);
	UART_IntConfig(LPC_UART2, UART_INTCFG_RBR, ENABLE); // Habilita interrupción por RX2
	UART_IntConfig(LPC_UART2, UART_INTCFG_RLS, ENABLE); // Habilita interrupción por estado de la linea
	NVIC_EnableIRQ(UART2_IRQn);
	return;
}

void Conf_INTGPIO(void){
	LPC_GPIO2->FIODIR &= ~((1<<SENSOR_BANIO)  | (1<<SENSOR_PIEZA));   	// Setea puertos como entrada
	LPC_GPIO2->FIODIR &= ~((1<<SENSOR_COCINA) | (1<<SENSOR_LIVING));
	LPC_GPIOINT->IO2IntEnF |= (1<<SENSOR_BANIO)  | (1<<SENSOR_PIEZA); 	// Interrupción por flanco de bajada
	LPC_GPIOINT->IO2IntEnF |= (1<<SENSOR_COCINA) | (1<<SENSOR_LIVING);
	LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_BANIO)  | (1<<SENSOR_PIEZA);
	LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_COCINA) | (1<<SENSOR_LIVING);
	NVIC_EnableIRQ(EINT3_IRQn);
}

void Conf_EINT0(void){
	LPC_GPIO2->FIODIR &=~(1<<10);                			// P2.10 Como entrada
	PINSEL_CFG_Type PinCfg;
		PinCfg.Portnum = 2;
		PinCfg.Pinnum = 10;
		PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
		PinCfg.Funcnum = 1;
		PinCfg.OpenDrain = 0;
	PINSEL_ConfigPin(&PinCfg);
	EXTI_Init(); // resetea todo
	EXTI_InitTypeDef EXTICfg;
		EXTICfg.EXTI_Line = EXTI_EINT0;
		EXTICfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
		EXTICfg.EXTI_polarity = 0;    						// 0 Falling Edge, 1 Rising Edge
	EXTI_Config(&EXTICfg);
	NVIC_EnableIRQ(EINT0_IRQn);
	EXTI_ClearEXTIFlag(EXTI_EINT0);
	return;
}

void Conf_EINT1(void){
	LPC_GPIO2->FIODIR &=~(1<<11);							// P2.11 Como entrada
	LPC_GPIO0->FIODIR |= (1<<LED_ROJO);                           // P0.22 LED Rojo
	LPC_GPIO0->FIOCLR |= (1<<LED_ROJO);
	PINSEL_CFG_Type PinCfg;
		PinCfg.Portnum = 2;
		PinCfg.Pinnum = 11;
		PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
		PinCfg.Funcnum = 1;
		PinCfg.OpenDrain = 0;
	PINSEL_ConfigPin(&PinCfg);
	EXTI_Init(); // resetea todo
	EXTI_InitTypeDef EXTICfg;
		EXTICfg.EXTI_Line = EXTI_EINT1;
		EXTICfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
		EXTICfg.EXTI_polarity = 0; 				  	 		// 0 Falling Edge, 1 Rising Edge
	EXTI_Config(&EXTICfg);
	NVIC_EnableIRQ(EINT1_IRQn);
	EXTI_ClearEXTIFlag(EXTI_EINT1);
	return;
}

void EINT0_IRQHandler(void){
	Retardo(100000);                                      	// Para evitar rebotes
	UART_SendByte(LPC_UART2, TOCAN_TIMBRE);
	EXTI_ClearEXTIFlag(EXTI_EINT0);
	return;
}

void EINT1_IRQHandler(void){
	Retardo(100000);
	if(Power){
		Power = 0;
		POWER_OFF();
		LPC_GPIO0->FIOCLR |= (1<<LED_ROJO);
	}
	else{
		Power++;
		POWER_ON();
		LPC_GPIO0->FIOSET |= (1<<LED_ROJO);
	}
	EXTI_ClearEXTIFlag(EXTI_EINT0);
	return;
}

void EINT3_IRQHandler(void){
	Retardo(100000);
	if(LPC_GPIOINT->IO2IntStatF & (1<<SENSOR_BANIO)){
		LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_BANIO);  	// Limpia la bandera de interrupción
		UART_SendByte(LPC_UART2, INTRUSO_EN_BANIO);     // Enviar sensor habitacion 1
	}
	if(LPC_GPIOINT->IO2IntStatF  & (1<<SENSOR_PIEZA)){
		LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_PIEZA);  	// Limpia la bandera de interrupción
		UART_SendByte(LPC_UART2, INTRUSO_EN_PIEZA);		// Enviar sensor habitacion 2
	}
	if(LPC_GPIOINT->IO2IntStatF  & (1<<SENSOR_COCINA)){
		LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_COCINA);  	// Limpia la bandera de interrupción
		UART_SendByte(LPC_UART2, INTRUSO_EN_COCINA);	// Enviar sensor habitacion 3
	}
	if(LPC_GPIOINT->IO2IntStatF  & (1<<SENSOR_LIVING)){
		LPC_GPIOINT->IO2IntClr |= (1<<SENSOR_LIVING);  	// Limpia la bandera de interrupción
		UART_SendByte(LPC_UART2, INTRUSO_EN_LIVING);	// Enviar sensor habitacion 4
	}
	return;
}

void Retardo(int cnt){
	uint32_t cuenta;
    for(cuenta=0;cuenta<cnt;cuenta++);
    return;
}

void Mandar_Nibble(char nibble){
    LPC_GPIO0->FIOCLR |= (1<<LCD_D4)|(1<<LCD_D5)|(1<<LCD_D6)|(1<<LCD_D7);   // Limpia dato anterior
    LPC_GPIO0->FIOPIN |= (((nibble>>0x00) & 0x0F) << LCD_D4);				// Manda un Nibble
    return;
}

void LCD_CmdWrite(char cmd){
    Mandar_Nibble((cmd >> 0x04) & 0x0F);  	// Manda el nibble mas significativo
    LPC_GPIO2->FIOCLR |= (1<<LCD_RS);  		// 0 en RS para modo Comando
    LPC_GPIO2->FIOCLR |= (1<<LCD_RW);  		// 0 en RW para modo Escritura
    LPC_GPIO2->FIOSET |= (1<<LCD_EN);  		// Manda un pulso ON/OFF en Enable para aceptar el nibble
    Retardo(2000);
    LPC_GPIO2->FIOCLR |= (1<<LCD_EN);
    Retardo(16000);

    Mandar_Nibble(cmd & 0x0F);            	// Manda el nibble menos significativo
    LPC_GPIO2->FIOCLR |= (1<<LCD_RS);
    LPC_GPIO2->FIOCLR |= (1<<LCD_RW);
    LPC_GPIO2->FIOSET |= (1<<LCD_EN);
    Retardo(2000);
    LPC_GPIO2->FIOCLR |= (1<<LCD_EN);
    Retardo(16000);
    return;
}

void LCD_DataWrite(char dat){
    Mandar_Nibble((dat >> 0x04) & 0x0F);  	// Manda el nibble mas significativo
    LPC_GPIO2->FIOSET |= (1<<LCD_RS);  		// 1 en RS para modo Datos
    LPC_GPIO2->FIOCLR |= (1<<LCD_RW);  		// 0 en RW para modo Escritura
    LPC_GPIO2->FIOSET |= (1<<LCD_EN);  		// Manda un pulso ON/OFF en Enable para aceptar el nibble
    Retardo(2000);
    LPC_GPIO2->FIOCLR |= (1<<LCD_EN);
    Retardo(16000);

    Mandar_Nibble(dat & 0x0F);            	// Manda el nibble menos significativo
    LPC_GPIO2->FIOSET |= (1<<LCD_RS);
    LPC_GPIO2->FIOCLR |= (1<<LCD_RW);
    LPC_GPIO2->FIOSET |= (1<<LCD_EN);
    Retardo(2000);
    LPC_GPIO2->FIOCLR |= (1<<LCD_EN);
    Retardo(16000);
    return;
}

void Conf_LCD(void){
	uint32_t i;
	LPC_GPIO0->FIODIR |= (1<<LCD_D4)|(1<<LCD_D5)|(1<<LCD_D6)|(1<<LCD_D7);     // Pines de salida
	LPC_GPIO2->FIODIR |= (1<<LCD_RS)|(1<<LCD_RW)|(1<<LCD_EN);                 // Pines de salida
	LCD_CmdWrite(0x02);                  	// Inicializa el LCD en modo 4 bits
	LCD_CmdWrite(0x28);                   	// Tamaño de caracter 5x7 cuadraditos
	LCD_CmdWrite(0x0E);                   	// Display OFF, Cursor ON
	LCD_CmdWrite(0x01);                   	// Limpia la pantalla
	LCD_CmdWrite(0x80);                   	// Lleva el cursor al inicio

	for(i=0;Primera_Frase[i]!=0;i++){		// "Temperatura"
		LCD_DataWrite(Primera_Frase[i]);
	}
	LCD_CmdWrite(0xC0);                   	// Salto de linea

	switch(Habitacion){
	case BANIO:          					// Temperatura del baño
		LCD_DataWrite('B');
		LCD_DataWrite('a');
		LCD_DataWrite((char)0xEE);			// ñ
		LCD_DataWrite('o');
		LCD_DataWrite(':');
		LCD_DataWrite(' ');
		break;
	case PIEZA:          			        // Temperatura de la pieza
		for(i=0;Pieza[i]!=0;i++){
			LCD_DataWrite(Pieza[i]);
		}
		break;
	case COCINA:       			         	// Temperatura de la cocina
		for(i=0;Cocina[i]!=0;i++){
			LCD_DataWrite(Cocina[i]);
		}
		break;
	case LIVING:          			        // Temperatura del living
		for(i=0;Living[i]!=0;i++){
			LCD_DataWrite(Living[i]);
		}
		break;
	}
    LCD_DataWrite((char)0x30+Decena);
    LCD_DataWrite((char)0x30+Unidad);
    LCD_DataWrite('.');
    LCD_DataWrite((char)0x30+Decimal);
    LCD_DataWrite((char)223);         			// Signo de grado
    LCD_DataWrite('C');
	return;
}

void Conf_Alarma(void){
	LPC_GPIO0->FIODIR |= (1<<PIN_ALARMA);       // P0.0 Salida al buzzer
	LPC_GPIO0->FIOCLR |= (1<<PIN_ALARMA);
	return;
}

void Conf_LUCES(void){
	LPC_GPIO0->FIODIR |= (1<<LUZ_BANIO);        // Pines como Salida
	LPC_GPIO0->FIODIR |= (1<<LUZ_PIEZA);
	LPC_GPIO0->FIODIR |= (1<<LUZ_COCINA);
	LPC_GPIO0->FIODIR |= (1<<LUZ_LIVING);
	LPC_GPIO0->FIOCLR |= (1<<LUZ_BANIO);        // Apago las luces
	LPC_GPIO0->FIOCLR |= (1<<LUZ_PIEZA);
	LPC_GPIO0->FIOCLR |= (1<<LUZ_COCINA);
	LPC_GPIO0->FIOCLR |= (1<<LUZ_LIVING);
	return;
}

void Alarma(uint32_t tmp){
	SYSTICK_InternalInit(100);
	SYSTICK_Cmd(tmp);         					// Activa/Desactiva el Systick
	SYSTICK_IntCmd(tmp);       					// Activa/Desactiva las interrupciones
	SYSTICK_ClearCounterFlag();
	return;
}

void SysTick_Handler(void){                    // Interrumpe cada 100mSeg
	static uint32_t Veces=0;
	if(Veces<5){
		LPC_GPIO0->FIOSET |= (1<<PIN_ALARMA);
	}
	else if(Veces<10){
		LPC_GPIO0->FIOCLR |= (1<<PIN_ALARMA);
	}
	Veces++;
	if(Veces==10){
		Veces=0;
	}
	SYSTICK_ClearCounterFlag();
	return;
}

void POWER_OFF(void){
	NVIC_DisableIRQ(EINT0_IRQn);
	NVIC_DisableIRQ(EINT3_IRQn);
	NVIC_DisableIRQ(UART2_IRQn);
	NVIC_DisableIRQ(SysTick_IRQn);
	NVIC_DisableIRQ(ADC_IRQn);
	Alarma(DESACTIVAR);
	return;
}

void POWER_ON(void){
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
	NVIC_EnableIRQ(UART2_IRQn);
	NVIC_EnableIRQ(ADC_IRQn);
	return;
}

void UART2_IRQHandler(void){
	uint32_t IntId=0;
	uint32_t Line_Status=0;
	IntId= UART_GetIntId(LPC_UART2);
	IntId &= (7<<1);                                 // Solo la ID del registro IIR
	if (IntId == (3<<1)){                            // bits 3:1 en (011) IntId Rx Line Status
		Line_Status = UART_GetLineStatus(LPC_UART2); // LSR (Line Status Register) completo
		Line_Status &= ((0xF<<1)|(1<<7));            // Andeado con todos los Errores
		if (Line_Status) {                           // Loop Infinito
			while(1){}
			}
		}
	if (IntId == (2<<1) || IntId == (6<<1)){         // Bits 3:1 en (010) IntId Rx Dato disponible
		                                             // Bits 3:1 en (110) IntId CTI (La FIFO contiene
		                                             // datos pero no hay actividad de recepcion en la FIFO)
		UART_Recibir();
	}
	return;
}

void UART_Recibir(void){
	uint8_t Dato_Recibido=0;
	static uint8_t Digito = 0;						 // Cantidad de digitos ingresados de la clave
	static uint8_t Correcto = 1; 					 // 0-> Cuando ingreso incorrectamente la clave
	Dato_Recibido = UART_ReceiveByte(LPC_UART2);

	if(Dato_Recibido>0x40 && Dato_Recibido<0x49){ 	 // A-H ( botones de ON-OFF luces)
		switch(Dato_Recibido){
			case 0x41:                    			 // A-> Prender luz del baño
				LPC_GPIO0->FIOSET |= (1<<LUZ_BANIO);
				break;
			case 0x42: 								 // B-> Apagar luz del baño
				LPC_GPIO0->FIOCLR |= (1<<LUZ_BANIO);
				break;
			case 0x43: 								 // C-> Prender luz de la pieza
				LPC_GPIO0->FIOSET |= (1<<LUZ_PIEZA);
				break;
			case 0x44: 								 // D-> Apagar luz de la pieza
				LPC_GPIO0->FIOCLR |= (1<<LUZ_PIEZA);
				break;
			case 0x45: 								 // E-> Prender luz de la cocina
				LPC_GPIO0->FIOSET |= (1<<LUZ_COCINA);
				break;
			case 0x46: 								 // F-> Apagar luz de la cocina
				LPC_GPIO0->FIOCLR |= (1<<LUZ_COCINA);
				break;
			case 0x47: 								 // G-> Prender luz del living
				LPC_GPIO0->FIOSET |= (1<<LUZ_LIVING);
				break;
			case 0x48: 								 // H-> Apagar luz del living
				LPC_GPIO0->FIOCLR |= (1<<LUZ_LIVING);
				break;
		}
	}
	if((Dato_Recibido>0x29 && Dato_Recibido<0x40)){	 // 0-9 (teclado)
		if(Dato_Recibido != Password[Digito]){
			Correcto = 0; 							 // Ingreso un digito incorrecto
		}
		Digito++;
		if(Digito == 4){
			Digito = 0;
			if(Correcto){
				UART_SendByte(LPC_UART2, ALARMA_EXITOSA);// Transmito "Contraseña correcta"
				Habilitada = !Habilitada;						 // Habilita-Deshabilita
				Sonido=0;
				Alarma(Sonido);
			}
			else{
				UART_SendByte(LPC_UART2, ALARMA_FALLIDA);// Transmito "Contraseña incorrecta"
				if(Habilitada){
					Sonido = 1;							 // Hacer sonar la alarma
				}
			}
			Correcto = 1;							  // Vuelvo a setear correcto
		}
	}
	return;
}
