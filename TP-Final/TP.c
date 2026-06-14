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
#include <stdint.h>
#include <string.h>         // ← NUEVO (para strcmp)
#include <stdio.h>          // ← NUEVO (para sprintf)

#define CONTROL 1000
#define CONTROL1 1000
#define MODO_MANUAL 0
#define MODO_AUTO   1

volatile int valor = 0;
volatile int PWM1 = 0;
volatile int PWM2 = 0;
volatile uint16_t convertido1 = 0;
volatile uint16_t convertido2 = 0;
uint32_t* Buffer = (uint32_t*) 0x2007C000;//banco 0
uint32_t* PosicionesGuar1 = (uint32_t*) 0x20080000;//primera mitad del banco 1
uint32_t* PosicionesGuar2 = (uint32_t*) 0x20082000;//segunda mitad del banco
volatile uint16_t totalPosiciones = 0;  // cuántas se guardaron
volatile uint16_t idxAuto = 0;          // cuál se reproduce ahora
volatile char rx_buf[16];
volatile uint8_t rx_idx = 0;
volatile uint8_t cmd_ready = 0;
volatile uint8_t modo = MODO_MANUAL;
volatile int angulo = 0;
volatile uint16_t angulo1 = 0;
volatile uint16_t angulo2 = 0;


/*static int int2str(char* buf, int val) {
    int i = 0;
    if(val < 0){ buf[i++] = '-'; val = -val; }
    if(val == 0){ buf[i++] = '0'; return i; }
    char tmp[10]; int t = 0;
    while(val > 0){ tmp[t++] = '0' + (val % 10); val /= 10; }
    while(t > 0){ buf[i++] = tmp[--t]; }
    return i;
}*/

void configPIN(void);
void confiEINT(void);
void configADC(void);
void configDAC(void);
void configDMA(void);
void configUART(void);
void enviarUART(void);
void configTimer(void);
void calcularPWMServo1(void);
void calcularPWMServo2(void);
void enviarUART(void);
void configGPIO_UART(void);

int main(){
	/*GPDMA_LLI_T lli;
	lli.srcAddr = (uint32_t)&LPC_ADC->ADGDR;
	lli.dstAddr = (uint32_t)&Buffer;
	lli.nextLLI = lli;
	lli.control = 4096 | (2 << 18) | (2 << 21) | (1 << 27);*/
	configPIN();
	confiEINT();
	configADC();
	configDAC();
	configDMA();
	configUART();
	//configGPIO_UART();
	configTimer();
	calcularPWMServo1();
	calcularPWMServo2();
	//char msg[] = "100";
	while(1){

	}
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

	//CONFIGURAR INTERRUPCION EXTERNA EINT0
	PINSEL_CFG_T pinEINT0;
	pinEINT0.port = PORT_2;
	pinEINT0.pin = PIN_10;
	pinEINT0.func = PINSEL_FUNC_01;
	pinEINT0.mode = PINSEL_PULLDOWN;
	pinEINT0.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinEINT0);

	//CONFIGURAR INTERRUPCION EXTERNA EINT1
	PINSEL_CFG_T pinEINT1;
	pinEINT1.port = PORT_2;
	pinEINT1.pin = PIN_11;
	pinEINT1.func = PINSEL_FUNC_01;
	pinEINT1.mode = PINSEL_PULLDOWN;
	pinEINT1.openDrain = DISABLE;
	PINSEL_ConfigPin(&pinEINT1);

	//CONFIGURACION DE PIN UART, EL TX
    PINSEL_CFG_T uartCfg;
	uartCfg.port = PORT_0;
	uartCfg.pin = PIN_2;
	uartCfg.func = PINSEL_FUNC_01;
	uartCfg.mode = PINSEL_TRISTATE;
	uartCfg.openDrain = DISABLE;
	PINSEL_ConfigPin(&uartCfg);
	GPIO_SetDir(PORT_0, (1 << 2), GPIO_OUTPUT);
	GPIO_ClearPins(PORT_0, 1 << 2);
	//CONFIGURACION DE PIN UART, EL RX
	PINSEL_CFG_T uartCfg1;
	uartCfg1.port = PORT_0;
	uartCfg1.pin = PIN_3;
	uartCfg1.func = PINSEL_FUNC_01;
	uartCfg1.mode = PINSEL_TRISTATE;
	uartCfg1.openDrain = DISABLE;
	PINSEL_ConfigPin(&uartCfg1);
	GPIO_SetDir(PORT_0, (1 << 3), GPIO_OUTPUT);
	GPIO_ClearPins(PORT_0, 1 << 3);
}

