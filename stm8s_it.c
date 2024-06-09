/**
  ******************************************************************************
  * @file     stm8s_it.c
  * @author   MCD Application Team
  * @version  V2.1.0
  * @date     18-November-2011
  * @brief    Main Interrupt Service Routines.
  *           This file provides template for all peripherals interrupt service 
  *           routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/

#include "stm8s_it.h"

#define	ADC_ZERO				100U		//100UL		=0 deg temp
#define SW_L 	(GPIOC->IDR & 0x20)	//PC5 - Left
#define SW_U 	(GPIOC->IDR & 0x40)	//PC6 - UP
#define SW_O	(GPIOC->IDR & 0x80)	//PC7 - Ok
#define SW_R	(GPIOD->IDR & 0x4)	//PD2 - Right
#define SW_D	(GPIOD->IDR & 0x8)	//PD3 - Down

extern volatile bool press[5],release[5];
extern volatile uint16_t idle_timer;
extern volatile uint16_t temp,adc_out,temp_old;
extern void TimingDelayDec(void);
#ifdef _COSMIC_
/**
  * @brief Dummy Interrupt routine
  * @par Parameters:
  * None
  * @retval
  * None
*/
INTERRUPT_HANDLER(NonHandledInterrupt, 25)							{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
#endif /*_COSMIC_*/
INTERRUPT_HANDLER_TRAP(TRAP_IRQHandler)									{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
INTERRUPT_HANDLER(TLI_IRQHandler, 0)										{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
INTERRUPT_HANDLER(AWU_IRQHandler, 1)										{
 //	AWU->CSR & AWU_CSR_AWUF;
}
INTERRUPT_HANDLER(CLK_IRQHandler, 2)										{//Switch rules HSE -> HSI

if (CLK->CSSR & CLK_CSSR_CSSD)	{
		CLK->CSSR &=(~CLK_CSSR_CSSD);
		}
if (CLK->CSSR & CLK_CSSR_AUX){
		CLK->ICKR |= CLK_ICKR_HSIEN; 
		CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV);
		}
		
}
INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler, 3)							{

}
INTERRUPT_HANDLER(EXTI_PORTB_IRQHandler, 4)							{

}
INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)							{ //PC5 - Up,		PC6 - Down, 	PC7 - Ok
if (!SW_L) 									{press[0]=1;release[0]=0;}	//Pressed Left
if (!SW_U) 									{press[1]=1;release[1]=0;}	//Pressed UP
if (!SW_O) 									{press[2]=1;release[2]=0;}	//Pressed Ok

if (SW_L	&& (press[0]==1))	{press[0]=0;release[0]=1;}	//Released  Left
if (SW_U	&& (press[1]==1))	{press[1]=0;release[1]=1;}	//Released  Up
if (SW_O	&& (press[2]==1))	{press[2]=0;release[2]=1;}	//Released Ok
idle_timer=0;
_asm("IRET");
}
INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)							{	//PD2 -Right,	PD3 - Left
if (!SW_R) 									{press[3]=1;release[3]=0;}	//Pressed Right
if (!SW_D) 									{press[4]=1;release[4]=0;}	//Pressed Down

if (SW_R	&& (press[3]==1))	{press[3]=0;release[3]=1;}	//Released Right
if (SW_D	&& (press[4]==1))	{press[4]=0;release[4]=1;}	//Released Down

idle_timer=0;
_asm("IRET");
}
INTERRUPT_HANDLER(EXTI_PORTE_IRQHandler, 7)							{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
#ifdef STM8S903

 INTERRUPT_HANDLER(EXTI_PORTF_IRQHandler, 8) {
 
 }
#endif /*STM8S903*/
#if defined (STM8S208) || defined (STM8AF52Ax)
/**
  * @brief CAN RX Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(CAN_RX_IRQHandler, 8)
 {
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }

/**
  * @brief CAN TX Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(CAN_TX_IRQHandler, 9)
 {
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
#endif /*STM8S208 || STM8AF52Ax */
INTERRUPT_HANDLER(SPI_IRQHandler, 10)										{
 
}
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_BRK_IRQHandler, 11)	{
 
}
/**
  * @brief Timer1 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_CAP_COM_IRQHandler, 12)					{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}

#ifdef STM8S903
/**
  * @brief Timer5 Update/Overflow/Break/Trigger Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM5_UPD_OVF_BRK_TRG_IRQHandler, 13) {
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
 /**
  * @brief Timer5 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM5_CAP_COM_IRQHandler, 14) 				{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
#else /*STM8S208, STM8S207, STM8S105 or STM8S103 or STM8AF62Ax or STM8AF52Ax or STM8AF626x */
/**
  * @brief Timer2 Update/Overflow/Break Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13) 		{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
/**
  * @brief Timer2 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM2_CAP_COM_IRQHandler, 14)					{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
#endif /*STM8S903*/

