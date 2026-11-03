/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "cmd_shell.h"
#include "usbd_cdc_if.h"
#include <sfud.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
QSPI_HandleTypeDef hqspi1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* 运行时取证用全局变量：OpenOCD halt 后可直接读内存 */
volatile uint32_t g_uptime_s = 0;    /* 上电后经过的秒数 */
volatile uint32_t g_heartbeat_tx = 0;/* 心跳打印成功次数 */
volatile uint32_t g_key_presses = 0; /* KEY(PB8) 有效按键次数（消抖后） */
volatile uint32_t g_jedec_id = 0;    /* QSPI Flash JEDEC ID；高16位非0时为错误码+阶段标记 */
volatile uint32_t g_qspi_speed = 0;  /* 最终成功的 QSPI 预分频值 */
volatile uint32_t g_led_mode = 0;    /* 呼吸灯模式：0=蓝 1=绿 2=红 3=全彩 */
volatile uint32_t g_sfud_ok = 0;     /* SFUD 初始化：1=成功 2=失败 */
volatile uint32_t g_flash_size = 0;  /* SFUD 识别的 Flash 容量（字节） */
volatile uint32_t g_usb_rx_bytes = 0;/* CDC 收到的字节总数（取证） */

/* ST7789 LCD（2.0 寸 320x240，FPC 扩展口）：驱动来自例程 BSP/LCD_ST7789 */
static uint16_t lcd_line_buffer[320];
static lcd_io lcd_io_desc = {
    .spi = &hspi2,
    .rst = {LCD_RST_GPIO_Port, LCD_RST_Pin, 0},
    .bl  = {LCD_BLK_GPIO_Port, LCD_BLK_Pin, 0},
    .cs  = {LCD_CS_GPIO_Port,  LCD_CS_Pin,  0},
    .dc  = {LCD_DC_GPIO_Port,  LCD_DC_Pin,  0},
    .te  = {0},
};
static lcd lcd_desc = {
    .io = &lcd_io_desc,
    .line_buffer = lcd_line_buffer,
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_QUADSPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* KEY(PB8)：每按一次切换呼吸灯颜色模式（蓝→绿→红→全彩），50ms 软件消抖。 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint32_t last_tick = 0;
  if (GPIO_Pin == KEY_Pin) {
    uint32_t now = HAL_GetTick();
    if (now - last_tick > 50) { /* 50 ms 软件消抖 */
      last_tick = now;
      g_key_presses++;
      g_led_mode = (g_led_mode + 1) & 3;
    }
  }
}

/* SFUD 自检：擦除+写入+回读比对 256B @ 地址 0。
 * 返回 0=成功，否则 SFUD/比对错误码。 */
int flash_selftest(void)
{
  static const uint8_t wbuf[256] = "X-PULSE-W25Q64-SFUD-SELFTEST";
  uint8_t rbuf[256];
  const sfud_flash *f = sfud_get_device(SFUD_W25_DEVICE_INDEX);
  sfud_err err;

  err = sfud_erase(f, 0, sizeof(wbuf));
  if (err != SFUD_SUCCESS)
    return (int)err + 1000;
  err = sfud_write(f, 0, sizeof(wbuf), wbuf);
  if (err != SFUD_SUCCESS)
    return (int)err + 2000;
  err = sfud_read(f, 0, sizeof(rbuf), rbuf);
  if (err != SFUD_SUCCESS)
    return (int)err + 3000;
  return memcmp(wbuf, rbuf, sizeof(wbuf)) == 0 ? 0 : 1;
}

/* QSPI Flash（W25Q 系列）读 JEDEC ID（指令 0x9F，单线）。
 * W25Q32=0xEF4016, W25Q64=0xEF4017, W25Q128=0xEF4018。
 * 返回值：成功=ID；失败=0xS0000|HAL状态（S=1命令阶段/S=2接收阶段，
 * HAL: 1=ERROR 2=BUSY 3=TIMEOUT）。 */
