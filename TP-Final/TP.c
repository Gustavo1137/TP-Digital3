#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_uart.h"
#include <cr_section_macros.h>
//#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define CONTROL 1000
#define CONTROL1 1000
#define MODO_MANUAL 0
#define MODO_AUTO   1

volatile int valor = 0;
volatile int PWM1 = 0;
volatile int PWM2 = 0;
volatile uint16_t convertido1 = 0;
volatile uint16_t convertido2 = 0;
uint32_t BufferDMA[1024];
uint16_t BufferServo1[512];
uint16_t BufferServo2[512];
//uint32_t* PosicionesGuar1 = (uint32_t*) 0x20080000;//primera mitad del banco 1
//uint32_t* PosicionesGuar2 = (uint32_t*) 0x20082000;//segunda mitad del banco
volatile uint16_t totalPosiciones = 0;  // cuántas se guardaron
volatile uint16_t idxAuto = 0;          // cuál se reproduce ahora
volatile char rx_buf[16];
volatile uint8_t rx_idx = 0;
volatile uint8_t cmd_ready = 0;
volatile uint8_t modo = MODO_MANUAL;
volatile int angulo = 0;
volatile uint16_t angulo1 = 0;
volatile uint16_t angulo2 = 0;
volatile uint16_t i1 = 0;
volatile uint16_t i2 = 0;
volatile uint16_t t1 = 0;
volatile uint16_t t2 = 0;
volatile int index=0;
uint16_t adc=0;
uint8_t canal=0;
volatile uint32_t dato = 0;
volatile uint32_t valorUART = 0;
volatile uint32_t Estado = 1;
volatile uint32_t contador = 0;


GPDMA_LLI_T lli;


static int itoa_simple(int val, char* buf) {
    int i = 0;
    if (val == 0) { buf[i++] = '0'; buf[i] = '\0'; return i; }
    char tmp[10]; int t = 0;
    while (val > 0) { tmp[t++] = '0' + (val % 10); val /= 10; }
    while (t > 0) buf[i++] = tmp[--t];
    buf[i] = '\0';
    return i;
}


void configPIN(){
	//CONFIGURO EL PIN P0.23, PARA EL ADC
	PINSEL_CFG_T pinCfg;
	pinCfg.port = PORT_0;
	pinCfg.pin = PIN_23;
	pinCfg.func = PINSEL_FUNC_01;
	pinCfg.mode = PINSEL_PULLDOWN;
	pinCfg.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinCfg);

	//CONFIGURO EL PIN P0.26, PARA EL ADC
	PINSEL_CFG_T pinCfg2p;
	pinCfg2p.port = PORT_0;
	pinCfg2p.pin = PIN_26;
	pinCfg2p.func = PINSEL_FUNC_01;
	pinCfg2p.mode = PINSEL_PULLDOWN;
	pinCfg2p.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinCfg2p);

	//CONFIGURO EL PIN P0.0, PARA LA SEÑAL PWM, SERVO 1
	PINSEL_CFG_T pinCfg1; //SEÑAL PWM
	pinCfg1.port = PORT_0;
	pinCfg1.pin = PIN_0;
	pinCfg1.func = PINSEL_FUNC_00;
	pinCfg1.mode = PINSEL_TRISTATE;
	pinCfg1.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinCfg1);
	GPIO_SetDir(PORT_0, (1 << 0), GPIO_OUTPUT);

	//CONFIGURO EL PIN P0.1, PARA LA SEÑAL PWM, SERVO 2
	PINSEL_CFG_T pinCfg1p; //SEÑAL PWM
	pinCfg1p.port = PORT_0;
	pinCfg1p.pin = PIN_1;
	pinCfg1p.func = PINSEL_FUNC_00;
	pinCfg1p.mode = PINSEL_TRISTATE;
	pinCfg1p.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinCfg1p);
	GPIO_SetDir(PORT_0, (1 << 1), GPIO_OUTPUT);

	//CONFIGURACION DEL PIN P0.4, CONFIGURACION DEL PIN DE SALIDA DEL ELECTROIMAN
	PINSEL_CFG_T pinCfg2;
	pinCfg2.port = PORT_0;
	pinCfg2.pin = PIN_4;
	pinCfg2.func = PINSEL_FUNC_00;
	pinCfg2.mode = PINSEL_TRISTATE;
	pinCfg2.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinCfg2);
	GPIO_SetDir(PORT_0, (1 << 4), GPIO_OUTPUT);
	GPIO_ClearPins(PORT_0, 1 << 4);

	//CONFIGURAR INTERRUPCION EXTERNA EINT1
	PINSEL_CFG_T pinEINT0;
	pinEINT0.port = PORT_2;
	pinEINT0.pin = PIN_11;
	pinEINT0.func = PINSEL_FUNC_01;
	pinEINT0.mode = PINSEL_PULLDOWN;
	pinEINT0.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinEINT0);

	//CONFIGURAR INTERRUPCION EXTERNA EINT2
	PINSEL_CFG_T pinEINT1;
	pinEINT1.port = PORT_2;
	pinEINT1.pin = PIN_12;
	pinEINT1.func = PINSEL_FUNC_01;
	pinEINT1.mode = PINSEL_PULLDOWN;
	pinEINT1.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinEINT1);

	//CONFIGURACION DE PIN UART, EL TX
    /*PINSEL_CFG_T uartCfg;
	uartCfg.port = PORT_0;
	uartCfg.pin = PIN_2;
	uartCfg.func = PINSEL_FUNC_01;
	uartCfg.mode = PINSEL_TRISTATE;
	uartCfg.openDrain = DISABLE;
	PINSEL_ConfigPin(&uartCfg);
	GPIO_SetDir(PORT_0, (1 << 2), GPIO_OUTPUT);
	//GPIO_ClearPins(PORT_0, 1 << 2);
	//CONFIGURACION DE PIN UART, EL RX
	PINSEL_CFG_T uartCfg1;
	uartCfg1.port = PORT_0;
	uartCfg1.pin = PIN_3;
	uartCfg1.func = PINSEL_FUNC_01;
	uartCfg1.mode = PINSEL_TRISTATE;
	uartCfg1.openDrain = DISABLE;
	PINSEL_ConfigPin(&uartCfg1);
	GPIO_SetDir(PORT_0, (1 << 3), GPIO_INPUT);
	//GPIO_ClearPins(PORT_0, 1 << 3);*/
}

