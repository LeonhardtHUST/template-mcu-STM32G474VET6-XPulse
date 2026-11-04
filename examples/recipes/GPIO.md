# 配方：GPIO（三色灯 + 按键中断）

来源工程：`docs/XPulse/例程/GPIO/GPIO.ioc`（`Mcu.IP*` / `Mcu.Pin*` 与 `<Pin>.Signal`
均从该文件提取）。对应 [总索引](../INDEX.md) 第 4 行，验证等级 **L0**。

## ① 对应 IP

`.ioc` 的 IP 清单里**没有独立的 GPIO IP**——GPIO 是引脚级配置：

```ini
Mcu.IP0=NVIC
Mcu.IP1=RCC
Mcu.IP2=SYS
Mcu.IPNb=3
Mcu.Pin0=PF0-OSC_IN
Mcu.Pin1=PF1-OSC_OUT
Mcu.Pin2=PC0
Mcu.Pin3=PC1
Mcu.Pin4=PC2
Mcu.Pin5=PB8-BOOT0
Mcu.Pin6=VP_SYS_VS_Systick
Mcu.PinsNb=7
```

引脚级键值（**白名单行 + 值行必须成对出现**）：

```ini
PC0.GPIOParameters=PinState,GPIO_Label
PC0.GPIO_Label=LED_B
PC0.PinState=GPIO_PIN_SET
PC0.Signal=GPIO_Output
PC1.GPIOParameters=PinState,GPIO_Label
PC1.GPIO_Label=LED_G
PC1.PinState=GPIO_PIN_SET
PC1.Signal=GPIO_Output
PC2.GPIOParameters=PinState,GPIO_Label
PC2.GPIO_Label=LED_R
PC2.PinState=GPIO_PIN_SET
PC2.Signal=GPIO_Output
PB8-BOOT0.GPIOParameters=GPIO_Label
PB8-BOOT0.GPIO_Label=KEY
PB8-BOOT0.Signal=GPXTI8
NVIC.EXTI9_5_IRQn=true\:0\:0\:false\:false\:true\:true\:true\:true
```

- PC0 / PC1 / PC2 = 板载三色灯（共阳，**低电平点亮**，初始 `GPIO_PIN_SET` = 灭）。
- PB8 是 BOOT0 复用脚，`GPXTI8` 外部中断；上电瞬间按下会进系统 Bootloader。
- 本仓 `Core/Inc/xpulse.h` 已有 `XPULSE_LED_B/G/R_*`、`XPULSE_KEY_BOOT_*` 宏，代码用宏，
  不要复制例程的 `LED_B_GPIO_Port` 之类名字。
- 注意：本仓默认 `.ioc` 已把 **PC2** 配成 `GPIO_Output`（`LED_R`，板载红灯，开箱即闪），
  接本配方时再补 PC0 / PC1 两个通道即可，见 [占位符清单](../../docs/PLACEHOLDERS.md) 与
  [CubeMX 工具实测 §3.6](../../docs/TOOLING-cubemx.md)。

## ② 可复用文件清单

例程 `Core/` 下的文件与取舍：

