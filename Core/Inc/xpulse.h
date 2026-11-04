/**
  ******************************************************************************
  * @file    xpulse.h
  * @brief   XPulse（Penguins-G474 / STM32G474VET6）板级引脚映射表。
  *
  * 本文件由人工维护：CubeMX 只会生成/覆盖 main.c、main.h、gpio.c、stm32g4xx_it.c
  * 等它自己管的文件，不会创建也不会改写本文件，适合存放"板上接线是死的"那部分
  * 信息，避免各处的魔法数字。
  *
  * 约定：
  *  - 命名沿用 CubeMX 的 <NAME>_Pin / <NAME>_GPIO_Port 形式，但统一加 XP_ 前缀，
  *    这样即使以后在 CubeMX 里给同一引脚另起 GPIO_Label，也不会宏重定义撞车；
  *  - 本文件只描述"信号 → 引脚"。引脚模式、复用功能（AF）、上下拉、时钟仍由
  *    .ioc / CubeMX 决定，改了连线请同步改 .ioc；
  *  - 用法示例：
  *      HAL_GPIO_WritePin(XP_LED_R_GPIO_Port, XP_LED_R_Pin, XP_LED_ON);
  *      XP_LED_R_ON();
  ******************************************************************************
  */
#ifndef __XPULSE_H
#define __XPULSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

/* ==========================================================================
 * 1. 板载 RGB 指示灯（LED2，共阳接 3V3，经 RN1 2 kΩ 限流）
 *    低电平点亮、高电平熄灭。PC0/PC1/PC2 是同一颗三色 LED 的三个阴极，
 *    注意顺序是 B / G / R，不要按 R / G / B 去猜。
 * ========================================================================== */
#define XP_LED_B_Pin            GPIO_PIN_0      /* 蓝，LQFP100 pin 15 */
#define XP_LED_B_GPIO_Port      GPIOC
#define XP_LED_G_Pin            GPIO_PIN_1      /* 绿，LQFP100 pin 16 */
#define XP_LED_G_GPIO_Port      GPIOC
#define XP_LED_R_Pin            GPIO_PIN_2      /* 红，LQFP100 pin 17 */
#define XP_LED_R_GPIO_Port      GPIOC

/* 三个通道合在一起，便于整组操作：HAL_GPIO_WritePin(GPIOC, XP_LED_RGB_Pins, XP_LED_OFF) */
#define XP_LED_RGB_Pins         (XP_LED_B_Pin | XP_LED_G_Pin | XP_LED_R_Pin)

/* 共阳接法：写 RESET 才亮 */
#define XP_LED_ON               GPIO_PIN_RESET
#define XP_LED_OFF              GPIO_PIN_SET

#define XP_LED_B_ON()           HAL_GPIO_WritePin(XP_LED_B_GPIO_Port, XP_LED_B_Pin, XP_LED_ON)
#define XP_LED_B_OFF()          HAL_GPIO_WritePin(XP_LED_B_GPIO_Port, XP_LED_B_Pin, XP_LED_OFF)
#define XP_LED_B_TOGGLE()       HAL_GPIO_TogglePin(XP_LED_B_GPIO_Port, XP_LED_B_Pin)
#define XP_LED_G_ON()           HAL_GPIO_WritePin(XP_LED_G_GPIO_Port, XP_LED_G_Pin, XP_LED_ON)
#define XP_LED_G_OFF()          HAL_GPIO_WritePin(XP_LED_G_GPIO_Port, XP_LED_G_Pin, XP_LED_OFF)
#define XP_LED_G_TOGGLE()       HAL_GPIO_TogglePin(XP_LED_G_GPIO_Port, XP_LED_G_Pin)
#define XP_LED_R_ON()           HAL_GPIO_WritePin(XP_LED_R_GPIO_Port, XP_LED_R_Pin, XP_LED_ON)
#define XP_LED_R_OFF()          HAL_GPIO_WritePin(XP_LED_R_GPIO_Port, XP_LED_R_Pin, XP_LED_OFF)
#define XP_LED_R_TOGGLE()       HAL_GPIO_TogglePin(XP_LED_R_GPIO_Port, XP_LED_R_Pin)

#define XP_LED_ALL_OFF()        HAL_GPIO_WritePin(XP_LED_B_GPIO_Port, XP_LED_RGB_Pins, XP_LED_OFF)

/* ==========================================================================
 * 2. 按键
 *    KEY1：接 3V3，10 kΩ（R6）下拉到 GND —— 按下为高电平，松开为低电平；
 *    KEY2：硬件复位键，串 1 kΩ（R7）+ 100 nF 到 GND，直接拉 NRST，不经过 GPIO。
 *    注意 KEY1 用的就是 BOOT0（PB8），上电瞬间按下会进系统 Bootloader，
 *    上电后再按才是普通用户键。
 * ========================================================================== */