static uint32_t QSPI_ReadJEDECID(void)
{
  uint8_t id[3] = {0};
  QSPI_CommandTypeDef cmd = {0};
  HAL_StatusTypeDef st;
  cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  cmd.Instruction       = 0x9F; /* JEDEC RDID */
  cmd.AddressMode       = QSPI_ADDRESS_NONE;
  cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  cmd.DataMode          = QSPI_DATA_1_LINE;
  cmd.NbData            = 3;
  cmd.DummyCycles       = 0;
  cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
  cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  st = HAL_QSPI_Command(&hqspi1, &cmd, 100);
  if (st != HAL_OK)
    return 0x10000u | (uint32_t)st;
  st = HAL_QSPI_Receive(&hqspi1, id, 100);
  if (st != HAL_OK)
    return 0x20000u | (uint32_t)st;
  return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

/* 先按 CubeMX 配置读，识别不出 W25Q 厂商码就降到 17MHz 重试 */
static void BringUp_QSPI(void)
{
  g_qspi_speed = hqspi1.Init.ClockPrescaler;
  g_jedec_id = QSPI_ReadJEDECID();
  if ((g_jedec_id >> 16) != 0xEF) {
    HAL_QSPI_DeInit(&hqspi1);
    hqspi1.Init.ClockPrescaler = 9; /* 170MHz/(9+1) = 17MHz */
    if (HAL_QSPI_Init(&hqspi1) == HAL_OK) {
      uint32_t id = QSPI_ReadJEDECID();
      if ((id >> 16) == 0xEF) {
        g_jedec_id = id;
        g_qspi_speed = hqspi1.Init.ClockPrescaler;
      }
    }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_QUADSPI1_Init();
  MX_TIM1_Init();
  MX_USB_Device_Init();
  /* USER CODE BEGIN 2 */
  BringUp_QSPI();
  if (sfud_init() == SFUD_SUCCESS) {
    const sfud_flash *f = sfud_get_device(SFUD_W25_DEVICE_INDEX);
    g_flash_size = f->chip.capacity;
    g_sfud_ok = 1;
    printf("SFUD: %s, %lu bytes\r\n", f->name, (unsigned long)g_flash_size);
  } else {
    g_sfud_ok = 2;
    printf("SFUD init FAILED\r\n");
  }
  /* 三色 LED（PC0/1/2 = TIM1_CH1/2/3，1kHz PWM，低电平点亮）呼吸灯 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  lcd_init_dev(&lcd_desc, LCD_2_00_INCH, LCD_ROTATE_90);
  lcd_clear(&lcd_desc, BLACK);
  lcd_set_font(&lcd_desc, FONT_2412, YELLOW, BLACK);
  lcd_print(&lcd_desc, 0, 10, "X Pulse bring-up");
  lcd_set_font(&lcd_desc, FONT_1608, GREEN, BLACK);
  lcd_print(&lcd_desc, 0, 40, "STM32G474VET6 @ 170MHz");
  lcd_print(&lcd_desc, 0, 60, "LCD 2.0 inch 320x240");
  lcd_print(&lcd_desc, 0, 80, "W25Q64 JEDEC: 0x%06lX", (unsigned long)g_jedec_id);
  lcd_set_font(&lcd_desc, FONT_1608, WHITE, BLACK);
  printf("x-pulse bring-up: LCD on, JEDEC ID = 0x%06lX (prescaler=%lu)\r\n",
         (unsigned long)g_jedec_id, (unsigned long)g_qspi_speed);
  cmd_banner();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 呼吸灯：50ms 步进三角波，2s 一个呼吸周期；LED 低电平点亮，
     * PWM1 模式下 CCR 越大高电平越久越暗，故比较值取 1000-duty。 */
    for (uint32_t step = 0; step < 20; step++) {
      static int16_t breath = 0;
      static int8_t dir = 1;
      uint16_t duty;
      cmd_poll(); /* 串口命令轮询（非阻塞，最坏 50ms 响应） */
      HAL_Delay(50);
      breath += (int16_t)(25 * dir);
      if (breath >= 1000) { breath = 1000; dir = -1; }
      if (breath <= 0)    { breath = 0;    dir = 1;  }
      duty = (uint16_t)(1000 - breath);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,
                            (g_led_mode == 0 || g_led_mode == 3) ? duty : 1000);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2,
                            (g_led_mode == 1 || g_led_mode == 3) ? duty : 1000);
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3,
                            (g_led_mode == 2 || g_led_mode == 3) ? duty : 1000);
    }
    /* USART1(PA9/PA10, 115200) 每 1 s 心跳；外接 USB-TTL 可直接查看。 */
    g_uptime_s++;
    lcd_print(&lcd_desc, 0, 110, "uptime:%lus key:%lu mode:%lu   ",
              (unsigned long)g_uptime_s, (unsigned long)g_key_presses,
              (unsigned long)g_led_mode);
    printf("x-pulse alive: 170MHz, uptime %lus, key=%lu, mode=%lu\r\n",
           (unsigned long)g_uptime_s, (unsigned long)g_key_presses,
           (unsigned long)g_led_mode);
    { /* 同一份心跳发 USB CDC（打开 COM29 即可看到） */
      char line[96];
      int n = snprintf(line, sizeof(line),
                       "x-pulse alive: 170MHz, uptime %lus, key=%lu, mode=%lu\r\n",
                       (unsigned long)g_uptime_s, (unsigned long)g_key_presses,
                       (unsigned long)g_led_mode);
      if (n > 0)
        (void)CDC_Transmit_FS((uint8_t *)line, (uint16_t)n);
    }
    g_heartbeat_tx++;
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief QUADSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI1_Init(void)
{

  /* USER CODE BEGIN QUADSPI1_Init 0 */

  /* USER CODE END QUADSPI1_Init 0 */

  /* USER CODE BEGIN QUADSPI1_Init 1 */

  /* USER CODE END QUADSPI1_Init 1 */
  /* QUADSPI1 parameter configuration*/
  hqspi1.Instance = QUADSPI;
  hqspi1.Init.ClockPrescaler = 9;
  hqspi1.Init.FifoThreshold = 4;
  hqspi1.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi1.Init.FlashSize = 23;
  hqspi1.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_3_CYCLE;
  hqspi1.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi1.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi1.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI1_Init 2 */

  /* USER CODE END QUADSPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 169;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RST_Pin|LCD_BLK_Pin|LCD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RST_Pin LCD_BLK_Pin LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_BLK_Pin|LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY_Pin */
  GPIO_InitStruct.Pin = KEY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
