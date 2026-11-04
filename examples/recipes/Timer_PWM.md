# 配方：Timer_PWM（TIM1 三通道呼吸灯）

来源工程：`docs/XPulse/例程/Timer_PWM/Timer_PWM.ioc`。对应 [总索引](../INDEX.md) 第 11 行，
验证等级 **L0**。

## ① 对应 IP

```ini
Mcu.IP0=NVIC
Mcu.IP1=RCC
Mcu.IP2=SYS
Mcu.IP3=TIM1
Mcu.Pin0=PF0-OSC_IN
Mcu.Pin1=PF1-OSC_OUT
Mcu.Pin2=PC0
Mcu.Pin3=PC1
Mcu.Pin4=PC2
Mcu.Pin5=VP_SYS_VS_Systick
Mcu.Pin6=VP_SYS_VS_DBSignals
Mcu.Pin7=VP_TIM1_VS_ClockSourceINT
Mcu.PinsNb=8
```

三个灯脚不是 `GPIO_Output`，而是 TIM1 的 PWM 复用信号（**信号名必须带 `S_` 前缀**）：

```ini
PC0.Signal=S_TIM1_CH1
PC1.Signal=S_TIM1_CH2
PC2.Signal=S_TIM1_CH3
```

`tim.c` 里的实测参数（170 MHz 定时器时钟下约 1 kHz）：

```c
htim1.Init.Prescaler = 169;      /* 170 MHz / 170 = 1 MHz */
htim1.Init.Period    = 1000;     /* 1 MHz / 1000 = 1 kHz */
sConfigOC.OCMode     = TIM_OCMODE_PWM1;
sConfigOC.Pulse      = 0;
/* HAL_TIM_PWM_ConfigChannel(..., TIM_CHANNEL_1/2/3) */
```

- 板载三色灯共阳、**低电平点亮**：比较值越大越亮（占空比高 = 低电平时间长），
  用 `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_x, value)` 调亮度；
  此时 `HAL_GPIO_TogglePin()` 对这些脚**无效**。
- `VP_TIM1_VS_ClockSourceINT` 表示内部时钟源，抄 `.ioc` 时不要漏掉这一行。
- 本仓默认是 **HSI 16 MHz**：`Prescaler=169 / Period=1000` 会得到约 94 Hz 而非 1 kHz。
  要么先按 [README 的可选 170 MHz 配方](../../README.md) 改时钟，要么按 16 MHz 重算 PSC/ARR。

## ② 可复用文件清单

| 文件 | 处理 | 理由 |
| --- | --- | --- |
| `Core/Src/tim.c` / `Core/Inc/tim.h` | **摘参数与启动序列** | `MX_TIM1_Init()` 的 PSC/ARR/OCMode/Pulse 与 `HAL_TIM_PWM_ConfigChannel` 三通道配置值得复用；但例程 `CoupleFile=true` 才有独立 `tim.c`，本仓应让 `.ioc` 生成 `MX_TIM1_Init()` |
| `Core/Src/main.c` 的 `USER CODE` 段 | **值得拷** | `HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_x)` 与呼吸循环逻辑，纯 HAL API |
| `Core/Src/gpio.c` | 不整文件拷 | 例程把 PC0–PC2 的 AF 配置放在 `gpio.c`；本仓由 `.ioc` 的 `Mcu.Pin*` + `Signal` 生成 |
| `Core/Src/stm32g4xx_it.c` / `stm32g4xx_hal_msp.c` | 不拷 | CubeMX 生成物，本仓已有 |
| `Core/Inc/main.h` / `stm32g4xx_hal_conf.h` / `stm32g4xx_it.h` | 不拷 | CubeMX 覆盖生成；缺 `HAL_TIM_MODULE_ENABLED` 就在 `.ioc` 加 TIM1 再生成 |
| `Core/Src/system_stm32g4xx.c` | 不拷 | 已由 `build_src_filter` 排除，拷入会重复定义 |
| `MDK-ARM/**`、`.mxproject`、`Drivers/**` | 丢掉 | Keil 样板 / 生成物，本仓 0 入库 |

业务代码放 `lib/<name>/` 并配 [library.json 范式](../../docs/LIB-PATTERN.md)，
不要写进 `Core/Src/main.c` 的块外。

## ③ 与 PlatformIO 的差异

- 例程是 **Keil MDK-ARM**（FW_G4 V1.5.1），本模板是 PlatformIO + GCC ARM
  （`framework-stm32cubeg4@1.6.3`），HAL 与启动文件来源不同。
- `.mxproject` / `Drivers/` 在本仓 **0 入库**（`.gitignore` 忽略），例程里的那份不要搬。
- **不能** `config load docs/XPulse/例程/Timer_PWM/Timer_PWM.ioc` 后直接
  `project generate`：工程名/路径不一致、旧版 `.ioc` 会触发迁移改写（见
  [CubeMX 工具实测 §3.2 / §3.10](../../docs/TOOLING-cubemx.md)）。
- **不能**在例程目录下 `pio run`：没有 `platformio.ini` 与板级定义。
- 例程 `CoupleFile=true`（`tim.c` / `gpio.c` 分文件），本仓 `CoupleFile=false`
  （`MX_TIM1_Init()` 集中在 `main.c`）；直接拷 `tim.c` 会与生成物重复定义。

## ④ 接入步骤 + 已知坑

1. 在本仓 `.ioc` 里加 `Mcu.IP3=TIM1`、补 `Mcu.Pin*` / `Mcu.PinsNb`、
   `PC0/PC1/PC2.Signal=S_TIM1_CHx`、`VP_TIM1_VS_ClockSourceINT`，
   以及 `TIM1.IPParameters`（所有手写参数都要进白名单）。
2. CubeMX `project generate`，核验日志无 `Cannot map`、`main.c` 有 `MX_TIM1_Init()`
   且 PSC/ARR/Pulse 与预期一致（L4）。
3. 把 `HAL_TIM_PWM_Start()` 与亮度循环放进 `lib/<name>/`，配 `library.json`；
   引脚引用用 `Core/Inc/xpulse.h` 的 `XPULSE_LED_*` 宏。
4. `pio run`（L3），记录 Flash / RAM。
5. **L5 实板**：用示波器或肉眼确认约 1 kHz PWM、三通道独立可调；共阳接法下
   占空比与亮度方向容易搞反，先固定一通道验证。
6. 已知坑：[占位符清单 D3 / D4](../../docs/PLACEHOLDERS.md)（参数白名单、
   `S_` 前缀信号枚举写错会被静默丢弃）、
   [CubeMX 工具实测 §3.9](../../docs/TOOLING-cubemx.md)（行尾混用致参数静默丢弃）、
   [README 可选 170 MHz 配方](../../README.md)（默认 HSI16 下频率不同）。

> 统一接入路径与 4 栏模板说明见 [示例目录说明](../README.md)。
