# 板载例程总索引

本表对应 `docs/XPulse/例程/` 下的 **14 个厂商 Keil 工程**。「对应外设」取自各例程自带
`.ioc` 的 `Mcu.IP*` 清单（IP 名以 CubeMX 重写后的写法为准，例如 QSPI 写作 `QUADSPI1`），
引脚取自 `Mcu.Pin*` / `<Pin>.Signal`。

- **复杂度**：低 = 单个外设；中 = 多个外设或 DMA / 中断配合；高 = 协议栈 / RTOS / 多层中间件。
- **验证等级**：沿用 [工程规约](../AGENTS.md) 的 L0–L5 口径。这些例程只是**只读参考资料**，
  均未在本模板上重新生成、编译或上板，因此**默认全为 L0（未验证）**；按配方接入后请自行
  补充 L3（构建）与 L5（实板）。
- 所有例程源码都在 `docs/XPulse/例程/` 下，**本目录不复制任何一份源码**。

| # | 例程名 | 对应外设 | 复杂度 | 验证等级 | 配方 / 来源 |
| --- | --- | --- | --- | --- | --- |
| 1 | ADC | `ADC1`、`DMA`、`USART1`；PA0–PA3 / PA9 / PA10 | 中 | L0 | [ADC.ioc](../docs/XPulse/例程/ADC/ADC.ioc) |
| 2 | DAC | `DAC1`、`DMA`、`TIM2`、`TIM6`；PA4 / PB8 | 中 | L0 | [DAC.ioc](../docs/XPulse/例程/DAC/DAC.ioc) |
| 3 | FreeRTOS | `FREERTOS`、`USART1`；PC0 / PA9 / PA10 | 高 | L0 | [FreeRTOS.ioc](../docs/XPulse/例程/FreeRTOS/FreeRTOS.ioc) |
| 4 | GPIO | 引脚级 `GPIO_Output` / `GPXTI8`（PB8）；PC0 / PC1 / PC2 | 低 | L0 | [GPIO 配方](recipes/GPIO.md) · [GPIO.ioc](../docs/XPulse/例程/GPIO/GPIO.ioc) |
| 5 | QSPI_W25Q_Flash | `QUADSPI1`、`TIM2`、`USART1`；PE10–PE15 | 高 | L0 | [QSPI_W25Q_Flash.ioc](../docs/XPulse/例程/QSPI_W25Q_Flash/QSPI_W25Q_Flash.ioc) |
| 6 | RTC | `RTC`、`USART1`；PC14 / PC15（LSE） | 中 | L0 | [RTC.ioc](../docs/XPulse/例程/RTC/RTC.ioc) |
| 7 | SPI_LCD_240x135_1.14inch | `SPI2`；PA5–PA7 / PB13–PB15 | 中 | L0 | [.ioc](../docs/XPulse/例程/SPI_LCD_240x135_1.14inch/SPI_LCD_240x135_1.14inch.ioc) |
| 8 | SPI_LCD_280x240_1.83inch | `SPI2`、`I2C3`；PC0–PC2 / PA5–PA7 / PB13–PB15 / PC8 | 中 | L0 | [.ioc](../docs/XPulse/例程/SPI_LCD_280x240_1.83inch/SPI_LCD_280x240_1.83inch.ioc) |
| 9 | SPI_LCD_320x172_1.47inch | `SPI2`；PA5–PA7 / PB13–PB15 | 中 | L0 | [.ioc](../docs/XPulse/例程/SPI_LCD_320x172_1.47inch/SPI_LCD_320x172_1.47inch.ioc) |
| 10 | SPI_LCD_320x240_2.0inch | `SPI2`；PA5–PA7 / PB13–PB15 | 中 | L0 | [.ioc](../docs/XPulse/例程/SPI_LCD_320x240_2.0inch/SPI_LCD_320x240_2.0inch.ioc) |
| 11 | Timer_PWM | `TIM1`（CH1/CH2/CH3）；PC0 / PC1 / PC2 | 中 | L0 | [Timer_PWM 配方](recipes/Timer_PWM.md) · [Timer_PWM.ioc](../docs/XPulse/例程/Timer_PWM/Timer_PWM.ioc) |
| 12 | Timer_interrupt | `TIM1`；PC0 | 低 | L0 | [Timer_interrupt.ioc](../docs/XPulse/例程/Timer_interrupt/Timer_interrupt.ioc) |
| 13 | USART | `USART1`；PA9 / PA10 / PC0 | 低 | L0 | [USART 配方](recipes/USART.md) · [USART.ioc](../docs/XPulse/例程/USART/USART.ioc) |
| 14 | USB_USART | `USB`、`USB_DEVICE`（CDC FS）；PA11 / PA12 | 高 | L0 | [USB_USART.ioc](../docs/XPulse/例程/USB_USART/USB_USART.ioc) |

## 两个需要留意的地方

- `USB_USART.ioc` 为凑 USB 的 48 MHz，把 `PLLN=18`、`SYSCLKFreq_VALUE=144000000`，
  主频降到 **144 MHz**；与其它例程的 170 MHz 不同。按 [README 的可选时钟配方](../README.md)
  改用 HSI48 可以在 170 MHz 下保留 USB 时钟。
- 各例程的 `RCC.PLLM` 在 `DIV2` / `DIV4` 之间不统一，两者都能凑出 170 MHz；
  统一以 `DIV2`（8 MHz / 2 = 4 MHz 作 VCO 输入）+ `PLLN=85` 为准。

## 怎么用

1. 先看 [示例目录说明](README.md)，理解例程与本模板的结构差异。
2. 按 [配方模板](README.md#配方模板) 的 4 栏逐项核对，别整目录搬。
3. 需要库封装时参考 [库封装范式](../docs/LIB-PATTERN.md)。
4. 想跑一条不依赖硬件的回归，看 [L2 主机回归样例](stdio_port/README.md)。

> 本目录是**可选资料**：`vpr init` 时可交互选择删除，或用 `vpr examples:remove` 一键删除。
> 删除只影响本目录，不会触碰 `docs/XPulse/例程/` 下的原始工程。