void configTimer(){
	//CONFIGURAR EL TIMER0
	TIM_TIMERCFG_T timerCfg;
	timerCfg.prescaleOpt = TIM_US;
	timerCfg.prescaleValue = 1;
	TIM_InitTimer(LPC_TIM0, &timerCfg);

	//CONFIGURAR EL TIMER1
	/*TIM_TIMERCFG_T timerCfg1;
	timerCfg1.prescaleOpt = TIM_US;
	timerCfg1.prescaleValue = 1;
	TIM_InitTimer(LPC_TIM1, &timerCfg1);*/

	//CONFIGURAR EL TIMER2
	TIM_TIMERCFG_T timerCfg1p;
	timerCfg1p.prescaleOpt = TIM_US;
	timerCfg1p.prescaleValue = 1;
	TIM_InitTimer(LPC_TIM2, &timerCfg1p);

	//CONFIGURAR EL MATCH0 DEL TIMER0
	TIM_MATCHCFG_T matchCfg;
	matchCfg.channel = TIM_MATCH_0;
	matchCfg.intEn = ENABLE;
	matchCfg.stopEn = DISABLE;
	matchCfg.resetEn = DISABLE;
	matchCfg.extOpt = TIM_NOTHING;
	matchCfg.matchValue = CONTROL;
	TIM_ConfigMatch(LPC_TIM0, &matchCfg);

	//CONFIGURAR EL MATCH1 DEL TIMER0
	TIM_MATCHCFG_T matchCfg1;
	matchCfg1.channel = TIM_MATCH_1;
	matchCfg1.intEn = ENABLE;
	matchCfg1.stopEn = DISABLE;
	matchCfg1.resetEn = ENABLE;
	matchCfg1.extOpt = TIM_NOTHING;
	matchCfg1.matchValue = 20000;
	TIM_ConfigMatch(LPC_TIM0, &matchCfg1);

	//CONFIGURAR EL MATCH0 DEL TIMER2
	TIM_MATCHCFG_T matchCfg1p;
	matchCfg1p.channel = TIM_MATCH_0;
	matchCfg1p.intEn = ENABLE;
	matchCfg1p.stopEn = DISABLE;
	matchCfg1p.resetEn = DISABLE;
	matchCfg1p.extOpt = TIM_NOTHING;
	matchCfg1p.matchValue = CONTROL1;
	TIM_ConfigMatch(LPC_TIM2, &matchCfg1p);

	//CONFIGURAR EL MATCH1 DEL TIMER2
	TIM_MATCHCFG_T matchCfg1s;
	matchCfg1s.channel = TIM_MATCH_1;
	matchCfg1s.intEn = ENABLE;
	matchCfg1s.stopEn = DISABLE;
	matchCfg1s.resetEn = ENABLE;
	matchCfg1s.extOpt = TIM_NOTHING;
	matchCfg1s.matchValue = 20000;
	TIM_ConfigMatch(LPC_TIM2, &matchCfg1s);

	GPIO_SetPins(PORT_0,(1<<0));
	GPIO_SetPins(PORT_0,(1<<1));

	//INTERRUPCIONES
	NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER2_IRQn);
    TIM_Enable(LPC_TIM0);
    TIM_Enable(LPC_TIM2);
}


