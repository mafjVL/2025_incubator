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
extern DMA_HandleTypeDef hdma_tim2_ch1;
// Variabele ophalen uit main.c
extern volatile uint8_t LED_KLEUR;

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
void SysTick_Handler(void) { HAL_IncTick(); }

/******************************************************************************/
/* STM32WBxx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

void DMA1_Channel1_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_tim2_ch1);
}

// --- KNOP INTERRUPTS ---

// KNOP 1 (PD0) -> GROEN (Kleur 1)
void EXTI0_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  LED_KLEUR = 1;
}

// KNOP 2 (PD1) -> ROOD (Kleur 2)
void EXTI1_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  LED_KLEUR = 2;
}

// KNOP 3 (PC4) -> BLAUW (Kleur 3)
void EXTI4_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
  LED_KLEUR = 3;
}