#define XP_KEY_BOOT_Pin         GPIO_PIN_8      /* 丝印 KEY1，LQFP100 pin 95 */
#define XP_KEY_BOOT_GPIO_Port   GPIOB
#define XP_KEY_BOOT_ACTIVE      GPIO_PIN_SET    /* 按下 = 高电平 */

/* ==========================================================================
 * 3. 调试接口 SWD（P1 排针 2/4 脚；复位期间别把它复用成 GPIO）
 * ========================================================================== */
#define XP_SWDIO_Pin            GPIO_PIN_13     /* PA13，pin 76 */
#define XP_SWDIO_GPIO_Port      GPIOA
#define XP_SWCLK_Pin            GPIO_PIN_14     /* PA14，pin 77 */
#define XP_SWCLK_GPIO_Port      GPIOA

/* ==========================================================================
 * 4. USART
 *    USART1：接 P1 调试排针的 TX/RX（pin1/pin3），默认 115200 8N1；
 *    USART3：接 FPC1 第 3/4 脚，供外部模块使用。
 * ========================================================================== */
#define XP_USART1_TX_Pin        GPIO_PIN_9      /* PA9，pin 70 */
#define XP_USART1_TX_GPIO_Port  GPIOA
#define XP_USART1_RX_Pin        GPIO_PIN_10     /* PA10，pin 71 */
#define XP_USART1_RX_GPIO_Port  GPIOA

#define XP_USART3_TX_Pin        GPIO_PIN_10     /* PC10，pin 79 */
#define XP_USART3_TX_GPIO_Port  GPIOC
#define XP_USART3_RX_Pin        GPIO_PIN_11     /* PC11，pin 80 */
#define XP_USART3_RX_GPIO_Port  GPIOC

/* ==========================================================================
 * 5. USB 2.0 全速从机（USB-C 口，D-/D+ 直连，CC1/CC2 各 5.1 kΩ 下拉）
 * ========================================================================== */
#define XP_USB_DM_Pin           GPIO_PIN_11     /* PA11，pin 72 */
#define XP_USB_DM_GPIO_Port     GPIOA
#define XP_USB_DP_Pin           GPIO_PIN_12     /* PA12，pin 73 */
#define XP_USB_DP_GPIO_Port     GPIOA

/* ==========================================================================
 * 6. 板载 QSPI NOR Flash（U2 = W25Q32JVZPIQ，8 脚 SOIC + EP）
 *    CubeMX 里选 QUADSPI，Bank1 使用 PE10..PE15 这一组复用脚。
 * ========================================================================== */
#define XP_QSPI_CLK_Pin         GPIO_PIN_10     /* PE10，pin 41 */
#define XP_QSPI_CLK_GPIO_Port   GPIOE
#define XP_QSPI_NCS_Pin         GPIO_PIN_11     /* PE11，pin 42 */
#define XP_QSPI_NCS_GPIO_Port   GPIOE
#define XP_QSPI_IO0_Pin         GPIO_PIN_12     /* PE12，pin 43 */
#define XP_QSPI_IO0_GPIO_Port   GPIOE
#define XP_QSPI_IO1_Pin         GPIO_PIN_13     /* PE13，pin 44 */
#define XP_QSPI_IO1_GPIO_Port   GPIOE
#define XP_QSPI_IO2_Pin         GPIO_PIN_14     /* PE14，pin 45 */
#define XP_QSPI_IO2_GPIO_Port   GPIOE
#define XP_QSPI_IO3_Pin         GPIO_PIN_15     /* PE15，pin 46 */
#define XP_QSPI_IO3_GPIO_Port   GPIOE

/* ==========================================================================
 * 7. FPC1 屏幕 / 外设接口（16 pin，0.5 mm）
 *    ┌────┬───────────────┬────────────────────────────────────────────┐
 *    │ 脚 │ 网络          │ MCU                                        │
 *    ├────┼───────────────┼────────────────────────────────────────────┤
 *    │  1 │ NRST          │ PG10-NRST                                  │
 *    │  2 │ BOOT          │ PB8（BOOT0）                               │
 *    │  3 │ EXT_TX        │ PC10  USART3_TX                            │
 *    │  4 │ EXT_RX        │ PC11  USART3_RX                            │
 *    │  5 │ 3V3           │ -                                          │
 *    │  6 │ GND           │ -                                          │
 *    │  7 │ EXT_ADC_CH1   │ PA0   ADC1_IN1                             │
 *    │  8 │ EXT_ADC_CH2   │ PA1   ADC1_IN2                             │
 *    │  9 │ EXT_I2C_SDA   │ PC9   I2C3_SDA                             │
 *    │ 10 │ EXT_I2C_SCL   │ PC8   I2C3_SCL                             │
 *    │ 11 │ EXT_SPI1_MOSI │ PA7   SPI1_MOSI                            │
 *    │ 12 │ EXT_SPI1_MISO │ PA6   SPI1_MISO                            │
 *    │ 13 │ EXT_SPI1_SCK  │ PA5   SPI1_SCK                             │
 *    │ 14 │ EXT_SPI2_MOSI │ PB15  SPI2_MOSI                            │
 *    │ 15 │ EXT_SPI2_MISO │ PB14  SPI2_MISO                            │
 *    │ 16 │ EXT_SPI2_SCK  │ PB13  SPI2_SCK                             │
 *    │ 17 │ GND（壳体）   │ -                                          │
 *    │ 18 │ GND（壳体）   │ -                                          │
 *    └────┴───────────────┴────────────────────────────────────────────┘
 *    配套的 LCD 例程把这几个脚当普通 GPIO 用（RST/BLK/CS/DC），
 *    所以下面的名字按"板上网络"取，接屏幕时按例程那样配成 GPIO_Output 即可。
 * ========================================================================== */