void configTimer(){
	//CONFIGURAR EL TIMER0
	TIM_TIMERCFG_T timerCfg;
	timerCfg.prescaleOpt = TIM_US;
	timerCfg.prescaleValue = 1;
	TIM_InitTimer(LPC_TIM0, &timerCfg);

	//CONFIGURAR EL TIMER1
	TIM_TIMERCFG_T timerCfg1;
	timerCfg1.prescaleOpt = TIM_US;
	timerCfg1.prescaleValue = 1;
	TIM_InitTimer(LPC_TIM1, &timerCfg1);

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

	//CONFIGURAR EL MATCH0 DEL TIMER1
	TIM_MATCHCFG_T matchCfg2;
	matchCfg2.channel = TIM_MATCH_0;
	matchCfg2.intEn = ENABLE;
	matchCfg2.stopEn = DISABLE;
	matchCfg2.resetEn = ENABLE;
	matchCfg2.extOpt = TIM_NOTHING;
	matchCfg2.matchValue = 250000;
	TIM_ConfigMatch(LPC_TIM1, &matchCfg2);

	GPIO_SetPins(PORT_0,(1<<0));
	GPIO_SetPins(PORT_0,(1<<1));

	//INTERRUPCIONES
	NVIC_EnableIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER2_IRQn);
    TIM_Enable(LPC_TIM0);
    TIM_Enable(LPC_TIM1);
    TIM_Enable(LPC_TIM2);
}


void configADC(){
    ADC_Init(100000);

    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_3);

    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_ChannelEnable(ADC_CHANNEL_3);

    ADC_PowerUp();
}

void confiEINT(){
	EXTI_Init();

	EXTI_CFG_T exti1;
	exti1.line = EXTI_EINT0;
	exti1.mode = EXTI_EDGE_SENSITIVE;
	exti1.polarity = EXTI_RISING_EDGE;

	EXTI_ConfigEnable(&exti1);

	EXTI_CFG_T exti2;
	exti2.line = EXTI_EINT1;
	exti2.mode = EXTI_EDGE_SENSITIVE;
	exti2.polarity = EXTI_RISING_EDGE;

	EXTI_Config(&exti2);
	EXTI_ConfigEnable(&exti2);

	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
}

void configGPIO_UART(void) {
    //nada
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

void TIMER1_IRQHandler(void){
	TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);

	/*ADC_StartCmd(ADC_START_NOW);

	while(!ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE));
	while(!ADC_ChannelGetStatus(ADC_CHANNEL_3, ADC_DATA_DONE));

	convertido1 = ADC_ChannelGetData(ADC_CHANNEL_0);
	convertido2 = ADC_ChannelGetData(ADC_CHANNEL_3);*/

	ADC_ChannelDisable(ADC_CHANNEL_3);
	ADC_ChannelEnable(ADC_CHANNEL_0);

	ADC_StartCmd(ADC_START_NOW);

	while(!ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE));
	convertido1 = ADC_ChannelGetData(ADC_CHANNEL_0);

	ADC_ChannelDisable(ADC_CHANNEL_0);
	ADC_ChannelEnable(ADC_CHANNEL_3);

	ADC_StartCmd(ADC_START_NOW);

	while(!ADC_ChannelGetStatus(ADC_CHANNEL_3, ADC_DATA_DONE));
	convertido2 = ADC_ChannelGetData(ADC_CHANNEL_3);

	calcularPWMServo1();
	calcularPWMServo2();

	angulo1 = (convertido1 * 360) / 4095;
	angulo2 = (convertido2 * 360) / 4095;

	enviarUART();
}


