/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : AS7331 - ULTIEME FIX: Harde Hardware Reset (PA5) + Correcte I2C/UART
  *
  * Deze code is de laatste, meest stabiele implementatie van de AS7331 driver.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
// ** Hardware Pin (Moet overeenkomen met CubeMX instellingen) **
#define AS7331_SYS_GPIO_Port  GPIOA
#define AS7331_SYS_Pin        GPIO_PIN_5

#define AS7331_ADDR_7BIT 0x74u
#define AS7331_ADDR      (AS7331_ADDR_7BIT << 1)

// ** Essentiële Registers (Datasheet) **
#define REG_CREG1        0x00
#define REG_CREG3        0x02
#define REG_OSR          0x03
#define REG_DATA_START   0x0B

// Configuratie
#define CONFIG_CREG1_MED_GAIN      0x48 // Tijd 4 + Gain 8 (Hogere gevoeligheid)
#define UART_BAUDRATE       9600

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

// Driver Prototypes
int __io_putchar(int ch);
static HAL_StatusTypeDef AS7331_Write(uint8_t reg, uint8_t val);
static HAL_StatusTypeDef AS7331_Read(uint8_t reg, uint8_t *data, uint16_t len);

static void AS7331_Hardware_Reset(void);
static void AS7331_Init(void);
static void AS7331_ReadSample(uint16_t *uva, uint16_t *uvb, uint16_t *uvc);

/* --------------------------------------------------------------------------
   I2C / UART HELPERS
-------------------------------------------------------------------------- */
int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    (void)HAL_UART_Transmit(&huart1, &c, 1, 100);
    return ch;
}

static HAL_StatusTypeDef AS7331_Write(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c1, AS7331_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

static HAL_StatusTypeDef AS7331_Read(uint8_t reg, uint8_t *data, uint16_t len)
{
    // Hoge timeout voor lage I2C snelheid
    return HAL_I2C_Mem_Read(&hi2c1, AS7331_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 1000);
}


/* --------------------------------------------------------------------------
   HARDWARE RESET & AS7331 DRIVER LOGICA
-------------------------------------------------------------------------- */
static void AS7331_Hardware_Reset(void)
{
    printf("--> AS7331 HARDWARE RESET (SYS-pin) gestart...\r\n");

    // Trek SYS pin LAAG (Disable/Reset)
    HAL_GPIO_WritePin(AS7331_SYS_GPIO_Port, AS7331_SYS_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    // Trek SYS pin HOOG (Enable/Power On)
    HAL_GPIO_WritePin(AS7331_SYS_GPIO_Port, AS7331_SYS_Pin, GPIO_PIN_SET);
    HAL_Delay(100); // Wacht tot de chip is opgestart

    printf("--> SYS-pin reset voltooid.\r\n");
}

static void AS7331_Init(void)
{
    printf("--- AS7331 Init (Command Mode, Gain 0x48) ---\r\n");

    // 1. Soft-Reset (Datasheet)
    (void)AS7331_Write(REG_CREG3, 0x80);
    HAL_Delay(50);

    // 2. Configureer CREG1 (Gain 0x48)
    (void)AS7331_Write(REG_CREG1, CONFIG_CREG1_MED_GAIN);

    // 3. Zet in Command Measurement Mode (Datasheet)
    (void)AS7331_Write(REG_CREG3, 0x00);

    printf("AS7331 gereed.\r\n");
}

static void AS7331_ReadSample(uint16_t *uva, uint16_t *uvb, uint16_t *uvc)
{
    uint8_t raw[8];

    // 1. Start meting (One-Shot Request)
    (void)AS7331_Write(REG_OSR, 0x80);

    // 2. Wacht op voltooiing
    HAL_Delay(70);

    // 3. Lees de 8 bytes data 
    if (AS7331_Read(REG_DATA_START, raw, 8) == HAL_OK)
    {
        // 4. Data Interpretatie 
        *uva = (raw[3] << 8) | raw[2];
        *uvb = (raw[5] << 8) | raw[4];
        *uvc = (raw[7] << 8) | raw[6];
    }
    else
    {
        // Retourneer 0 bij I2C-fout 
        *uva = *uvb = *uvc = 0;
        printf("I2C Leesfout!\r\n");
    }
}


/* --------------------------------------------------------------------------
   MAIN PROGRAM
-------------------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    printf("\r\n--- STM32 Start (9600 Baud) ---\r\n");

    // ** ESSENTIËLE HARDWARE RESET **
    AS7331_Hardware_Reset();

    AS7331_Init();

    uint16_t uva, uvb, uvc;

    while (1)
    {
        AS7331_ReadSample(&uva, &uvb, &uvc);

        printf("UVA=%u, UVB=%u, UVC=%u\r\n", uva, uvb, uvc);

        HAL_Delay(1000);
    }
}

/* --------------------------------------------------------------------------
   PERIPHERALS (CubeMX-gegenereerde functies)
-------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
    // ... (CubeMX code - Onveranderd) ...
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4|RCC_CLOCKTYPE_HCLK2|
                                  RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|
                                  RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

/**
  * @brief I2C1 Initialization Function.
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    // ** VERIFICATIE: DEZE WAARDE MOET DE LAGE SNELHEID (10KHZ) TIMING ZIJN! **
    hi2c1.Init.Timing = 0x00B07CB4;

    // ... (rest van de init instellingen) ...
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) Error_Handler();
}


/**
  * @brief USART1 Initialization Function (9600 BAUD)
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE; // 9600
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    // ... (CubeMX code - Zorg dat PA5 is geconfigureerd als Output) ...
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // ** GPIO Init voor SYS-pin **
    GPIO_InitStruct.Pin = AS7331_SYS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(AS7331_SYS_GPIO_Port, &GPIO_InitStruct);
    // ... (rest van de GPIO-initialisatie) ...
}

void Error_Handler(void)
{
    __disable_irq();
    while(1);
}
