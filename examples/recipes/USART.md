# 配方：USART（USART1 中断回显 + printf 重定向）

来源工程：`docs/XPulse/例程/USART/USART.ioc`。对应 [总索引](../INDEX.md) 第 13 行，
验证等级 **L0**。

## ① 对应 IP

```ini
Mcu.IP0=NVIC
Mcu.IP1=RCC
Mcu.IP2=SYS
Mcu.IP3=USART1
Mcu.Pin0=PF0-OSC_IN
Mcu.Pin1=PF1-OSC_OUT
Mcu.Pin2=PC0
Mcu.Pin3=PA9
Mcu.Pin4=PA10
Mcu.Pin5=VP_SYS_VS_Systick
Mcu.PinsNb=6
```

引脚与串口参数：

```ini
PA9.Mode=Asynchronous
PA9.Signal=USART1_TX
PA10.Mode=Asynchronous
PA10.Signal=USART1_RX
PC0.Signal=GPIO_Output          # LED_B，回显时的指示
```

```c
/* Core/Src/usart.c */
huart1.Init.BaudRate     = 115200;
huart1.Init.WordLength   = UART_WORDLENGTH_8B;
huart1.Init.StopBits     = UART_STOPBITS_1;
huart1.Init.Parity       = UART_PARITY_NONE;
huart1.Init.Mode         = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
huart1.Init.OverSampling = UART_OVERSAMPLING_16;
```

- PA9 / PA10 是 P1 排针的 TX / RX（本仓 `platformio.ini` 的 `monitor_speed = 115200`），
  板上**没有** USB 转 UART 桥；Type-C 的 D+/D− 是 USB 设备口，不是串口。
- 例程时钟为 HSE 8 MHz + PLL 170 MHz（`RCC.PLLM=DIV2`、`PLLN=85`）；
  本仓默认 HSI 16 MHz，波特率由 CubeMX 按实际 PCLK 重算，不要手抄分频值。
- 例程用 `int fputc(int ch, FILE *f)` 重定向 printf 到 `HAL_UART_Transmit()`。
  本仓若采用 newlib 的 `_read` / `_write` 覆盖方案，应改为实现
  `__io_putchar()` / `__io_getchar()`（见 [L2 样例](../stdio_port/README.md)），
  两套方案不要同时用。

## ② 可复用文件清单

| 文件 | 处理 | 理由 |
| --- | --- | --- |
| `Core/Src/usart.c` / `Core/Inc/usart.h` | **摘参数，不整文件拷** | `MX_USART1_UART_Init()` 的波特率/模式参数值得复用；本仓 `CoupleFile=false`，应由 `.ioc` 生成 `MX_USART1_UART_Init()` |
| `Core/Src/main.c` 的 `USER CODE BEGIN 0` / `BEGIN 2` 段 | **值得拷（改写后）** | `HAL_UART_RxCpltCallback()` 回显逻辑、`HAL_UART_Receive_IT()` 启动；把 `fputc` 换成 `__io_putchar()` 或明确二选一 |
| `Core/Src/gpio.c` | 不整文件拷 | PC0 指示脚由 `.ioc` 生成 |
| `Core/Src/stm32g4xx_it.c` / `stm32g4xx_hal_msp.c` | 不拷 | CubeMX 生成物；USART1 中断入口由 CubeMX 生成，别手抄 |
| `Core/Inc/main.h` / `stm32g4xx_hal_conf.h` / `stm32g4xx_it.h` | 不拷 | CubeMX 覆盖生成；缺 `HAL_UART_MODULE_ENABLED` 就在 `.ioc` 加 USART1 再生成 |
| `Core/Src/system_stm32g4xx.c` | 不拷 | 已被 `build_src_filter` 排除 |
| `MDK-ARM/**`、`.mxproject`、`Drivers/**`、`readme.txt` | 丢掉 | Keil 样板 / 生成物，本仓 0 入库 |

回显回调放 `lib/<name>/` 并配 [library.json 范式](../../docs/LIB-PATTERN.md)；
引脚用 `Core/Inc/xpulse.h` 的 `XPULSE_USART1_TX/RX_*` 宏。

## ③ 与 PlatformIO 的差异

- 例程是 **Keil MDK-ARM**（`MDK-ARM V5.32`，FW_G4 V1.5.1），本模板是
  PlatformIO + GCC ARM（`framework-stm32cubeg4@1.6.3`）。
- `.mxproject` / `Drivers/` 在本仓 **0 入库**（`.gitignore` 忽略），HAL 由 framework 提供。
- **不能** `config load docs/XPulse/例程/USART/USART.ioc` 后直接 `project generate`：
  工程名 / 路径不一致，旧版 `.ioc` 会触发迁移改写（见
  [CubeMX 工具实测 §3.2 / §3.10](../../docs/TOOLING-cubemx.md)）。
- **不能**在例程目录下 `pio run`：没有 `platformio.ini` 与板级定义。
- 例程 `CoupleFile=true`（独立 `usart.c`），本仓 `CoupleFile=false`
  （`MX_USART1_UART_Init()` 集中在 `main.c`）；直接拷 `usart.c` 会重复定义。
- 例程的 `fputc` 重定向面向 Keil 的 MicroLIB；PlatformIO 用 newlib-nano，
  printf 走 `_write()`，两套 I/O 桩语义不同，不能照搬。

## ④ 接入步骤 + 已知坑

1. 在本仓 `.ioc` 里加 `Mcu.IP3=USART1`、补 `Mcu.Pin*` / `Mcu.PinsNb`、
   `PA9/PA10` 的 `Mode` / `Signal`，以及 `USART1.IPParameters` 白名单和 NVIC 中断使能。
2. CubeMX `project generate`，核验日志无 `Cannot map`、`main.c` 有
   `MX_USART1_UART_Init()` 且波特率为 115200（L4）。
3. 把回显回调与 `__io_putchar()` / `__io_getchar()` 后端放进 `lib/<name>/`，
   配 `library.json`；`.ioc` 生成物不要手改。
4. `pio run`（L3），记录 Flash / RAM。默认 newlib-nano 不含浮点 printf，
   需要 `%f` 时按 [README 模板维护与验证](../../README.md) 加 `-Wl,-u,_printf_float`。
5. **L2 主机回归**：若实现了 `_read` / `_write` 覆盖，先用主机 GCC 跑
   [stdio_port 样例](../stdio_port/README.md) 的两条命令，不需要开发板。
6. **L5 实板**：`pio device monitor` 打开 115200 8N1，确认收发回显；
   TX/RX 接反、共地缺失、把 Type-C 当串口是最常见的三类失败。
7. 已知坑：[占位符清单 D3 / D5](../../docs/PLACEHOLDERS.md)（参数白名单、
   `Cannot map` 静默丢外设）、
   [CubeMX 工具实测 §3.9](../../docs/TOOLING-cubemx.md)（行尾混用）。

> 统一接入路径与 4 栏模板说明见 [示例目录说明](../README.md)。