void configADC(){
    ADC_Init(100000);

    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_3);

    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_ChannelEnable(ADC_CHANNEL_3);

    ADC_PowerUp();

    ADC_BurstEnable();
}

void confiEINT(){
	EXTI_Init();

	EXTI_CFG_T exti1;
	exti1.line = EXTI_EINT1;
	exti1.mode = EXTI_EDGE_SENSITIVE;
	exti1.polarity = EXTI_RISING_EDGE;

	EXTI_ConfigEnable(&exti1);

	EXTI_CFG_T exti2;
	exti2.line = EXTI_EINT2;
	exti2.mode = EXTI_EDGE_SENSITIVE;
	exti2.polarity = EXTI_RISING_EDGE;

	EXTI_Config(&exti2);
	EXTI_ConfigEnable(&exti2);

	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
}



void configUART(void) {

    // configurar P0.2 (TXD0) y P0.3 (RXD0)
    PINSEL_CFG_T pinCfg;

    pinCfg.port   = PORT_0;
    pinCfg.pin    = PIN_2;
    pinCfg.func   = PINSEL_FUNC_01;   // funcion UART0 TXD
    pinCfg.mode   = PINSEL_TRISTATE;
    pinCfg.openDrain = DISABLE;
    PINSEL_ConfigPin(&pinCfg);

    pinCfg.pin    = PIN_3;
    pinCfg.func   = PINSEL_FUNC_01;   // funcion UART0 RXD
    pinCfg.mode      = PINSEL_PULLUP;
    PINSEL_ConfigPin(&pinCfg);

    UART_CFG_T uartCfg;
    uartCfg.baudRate = 9600;
    uartCfg.parity   = UART_PARITY_NONE;
    uartCfg.dataBits  = UART_DBITS_8;
    uartCfg.stopBits  = UART_STOPBIT_1;
    UART_Init(UART0, &uartCfg);

    UART_FIFO_CFG_T fifoCfg;
    fifoCfg.resetRxBuf = ENABLE;
    fifoCfg.resetTxBuf = ENABLE;
    fifoCfg.dmaMode    = DISABLE;
    fifoCfg.level      = UART_FIFO_TRGLEV0;
    UART_FIFOConfig(UART0, &fifoCfg);

    UART_IntConfig(UART0, UART_INT_RBR, ENABLE);
    NVIC_EnableIRQ(UART0_IRQn);

    UART_TxEnable(UART0);
}

void enviarUART(void){

    char msg[40];
     int i = 0;
     const char* sufijo = ",M:MANUAL\r\n";

     msg[i++] = '$';
     msg[i++] = 'S'; msg[i++] = '1'; msg[i++] = ':';
     i += itoa_simple(angulo1, msg + i);
     msg[i++] = ',';
     msg[i++] = 'S'; msg[i++] = '2'; msg[i++] = ':';
     i += itoa_simple(angulo2, msg + i);

     if(Estado == 1){
    	 sufijo = ",M:MANUAL\r\n";
     }else{
    	 sufijo = ",M:AUTO\r\n";
     }
     const char* p = sufijo;
     while (*p) msg[i++] = *p++;
     msg[i] = '\0';

     UART_Send(UART0, (uint8_t*)msg, i, BLOCKING);
}



void configDMA(void){
	GPDMA_Init();
	GPDMA_Channel_CFG_T dmaCfg;
	dmaCfg.channelNum=0;
	dmaCfg.transferSize=1024;
	dmaCfg.type=GPDMA_P2M;
	dmaCfg.srcMemAddr=(uint32_t)&LPC_ADC->ADGDR;
	dmaCfg.dstMemAddr=(uint32_t)BufferDMA;
	dmaCfg.srcConn=GPDMA_ADC;
	dmaCfg.dstConn=0;
	dmaCfg.src.burst=GPDMA_BSIZE_1;
	dmaCfg.src.width=GPDMA_WORD;
	dmaCfg.src.increment=0;
	dmaCfg.dst.burst=GPDMA_BSIZE_1;
	dmaCfg.dst.width=GPDMA_WORD;
	dmaCfg.dst.increment=ENABLE;
	dmaCfg.intTC=DISABLE;
	dmaCfg.intErr=DISABLE;
	dmaCfg.linkedList=(uint32_t)&lli;
	GPDMA_SetupChannel(&dmaCfg);
	GPDMA_ChannelStart(GPDMA_CH_0);

}


