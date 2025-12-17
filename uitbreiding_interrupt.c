/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32wbxx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32wbxx_it.h"

/* External variables --------------------------------------------------------*/
// GEEN EXTERNE VARIABELEN MEER NODIG HIER

/******************************************************************************/
/* Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/

void NMI_Handler(void) { while (1) {} }
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32WBxx Peripheral Interrupt Handlers                                    */
/* We hebben de TIM2 en DMA handlers hier WEGGEHAALD en verplaatst naar main  */
/******************************************************************************/