#define XP_EXT_SPI1_MOSI_Pin        GPIO_PIN_7  /* PA7，pin 29 */
#define XP_EXT_SPI1_MOSI_GPIO_Port  GPIOA
#define XP_EXT_SPI1_MISO_Pin        GPIO_PIN_6  /* PA6，pin 28 */
#define XP_EXT_SPI1_MISO_GPIO_Port  GPIOA
#define XP_EXT_SPI1_SCK_Pin         GPIO_PIN_5  /* PA5，pin 27 */
#define XP_EXT_SPI1_SCK_GPIO_Port   GPIOA

#define XP_EXT_SPI2_MOSI_Pin        GPIO_PIN_15 /* PB15，pin 54 */
#define XP_EXT_SPI2_MOSI_GPIO_Port  GPIOB
#define XP_EXT_SPI2_MISO_Pin        GPIO_PIN_14 /* PB14，pin 53 */
#define XP_EXT_SPI2_MISO_GPIO_Port  GPIOB
#define XP_EXT_SPI2_SCK_Pin         GPIO_PIN_13 /* PB13，pin 52 */
#define XP_EXT_SPI2_SCK_GPIO_Port   GPIOB

#define XP_EXT_I2C_SCL_Pin          GPIO_PIN_8  /* PC8，pin 67 */
#define XP_EXT_I2C_SCL_GPIO_Port    GPIOC
#define XP_EXT_I2C_SDA_Pin          GPIO_PIN_9  /* PC9，pin 68 */
#define XP_EXT_I2C_SDA_GPIO_Port    GPIOC

#define XP_EXT_ADC_CH1_Pin          GPIO_PIN_0  /* PA0，pin 20 */
#define XP_EXT_ADC_CH1_GPIO_Port    GPIOA
#define XP_EXT_ADC_CH2_Pin          GPIO_PIN_1  /* PA1，pin 21 */
#define XP_EXT_ADC_CH2_GPIO_Port    GPIOA

#define XP_EXT_UART_TX_Pin          XP_USART3_TX_Pin
#define XP_EXT_UART_TX_GPIO_Port    XP_USART3_TX_GPIO_Port
#define XP_EXT_UART_RX_Pin          XP_USART3_RX_Pin
#define XP_EXT_UART_RX_GPIO_Port    XP_USART3_RX_GPIO_Port

/* ==========================================================================
 * 8. 时钟 / 复位（只在 CubeMX 里配 RCC，代码里一般用不到这些宏）
 *    HSE：8 MHz 晶振（X1）+ 2 × 20 pF
 *    LSE：32.768 kHz 晶振（X2）
 * ========================================================================== */
#define XP_OSC_IN_H_Pin         GPIO_PIN_0      /* PF0-OSC_IN，pin 12 */
#define XP_OSC_IN_H_GPIO_Port   GPIOF
#define XP_OSC_OUT_H_Pin        GPIO_PIN_1      /* PF1-OSC_OUT，pin 13 */
#define XP_OSC_OUT_H_GPIO_Port  GPIOF
#define XP_OSC_IN_L_Pin         GPIO_PIN_14     /* PC14-OSC32_IN，pin 8 */
#define XP_OSC_IN_L_GPIO_Port   GPIOC
#define XP_OSC_OUT_L_Pin        GPIO_PIN_15     /* PC15-OSC32_OUT，pin 9 */
#define XP_OSC_OUT_L_GPIO_Port  GPIOC

/* NRST 是专用复位脚（PG10，pin 14），不能当普通 GPIO，只做说明不留宏；
 * 板上有 KEY2 复位键，需要软件复位请用 NVIC_SystemReset()。 */

#ifdef __cplusplus
}
#endif

#endif /* __XPULSE_H */