void actualizarADC(void){
    for (int i = 0; i < 1024; i++){
        uint32_t dato = BufferDMA[i];

        if (dato & (1UL << 31))        {
            uint16_t adc   = (dato >> 4) & 0xFFF;
            uint8_t  canal = (dato >> 24) & 0x07;

            if (canal == 0)
                convertido1 = adc;
            else if (canal == 3)
                convertido2 = adc;
        }
    }
}




void actualizarServo1(void){
    PWM1 = 500 + ((uint32_t)convertido1 * 2000) / 4095;

    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_0, PWM1);
}

void actualizarServo2(void){
    PWM2 = 500 + ((uint32_t)convertido2 * 2000) / 4095;

    TIM_UpdateMatchValue(LPC_TIM2, TIM_MATCH_0, PWM2);
}


int main(void){
	BufferServo1[0] = 1000;

	BufferServo2[0] = 1000;


	lli.srcAddr = (uint32_t)&LPC_ADC->ADGDR;
	lli.dstAddr = (uint32_t)BufferDMA;
	lli.nextLLI = (uint32_t)&lli;
	lli.control = 1024 | (2 << 18) | (2 << 21) | (1 << 27);
	configPIN();
	confiEINT();
	configADC();
	configDMA();
	configUART();
	configTimer();
	while(1){
		if(Estado == 1){
		        // MODO MANUAL
		        actualizarADC();
		        actualizarServo1();
		        actualizarServo2();
		        angulo1 = ((uint32_t)(PWM1 - 500) * 180) / 2000;
		        angulo2 = ((uint32_t)(PWM2 - 500) * 180) / 2000;
		        enviarUART();

		    } else {
		        TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_0, BufferServo1[contador]);
		        TIM_UpdateMatchValue(LPC_TIM2, TIM_MATCH_0, BufferServo2[contador]);
		        //angulo1 = 90;
		        //angulo2 = 90;
		        contador++;
				if(contador >= index){
					contador = 0;
				}

				for(volatile int i = 0; i < 5000000; i++);
		    }

		    for(volatile int i = 0; i < 1000000; i++);
	}
}


void TIMER0_IRQHandler(void){
	    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){
	        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	        GPIO_ClearPins(PORT_0, (1 << 0));  // fin del pulso → LOW
	    }
	    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)){
	        TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
	        GPIO_SetPins(PORT_0, (1 << 0));    // inicio nuevo pulso → HIGH
	    }
}

void TIMER2_IRQHandler(void){
	    if(TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)){
	        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
	        GPIO_ClearPins(PORT_0, (1 << 1));  // fin del pulso → LOW
	    }
	    if(TIM_GetIntStatus(LPC_TIM2, TIM_MR1_INT)){
	        TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);
	        GPIO_SetPins(PORT_0, (1 << 1));    // inicio nuevo pulso → HIGH
	    }
}


void EINT1_IRQHandler(void){
	EXTI_ClearFlag(EXTI_EINT1);
	GPIO_TogglePins(PORT_0, 1 << 4);
	//enviarUART();
	//GPIO_TogglePins(PORT_0, 1 << 2);
}

void EINT2_IRQHandler(void){
	EXTI_ClearFlag(EXTI_EINT2);

	actualizarADC();
	actualizarServo1();
	actualizarServo2();

	//BufferServo1[index] = PWM1;
	//BufferServo2[index] = PWM2;

	//index++;

	if(index < 512){
	    BufferServo1[index] = PWM1;
	    BufferServo2[index] = PWM2;
	    index++;
	}

	/*EXTI_ClearFlag(EXTI_EINT2);

	GPIO_TogglePins(PORT_0, (1<<4));

	BufferServo1[index] = PWM1;
	BufferServo2[index] = PWM2;
    index++;
    if(index==512){
    	index=0;
    	//contador++;
    }*/
}

void UART0_IRQHandler(void){

    while(UART_CheckBusy(UART0) == RESET){

        if(!(LPC_UART0->LSR & (1<<0)))
            break;

        uint8_t byte = UART_ReceiveByte(UART0);

        if(byte == '1')
            Estado = 1;

        if(byte == '0')
            Estado = 0;
            contador = 0;
    }

	/*GPIO_TogglePins(PORT_0, (1<<4));
	uint8_t byte = UART_ReceiveByte(UART0);

	    if(byte == '1'){
	        Estado = 1;
	    }
	    else if(byte == '0'){
	        Estado = 0;
	    }*/
    /*NVIC_ClearPendingIRQ(UART0_IRQn);

	uint32_t valorUART = UART_ReceiveByte(UART0);

    if (valorUART == '1') {
    	Estado = 1;
    } else if (valorUART == '0') {
        Estado = 0;
    }*/
}


