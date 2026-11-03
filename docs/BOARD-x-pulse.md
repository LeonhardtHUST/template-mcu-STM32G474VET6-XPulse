# X Pulse 板卡引脚与时钟配方

本模板默认面向 **X Pulse** 开发板（嘉立创EDA V1.0，STM32G474VET6，LQFP100）。
板卡资料与配套例程在 `D:\Projects\GitHub\NUEDC_2027\X Pulse开发板参考资料\`：
14 个 CubeMX 例程（GPIO/USART/ADC/DAC/Timer/QSPI/LCD/USB/FreeRTOS/RTC）+ 原理图 PDF。
例程用 **Keil MDK + CubeMX（FW_G4 V1.5.1）**；本模板把引脚与时钟事实提炼进
PlatformIO 工程。本文的引脚表已与例程 `.ioc` 和原理图交叉核对。

## 1. 主控与存储

| 项 | 值 |
| --- | --- |
| 型号 | STM32G474VET6 |
| 封装 | LQFP100 |
| 内核 / 主频 | Cortex-M4F，**170 MHz**（HSE 8 MHz + PLL） |
| Flash / RAM | 512 KB / 128 KB（含 32 KB CCM SRAM） |
| 模板板级定义 | `boards/genericSTM32G474VE.json`（`mcu = stm32g474vet6`） |

## 2. 板上外设引脚

引脚定义来自例程 `.ioc`（各例程交叉一致）并与原理图核对：

| 功能 | 引脚 | 电气 | 来源例程 |
| --- | --- | --- | --- |
| LED_B | PC0 | 推挽输出，**低电平点亮**（初始高=灭） | GPIO / 全部 |
| LED_G | PC1 | 推挽输出，低电平点亮 | GPIO / 全部 |
| LED_R | PC2 | 推挽输出，低电平点亮 | GPIO / 全部 |
| KEY | PB8（BOOT0 复用） | 外部中断 EXTI8 | GPIO |
| USART1 TX/RX | PA9 / PA10 | 异步 115200（调试串口） | USART 等 |
| ADC1_IN1…IN4 | PA0…PA3 | 单端 | ADC |
| DAC1_OUT1 | PA4 | — | DAC |
| QSPI Flash（**W25Q64**，8MB） | PE10=CLK, PE11=NCS, PE12…15=IO0…3 | 单 Bank 1 | QSPI_W25Q_Flash |
| SPI2 LCD（ST7789，FPC 扩展口） | PB13=SCK, PB15=MOSI（单工）；PA5=RST, PA6=BLK, PA7=CS, PB14=DC | 例程用 SPI 模式 3、软件 NSS | SPI_LCD_* |
| USB DM/DP | PA11 / PA12 | 全速设备 | USB_USART |
| HSE 晶振 | PF0 / PF1 | **8 MHz**，`RCC_OSC_IN/OUT` | 全部 |
| LSE 晶振 | PC14 / PC15 | 32.768 kHz（仅 RTC 例程启用） | RTC |
| SWD | PA13 / PA14 | `SYS_JTMS-SWDIO` / `SYS_JTCK-SWCLK` | 模板显式声明 |

注意：

- LED 三颗支持 TIM1_CH1/2/3 硬件 PWM（Timer_PWM 例程），呼吸灯不要占用这三脚做其它用途。
- PB8 是 BOOT0 复用脚，作为 KEY 使用时注意上电瞬间的采样电平（BOOT0 电平决定启动区）。
- 模板已启用：时钟 + 三色 LED + KEY(EXTI) + USART1 + QSPI1 + SPI2(LCD)。
  ADC/DAC/USB/LSE 未启用，需要时在 `.ioc` 里参照例程补加再生成。
- 板上颗粒实测为 **W25Q64（JEDEC ID 0xEF4017，8MB）**，`.ioc` 的 `FlashSize=23` 已按实板修正；
  例程按 W25Q32 写的 `FlashSize=22` 与实板不符。
- **QSPI 跑不了 CubeMX 例程默认的 85MHz**（`ClockPrescaler=1` 实板读取失败）：
  降到 17MHz（`ClockPrescaler=9`）后稳定，`.ioc` 已按实板修正。
- 板上**没有 USB 转 UART 桥**（Type-C 的 D+/D− 直连 MCU，是 **USB 设备口**）：
  插电脑后枚举为 STM32 Virtual ComPort（VID 0483/PID 5740）；固件已启用 USB CDC，
  心跳与命令行（help/uptime/key/mode/flash/flashtest/reset）在 CDC 与 USART1 双通道可用。
- USB 内核时钟走 **HSI48**（`RCC.CK48CLockSelection=RCC_USBCLKSOURCE_HSI48`）：
  例程方案（PLLQ=DIV6）要求 VCO=288M、主频降到 144M，模板不采用；HSI48 下 170M 主频不动。
  实测不加 CRS 校准即可枚举稳定；若日后通信误码，再启用 CRS 以 SOF 校准。
- LED 三脚已配为 TIM1_CH1/2/3 PWM（1kHz，PSC=169/ARR=1000，低电平点亮），
  **不再是普通 GPIO**：HAL_GPIO_TogglePin 对它们无效，调亮度用 `__HAL_TIM_SET_COMPARE`
  （比较值 1000=全暗，0=全亮）。

## 3. 时钟配方（模板默认，与例程一致）

```ini
RCC.HSE_VALUE=8000000
RCC.PLLSourceVirtual=RCC_PLLSOURCE_HSE
RCC.PLLM=RCC_PLLM_DIV2      # 8 MHz / 2 = 4 MHz（VCO 输入）
RCC.PLLN=85                 # 4 MHz × 85 = 340 MHz（VCO）
# PLLR = /2 → 170 MHz（CubeMX 默认 R=2，不单列在 IPParameters）
RCC.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK
```

170 MHz 下同时满足：

- 电压档 `PWR_REGULATOR_VOLTAGE_SCALE1_BOOST`；
- Flash 等待周期 `FLASH_LATENCY_4`（CubeMX 自动生成，**不要手写更小的值**）；
- APB1/APB2 均 170 MHz；定时器时钟按 `RCC.APBxTimFreq_Value` 算。

> 例程里的 GPIO.ioc（CubeMX 6.9.1）写的是 `PLLM=DIV4`，与 USART/QSPI 例程（DIV2）不一致；
> 以 DIV2 + N=85 为准（两组在数学上都凑出 170 MHz，DIV2 路径 VCO 输入 4 MHz 更规范）。

## 4. 与例程工程的结构差异

| 项 | 例程（X Pulse 参考资料） | 本模板 |
| --- | --- | --- |
| 工具链 | Keil MDK（AC5/AC6） | PlatformIO + GCC ARM |
| CubeMX 工具链选项 | `MDK-ARM V5.32` | `Makefile`（只用来产出 Core/） |
| `CoupleFile` | `true`（生成 `gpio.c` / `quadspi.c` 等） | `false`（全部 `MX_xxx_Init()` 在 `main.c`） |
| 固件包 | FW_G4 V1.5.1 | FW_G4 V1.6.3（与 PlatformIO 包同版本） |
| 库来源 | 仓库内 `Drivers/` 全量提交 | PlatformIO `framework-stm32cubeg4@1.6.3` |

移植例程外设时，把 CubeMX 配置照抄进本模板 `.ioc`，重新生成即可；
业务代码写进 `USER CODE` 块或 `Core/Src` 下新增的独立文件（`src_dir` 自动编译）。