void calcularPWMServo1(){
	/*ADC_StartCmd(ADC_START_NOW);
	while(!ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE));
	convertido = ADC_ChannelGetData(ADC_CHANNEL_0);*/
    if(convertido1<455){
    	PWM1 = 500;
    }else if(convertido1>=455 && convertido1<910){
    	PWM1 = 722;
    }else if(convertido1>=910 && convertido1<1365){
    	PWM1 = 944;
    }else if(convertido1>=1365 && convertido1<1820){
    	PWM1 = 1166;
    }else if(convertido1>=1820 && convertido1<2275){
    	PWM1 = 1388;
    }else if(convertido1>=2275 && convertido1<2730){
    	PWM1 = 1610;
    }else if(convertido1>=2730 && convertido1<3185){
    	PWM1 = 1832;
    }else if(convertido1>=3185 && convertido1<3640){
    	PWM1 = 2054;
    }else if(convertido1>=3640 && convertido1<3950){
    	PWM1 = 2276;
    }else if(convertido1>=3950){
    	PWM1 = 2500;
    }

    TIM_ResetCounter(LPC_TIM0);
    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_0, PWM1);
}


void calcularPWMServo2(){/*
	ADC_StartCmd(ADC_START_NOW);
	while(!ADC_ChannelGetStatus(ADC_CHANNEL_3, ADC_DATA_DONE));
	convertido = ADC_ChannelGetData(ADC_CHANNEL_3);*/
    if(convertido2<455){
    	PWM2 = 500;
    }else if(convertido2>=455 && convertido2<910){
    	PWM2 = 722;
    }else if(convertido2>=910 && convertido2<1365){
    	PWM2 = 944;
    }else if(convertido2>=1365 && convertido2<1820){
    	PWM2 = 1166;
    }else if(convertido2>=1820 && convertido2<2275){
    	PWM2 = 1388;
    }else if(convertido2>=2275 && convertido2<2730){
    	PWM2 = 1610;
    }else if(convertido2>=2730 && convertido2<3185){
    	PWM2 = 1832;
    }else if(convertido2>=3185 && convertido2<3640){
    	PWM2 = 2054;
    }else if(convertido2>=3640 && convertido2<3950){
    	PWM2 = 2276;
    }else if(convertido2>=3950){
    	PWM2 = 2500;
    }

    TIM_ResetCounter(LPC_TIM2);
    TIM_UpdateMatchValue(LPC_TIM2, TIM_MATCH_0, PWM2);
}


void EINT0_IRQHandler(void){
	EXTI_ClearFlag(EXTI_EINT0);
    if(totalPosiciones < 2048){
        PosicionesGuar1[totalPosiciones] = (uint32_t)PWM1;
        PosicionesGuar2[totalPosiciones] = (uint32_t)PWM2;
        totalPosiciones++;
    }
}

void EINT1_IRQHandler(void){
	EXTI_ClearFlag(EXTI_EINT1);
	GPIO_TogglePins(PORT_0, 1 << 4);
	//enviarUART();
	//GPIO_TogglePins(PORT_0, 1 << 2);
}

void configUART(void) {
    UART_CFG_T uartCfg;
    uartCfg.baudRate = 9600;
    uartCfg.parity = UART_PARITY_NONE;
    uartCfg.dataBits = UART_DBITS_8;
    uartCfg.stopBits = UART_STOPBIT_1;
	UART_Init(UART0, &uartCfg);

	UART_FIFO_CFG_T fifoCfg;
	fifoCfg.resetRxBuf = ENABLE;
	fifoCfg.resetTxBuf = ENABLE;
	fifoCfg.dmaMode = DISABLE;
	fifoCfg.level = UART_FIFO_TRGLEV0;
	UART_FIFOConfig(UART0, &fifoCfg);

	UART_TxEnable(UART0);
}

void enviarUART(void)
{
    char msg[40];

    sprintf(msg,
            "$S1:%d,S2:%d,M:MANUAL\n",
            angulo1,
            angulo2);

    UART_Send(UART0,
              (uint8_t*)msg,
              strlen(msg),
              BLOCKING);
}

void configDAC(void){
	//nada
}
void configDMA(void){
	//nada
}
//void configUART(void){
	//nada
//}

