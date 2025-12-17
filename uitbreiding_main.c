/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DE STABIELE VERSIE + PIN A3 FIX
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"

/* --- INSTELLINGEN --- */
#define MAX_LEDS       300
#define RESET_SLOTS    60
#define HIGH_CODE      26
#define LOW_CODE       13
#define DIVIDER        60

/* --- VARIABELEN --- */
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_tim2_ch1;
TIM_HandleTypeDef htim2;

uint32_t pwmData[(MAX_LEDS * 24) + RESET_SLOTS];
uint8_t System_Active = 0;

/* --- FUNCTIES --- */
void SystemClock_Config(void);
void Init_Hardware_Stable(void);
uint8_t Read_Pot_Value(uint32_t Channel);
void Set_Color(uint8_t R, uint8_t G, uint8_t B);
void Update_Strip(void);

// INTERRUPT HANDLERS
void DMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_tim2_ch1); }
void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  Init_Hardware_Stable();

  // 1. VISUELE CHECK: Knipper groen lampje
  for(int i=0; i<3; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
      HAL_Delay(100);
  }

  // 2. START OP ZWART
  Set_Color(0, 0, 0);
  Update_Strip();

  while (1)
  {
      // --- KNOPPEN ---
      if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET) { // START
          System_Active = 1;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
          HAL_Delay(200);
      }

      if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1) == GPIO_PIN_RESET) { // STOP
          System_Active = 0;
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
          Set_Color(0, 0, 0);
          Update_Strip();
          HAL_Delay(200);
      }

      // --- LOGICA ---
      if (System_Active == 1)
      {
          uint8_t val_G = Read_Pot_Value(ADC_CHANNEL_1);

          uint8_t val_B = Read_Pot_Value(ADC_CHANNEL_2);

          uint8_t val_R = Read_Pot_Value(ADC_CHANNEL_4);

          uint8_t val_Backup = Read_Pot_Value(ADC_CHANNEL_6);
          if (val_Backup > val_R) val_R = val_Backup;

          Set_Color(val_R, val_G, val_B);
          Update_Strip();
      }

      HAL_Delay(10);
  }
}

// --- FUNCTIES ---

uint8_t Read_Pot_Value(uint32_t Channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = Channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;
    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK)
    {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        uint32_t calc = raw / DIVIDER;
        if (calc > 255) calc = 255;
        return (uint8_t)calc;
    }
    HAL_ADC_Stop(&hadc1);
    return 0;
}

void Set_Color(uint8_t R, uint8_t G, uint8_t B) {
    uint32_t color = (G << 16) | (R << 8) | B;
    for (int i = 0; i < MAX_LEDS; i++) {
        int idx = i * 24;
        for (int b = 23; b >= 0; b--) {
            pwmData[idx++] = (color & (1 << b)) ? HIGH_CODE : LOW_CODE;
        }
    }
}

void Update_Strip(void) {
    for(int i = (MAX_LEDS * 24); i < (MAX_LEDS * 24) + RESET_SLOTS; i++) pwmData[i] = 0;
    HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)pwmData, (MAX_LEDS * 24) + RESET_SLOTS);
}

// --- INIT ---
void Init_Hardware_Stable(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // KLOKKEN
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();

    // LED STRIP
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // KNOPPEN
    GPIO_InitStruct.Pin = GPIO_PIN_4; // Start
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_1; // Stop
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // POTMETERS: We zetten A0(PC0), A1(PC1), A2(PC2), A3(PC3) open
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // En A4 (PA1) ook voor de zekerheid
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // LED
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // ADC (Veilige instellingen)
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    HAL_ADC_Init(&hadc1);
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

    // DMA & Timer
    hdma_tim2_ch1.Instance = DMA1_Channel1;
    hdma_tim2_ch1.Init.Request = DMA_REQUEST_TIM2_CH1;
    hdma_tim2_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim2_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim2_ch1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim2_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim2_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim2_ch1.Init.Mode = DMA_NORMAL;
    hdma_tim2_ch1.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_tim2_ch1);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 39;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);
    __HAL_LINKDMA(&htim2, hdma[TIM_DMA_ID_CC1], hdma_tim2_ch1);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
}

void Error_Handler(void) {}