| 文件 | 处理 | 理由 |
| --- | --- | --- |
| `Core/Src/main.c` 的 `USER CODE BEGIN 0` / `BEGIN 4` 段 | **值得拷**（只拷回调逻辑） | `HAL_GPIO_EXTI_Callback()` 里按 KEY 翻转 `LED_R/G/B` 的业务代码，纯 HAL API，与工具链无关 |
| `Core/Src/gpio.c` / `Core/Inc/gpio.h` | **只摘参数，不整文件拷** | 例程 `CoupleFile=true` 才有这两个文件；本仓 `CoupleFile=false`，`MX_GPIO_Init()` 由 `main.c` 生成。把参数抄进 `.ioc` 即可 |
| `Core/Src/stm32g4xx_hal_msp.c`、`stm32g4xx_it.c` | 不拷 | CubeMX 生成物，本仓已有同名文件，覆盖会破坏骨架 |
| `Core/Inc/main.h`、`stm32g4xx_hal_conf.h`、`stm32g4xx_it.h` | 不拷 | CubeMX 覆盖生成；缺 HAL 模块就在 `.ioc` 加外设再生成 |
| `Core/Src/system_stm32g4xx.c` | 不拷 | 本仓 `build_src_filter` 已排除，拷进来会与 framework 的 CMSIS 重复定义 |
| `MDK-ARM/**`（`.uvprojx`、`.uvoptx`、`startup_stm32g474xx.s`、`DebugConfig/`、`RTE/`、`readme.txt`） | 丢掉 | Keil 样板，PlatformIO 用 framework 提供的启动文件与链接脚本 |
| `.mxproject` / `Drivers/**` | 丢掉 | 本仓 0 入库、已忽略 |

拷出的回调请放进 `lib/<name>/`（例如 `lib/blinky/`）或 `main.c` 的 USER CODE 块，
并配 [library.json 范式](../../docs/LIB-PATTERN.md)。

## ③ 与 PlatformIO 的差异

- 例程是 **Keil MDK-ARM**（`MDK-ARM V5.32`），本模板是 PlatformIO + GCC ARM，
  两者启动文件、链接脚本、HAL 来源都不同。
- `.mxproject` 与 `Drivers/` 在本仓**均为 0 入库**（`.gitignore` 忽略）；HAL/CMSIS 由
  `framework-stm32cubeg4@1.6.3` 提供，例程里那份 FW_G4 V1.5.1 不要搬。
- **不能**对 `docs/XPulse/例程/GPIO/GPIO.ioc` 执行 `config load` + `project generate`：
  旧版 `.ioc` 会触发迁移改写、工程名与路径都对不上，生成物会散落到别的目录。
- **不能**在 `docs/XPulse/例程/GPIO/` 下跑 `pio run`：没有 `platformio.ini`、
  板级定义与 framework，必然失败。
- 例程 `ProjectManager.CoupleFile=true`（独立 `gpio.c`），本仓是 `false`
  （`MX_GPIO_Init()` 集中在 `main.c`），别指望把 `gpio.c` 丢进 `Core/Src/` 就能编译。

## ④ 接入步骤 + 已知坑

1. 在本仓根目录 `.ioc` 里补上 ① 的 `Mcu.Pin*`、`Mcu.PinsNb`、三组
   `GPIOParameters` / `PinState` / `Signal`、PB8 的 `GPXTI8` 与 NVIC 中断使能；
   **每个新增参数都要同步写进 `GPIOParameters` 白名单**。
2. CubeMX `project generate`，核验：日志无 `Cannot map`、`main.c` 里有
   `MX_GPIO_Init()`、PC0/PC1/PC2 初始电平为 `GPIO_PIN_SET`（L4）。
3. 把 `HAL_GPIO_EXTI_Callback()` 的业务代码放进 `lib/<name>/` 并配 `library.json`；
   代码里用 `XPULSE_LED_*` / `XPULSE_KEY_BOOT_*` 宏，不要写死 `GPIOC` / `GPIO_PIN_0`。
4. `pio run`（L3），记录 Flash / RAM 字节数。
5. **L5 实板**：确认三色灯低电平点亮、按键触发中断；共阳接法写 `RESET` 才亮，
   写反会看到「一直亮」。
6. 若引脚改成 PC2 后现象不对，先查 [CubeMX 工具实测 §3.6 / §3.9](../../docs/TOOLING-cubemx.md)
   （GPIO 白名单、行尾混用致参数静默丢弃）与 [占位符清单 D3–D5](../../docs/PLACEHOLDERS.md)。

> 统一接入路径与 4 栏模板说明见 [示例目录说明](../README.md)；验证等级定义见
> [工程规约 §3](../../AGENTS.md)。