#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S105) || \
    defined(STM8S005) ||  defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8AF626x)
/**
  * @brief Timer3 Update/Overflow/Break Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM3_UPD_OVF_BRK_IRQHandler, 15) 		{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
/**
  * @brief Timer3 Capture/Compare Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM3_CAP_COM_IRQHandler, 16) 				{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
#endif /*STM8S208, STM8S207 or STM8S105 or STM8AF62Ax or STM8AF52Ax or STM8AF626x */
#if defined (STM8S208) || defined(STM8S207) || defined(STM8S007) || defined(STM8S103) || \
    defined(STM8S003) ||  defined (STM8AF62Ax) || defined (STM8AF52Ax) || defined (STM8S903)
/**
  * @brief UART1 TX Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(UART1_TX_IRQHandler, 17) {
 if (UART1->SR & UART1_SR_TXE)	{
// Действия при пустом буфере
	} 
 if (UART1->SR & UART1_SR_TC)		{
// Действия когда передача завершена
	} 
}
 INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18){
}
#endif /*STM8S208 or STM8S207 or STM8S103 or STM8S903 or STM8AF62Ax or STM8AF52Ax */

INTERRUPT_HANDLER(I2C_IRQHandler, 19)										{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}

#if defined(STM8S105) || defined(STM8S005) ||  defined (STM8AF626x)
/**
  * @brief UART2 TX interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(UART2_TX_IRQHandler, 20) 						{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 }
/**
  * @brief UART2 RX interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(UART2_RX_IRQHandler, 21) 						{
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 }
#endif /* STM8S105 or STM8AF626x */
#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
/**
  * @brief UART3 TX interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(UART3_TX_IRQHandler, 20)
 {
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 }

/**
  * @brief UART3 RX interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(UART3_RX_IRQHandler, 21)
 {
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 }
#endif /*STM8S208 or STM8S207 or STM8AF52Ax or STM8AF62Ax */
#if defined(STM8S207) || defined(STM8S007) || defined(STM8S208) || defined (STM8AF52Ax) || defined (STM8AF62Ax)
/**
  * @brief ADC2 interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(ADC2_IRQHandler, 22)
 {
    /* In order to detect unexpected events during development,
       it is recommended to set a breakpoint on the following instruction.
    */
 }
#else /*STM8S105, STM8S103 or STM8S903 or STM8AF626x */
/**
  * @brief ADC1 interrupt routine.
  * @par Parameters:
  * None
  * @retval 
  * None
  */
 INTERRUPT_HANDLER(ADC1_IRQHandler, 22)					 {
	if (ADC1->CSR & ADC1_CSR_EOC) {
		adc_out = ADC1->DRL;
		adc_out |=(ADC1->DRH & (~0b11111100))<<8;//ADC_result <<=8;
		temp_old=temp;
		temp=(adc_out-ADC_ZERO)/3;
		ADC1->CSR &=(~ ADC1_CSR_EOC);
	}
	
 }
#endif /*STM8S208 or STM8S207 or STM8AF52Ax or STM8AF62Ax */
#ifdef STM8S903
/**
  * @brief Timer6 Update/Overflow/Trigger Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM6_UPD_OVF_TRG_IRQHandler, 23)
 {
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
 }
#else /*STM8S208, STM8S207, STM8S105 or STM8S103 or STM8AF52Ax or STM8AF62Ax or STM8AF626x */
/**
  * @brief Timer4 Update/Overflow Interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23) {
 	
/* Update remaining Delay every 1 ms interrupt*/
  TimingDelayDec();
  TIM4->SR1&=~TIM4_SR1_UIF;
  }
#endif /*STM8S903*/
/**
  * @brief Eeprom EEC Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(EEPROM_EEC_IRQHandler, 24)						{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
}
/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/