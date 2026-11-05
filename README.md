# template-mcu-STM32G474VET6-XPulse

面向 STM32CubeMX + PlatformIO + VS Code 的最小 STM32G474VET6 起步模板。

- 芯片：STM32G474VET6（LQFP100 封装，512 KB Flash，128 KB RAM）
- 开发板：X Pulse
- 调试与烧录：ST-Link（SWD 接口）

## 快速上手

### 1. 安装依赖

- 必需：STM32CubeMX、PlatformIO Core、VS Code
- 可选：Node.js ≥ 20 + `vpr` / `pnpm` / `npm` 任一（只用于提交校验与辅助脚本，不装也能 `pio run`）

> 下文任务命令在三种包管理器间等价：`vpr <task>` ≡ `pnpm run <task>` ≡ `npm run <task>`，各取一个即可。`vpr` 每次打印的「This project does not use vite-plus」可忽略。

### 2. 初始化项目

```bash
vpr init                      # 交互式初始化
vpr init my-board --yes       # 非交互直接执行；--dry-run 只打印计划
```

npm 传参要加 `--`：`npm run init -- my-board --yes`。

脚本会重命名根目录的 `.ioc`、改写 `.ioc` 里的 `ProjectManager.ProjectFileName` 与
`ProjectManager.ProjectName`，并把 `README.md`、`.vscode/tasks.json`、`package.json`、
`scripts/cubemx-generate.mx` 里的旧名一并替换（后者硬编码了 `.ioc` 名，改名后必须同步）。
目录名脚本不管，需要时自己改。

<!-- BEGIN EXAMPLES -->
### 示例目录（可选，可整目录删除）

模板自带 `examples/`：14 个板载例程总索引、3 篇高频接入配方、一份可在主机 GCC 上跑的
L2 回归样例。它只是**可选参考资料**，不参与固件构建。

初始化时脚本会在总确认之前**单独询问**「是否删除 examples/ 目录？[y/N]」，
默认 **N = 保留**；也可以用参数直接决定（此时不再询问）：

```bash
vpr init --no-examples      # 删除 examples/，不改项目名
vpr init --keep-examples    # 明确保留，不询问
```

两者互斥；`--yes` 只跳过确认，**不会**顺带删除示例。不想改名、只想删示例：

```bash
vpr examples:remove                                   # 也可 pnpm run / npm run
node scripts/remove-examples.mjs --dry-run            # 只预览，不删
```

删除只针对仓库根目录的 `examples/`，会同步移除本区块（README 中的 EXAMPLES 标记），
避免留下死链；`docs/XPulse/` 下的 14 个原始 Keil 工程不受影响。

- [示例总索引](examples/INDEX.md) · [示例目录说明与统一接入流程](examples/README.md)
- 配方：[GPIO 三色灯与按键](examples/recipes/GPIO.md) ·
  [Timer_PWM 呼吸灯](examples/recipes/Timer_PWM.md) ·
  [USART1 中断回显](examples/recipes/USART.md)
- [L2 主机回归样例](examples/stdio_port/README.md)
<!-- END EXAMPLES -->

### 3. 编译

```bash
pio run
```

### 4. 烧录

```bash
pio run -t upload
```

### 5. 验证

上电后板载**红灯（PC2）应闪烁**：`Core/Src/main.c` 的 `USER CODE BEGIN WHILE` 块里
用 `xpulse.h` 的 `XPULSE_LED_R_ON()` / `XPULSE_LED_R_OFF()` 交替点灯。这是模板唯一
自带的功能代码，看到红灯闪就可以动手改成自己的逻辑了。

## 修改外设（日常循环）

STM32CubeMX 打开 `template-mcu-STM32G474VET6-XPulse.ioc` → 调整时钟、引脚或外设 →
`Generate Code` 生成代码 → `pio run` 重新编译。

> - CubeMX 顺带产出的 `Drivers/` 与 `.mxproject` 已被 `.gitignore` 忽略；HAL 头文件
>   实际取自 PlatformIO 的 `framework-stm32cubeg4` 包。
> - 用户代码只写在 `USER CODE BEGIN/END` 块内，块外内容下次生成会被覆盖。
> - 改 `.ioc` 属于 **L4 生成核验**：生成后必须核对日志无 `Cannot map`、`main.c` 里
>   出现对应的 `MX_xxx_Init()`，详见 [工程规约](AGENTS.md) 与
>   [占位符清单](docs/PLACEHOLDERS.md)。

## 默认引脚与时钟

- 板载三色灯（LED2，共阳接 3V3、经 `RN1` 2 kΩ 限流，**低电平点亮**）：
  PC0 = 蓝、PC1 = 绿、PC2 = 红
- 用户按键 KEY1：PB8（即 BOOT0 脚，按下为高电平）；KEY2：硬件复位键（拉 NRST）
- 调试口 SWD：PA13 = SWDIO、PA14 = SWCLK（P1 排针 4 / 2 脚）
- 串口：PA9 = USART1_TX、PA10 = USART1_RX（P1 排针 1 / 3 脚，115200 8N1）
- 默认时钟：HSI 16 MHz（未启用 PLL）；板上还有 8 MHz HSE（PF0/PF1）与
  32.768 kHz LSE（PC14/PC15），需要时在 CubeMX 的 RCC 里启用

> 自带的 `.ioc` 是 **HSI 16 MHz 最小骨架**（3 IP / 4 引脚，`FLASH_LATENCY_0`），
> 已默认把 **PC2** 配成 `GPIO_Output`（`GPIO_Label=LED_R`），因此开箱即可闪板载
> 红灯。PC13 只引到 P2/P3 扩展排针，并没有 LED。

### 可选：170 MHz 进阶配方（默认不启用）

要跑到 170 MHz，在 CubeMX 的 RCC 里按下面顺序改并重新生成：

```ini
RCC.HSE_VALUE=8000000
RCC.PLLSourceVirtual=RCC_PLLSOURCE_HSE
RCC.PLLM=RCC_PLLM_DIV2      ; 8 MHz / 2 = 4 MHz（VCO 输入）
RCC.PLLN=85                 ; 4 MHz x 85 = 340 MHz（VCO）
RCC.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK
                            ; PLLR = /2 -> 170 MHz
```

同时由 CubeMX 自动决定、**不要手写更小的值**：

- 电压档 `PWR_REGULATOR_VOLTAGE_SCALE1_BOOST`；
- Flash 等待周期 `FLASH_LATENCY_4`（HSI 16 MHz 默认是 `FLASH_LATENCY_0`）；
- APB1 / APB2 均为 170 MHz，定时器时钟按 `RCC.APBxTimFreq_Value` 计算；
- `boards/genericSTM32G474VE.json` 的 `f_cpu` 从 `16000000L` 改成 `170000000L`
  （只影响 `F_CPU` 宏，不改变 `.ioc` 里的真实时钟）。

USB 从机需要 48 MHz 内核时钟时用 **HSI48**
（`RCC.CK48CLockSelection=RCC_USBCLKSOURCE_HSI48`），不要照抄厂商例程的
PLLQ=DIV6 方案——那会把主频压到 144 MHz。

## 板级引脚映射（`Core/Inc/xpulse.h`）

板上接线固定，所以把「信号 → 引脚」集中写进 `Core/Inc/xpulse.h`，代码里不再散落
`GPIO_PIN_x` / `GPIOx` 魔法数字。它是人工维护的普通头文件，CubeMX **既不生成也不
覆盖**；对它的 `#include` 放在 `main.h` 的 `USER CODE BEGIN Includes` 块里，重新
生成后依然保留。命名沿用 CubeMX 的 `<NAME>_Pin` / `<NAME>_GPIO_Port` 形式并加
`XPULSE_` 前缀，避免以后在 CubeMX 里给同一引脚另起 `GPIO_Label` 时撞名。

| 功能 | 网络名 | 引脚 | 说明 |
| --- | --- | --- | --- |
| RGB 灯 | `LED_B/G/R` | PC0 / PC1 / PC2 | 共阳，低电平点亮 |
| 用户键 | `BOOT` | PB8（BOOT0） | 按下为高，10 kΩ 下拉 |
| 调试 | `SWDIO` / `SWCLK` | PA13 / PA14 | 接 P1 排针 4 / 2 脚 |
| 串口 1 | `TX` / `RX` | PA9 / PA10 | 接 P1 排针 1 / 3 脚 |
| 串口 3 | `EXT_TX` / `EXT_RX` | PC10 / PC11 | 接 FPC1 |
| USB 从机 | `USB_DN` / `USB_DP` | PA11 / PA12 | USB-C 口，CC1/CC2 各 5.1 kΩ 下拉 |
| QSPI Flash | `QSPI_*` | PE10 / PE11 / PE12 / PE13 / PE14 / PE15 | CLK / NCS / IO0..IO3，W25Q32 |
| SPI1 | `EXT_SPI1_*` | PA5 / PA6 / PA7 | SCK / MISO / MOSI，接 FPC1 |
| SPI2 | `EXT_SPI2_*` | PB13 / PB14 / PB15 | SCK / MISO / MOSI，接 FPC1 |
| I2C3 | `EXT_I2C_*` | PC8 / PC9 | SCL / SDA，接 FPC1 |
| ADC | `EXT_ADC_CH1/2` | PA0 / PA1 | ADC1_IN1 / ADC1_IN2，接 FPC1 |
| 晶振 | `OSC_IN/OUT_H` | PF0 / PF1 | 8 MHz HSE |
| 晶振 | `OSC_IN/OUT_L` | PC14 / PC15 | 32.768 kHz LSE |

用法：

```c
#include "main.h"   /* 已经间接包含 xpulse.h */

XPULSE_LED_ALL_OFF();          /* 先把三色灯全灭 */
XPULSE_LED_R_ON();             /* 点亮红灯（PC2） */
HAL_GPIO_TogglePin(XPULSE_LED_G_GPIO_Port, XPULSE_LED_G_Pin);
```

> `xpulse.h` 只负责「引脚在哪」，引脚模式 / AF / 上下拉仍由 `.ioc` 决定：例如要用
> SPI2，先在 CubeMX 里把 PB13/PB14/PB15 配成 SPI2，再到代码里引用
> `XPULSE_EXT_SPI2_*`。

## clangd（补全与跳转）

`pio run -t compiledb` 生成 `compile_commands.json`，clangd 靠它拿到每个源文件的真实
编译参数（含 `arm-none-eabi-gcc` 的 `--sysroot` / `-mcpu` 等），否则 HAL 头文件会
大面积报红。

```bash
vpr clangd     # 跑 compiledb、写 .clangd、缺失时补 .vscode/settings.json（已存在则不动）
```

完成后在命令面板执行 `clangd: Restart language server`。每当 `platformio.ini`、
board、framework、`build_flags`、依赖或源文件集合变化后都要重跑（过期表现为 clangd
报头文件找不到、宏对不上）。`compile_commands.json` 与 `.clangd` 都是生成物，已在
`.gitignore` 中忽略。仓库自带的 `.vscode/settings.json` 已预置 clangd 配置。

不想用 clangd（交回 PlatformIO IntelliSense）：`vpr clangd:disable` 删除相关配置
（原文件备份为 `settings.json.bak`），再到扩展面板把 clangd 设为 Disable (Workspace)；
恢复用 `vpr clangd`。

不走 Node 直接调脚本时，Windows 上未签名须绕过执行策略：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\init-clangd.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\disable-clangd.ps1
```

> 从别处拷来的脚本可能带 `Zone.Identifier` 标记，先 `Unblock-File`。`scripts/*.sh`
> 需要真正的 bash（Git Bash / WSL）；PATH 上的 `bash` 若是 STM32CubeCLT 自带的
> 精简 sh，会报 `bad substitution`。

## 提交规范（commitlint）

提交信息必须是 **Conventional Commits** 格式（规则集 `@commitlint/config-conventional`，
另由 `commitlint.config.mjs` 追加「简述至少含一个汉字」校验，首字不限），
Git `commit-msg` 钩子校验，不合规直接拒绝：

```text
<type>(<scope>): <中文简述，动词开头，不超过 50 字，结尾不加句号>
```

- `type`：`feat` `fix` `docs` `style` `refactor` `perf` `test` `build` `ci` `chore` `revert`
- `scope` 可选：本仓库惯用 `core` / `board` / `scripts` / `docs` / `xpulse` / `deps`
- 破坏性变更写成 `feat(core)!: ...`，或在 footer 加 `BREAKING CHANGE: ...`

```text
feat(core): 点亮 PC2 板载红灯
fix(scripts): 修正 clangd 脚本的路径转义
docs: 补充烧录说明
```

> 分工：commitlint 钩子只管**提交信息格式**；[工程规约](AGENTS.md) 管**代码与
> CubeMX 生成边界**和 L0–L5 验证关卡，两者互补。

### 首次启用

依赖只用于提交校验，**不参与固件编译**，不装也能正常 `pio run`：

```bash
pnpm install        # 或 vp install / npm install
vpr hooks:install   # 启用 Hook 转发器，之后提交会自动调用 .vite-hooks/commit-msg
```

> 仓库提交的是 `pnpm-lock.yaml`；用 npm 会另外生成 `package-lock.json`，
> 两种 lockfile 不要同时提交。

复查上一次提交信息：`pnpm exec commitlint --edit .git/COMMIT_EDITMSG`；
临时跳过校验：`VP_GIT_HOOKS=0 git commit -m "..."`（PowerShell 不支持行内赋值，先
`$env:VP_GIT_HOOKS=0` 再提交，用 `Remove-Item Env:VP_GIT_HOOKS` 复位）。

卸载时按顺序操作，否则 Hook 会因找不到 commitlint 让之后每次提交都失败：
`vpr hooks:uninstall` → `pnpm remove @commitlint/cli @commitlint/config-conventional` →
`git rm -r .vite-hooks commitlint.config.mjs`。只想本机停用而不卸载，执行第一步即可
（`vpr hooks status` 查看状态）。

## 故障排查

- **`pio` 命令不存在**：安装 PlatformIO IDE 扩展后重启 VS Code，或 `pip install platformio`。
  用 `pio --version` 核对；Windows 集成终端的 PATH 可能先匹配到旧的 pio shim。
- **找不到下载器 / 探针连不上**：默认走 ST-Link（`upload_protocol = stlink`、
  `debug_tool = stlink`），底层是 PlatformIO 自带的 OpenOCD + `interface/stlink.cfg`。
  换 CMSIS-DAP 时两项一起改成 `cmsis-dap`；本模板刻意不入库 `launch.json`
  （已在 `.gitignore` 忽略），需要 Cortex-Debug 的 launch / attach 配置时按
  [Cortex-Debug 调试示例](docs/DEBUG-CORTEX.md) 自建，并复用
  `scripts/openocd-stlink.cfg` / `scripts/openocd-cmsis-dap.cfg` 两份探针配置。
- **SWD 连接失败（`cannot read IDR` / `No ACK`）**：先查 SWDIO(PA13) / SWCLK(PA14)
  是否接反、GND 是否共地、目标板是否独立供电；必要时降低 `debug_speed`
  或改用「连接时复位」（`connect_assert_srst`）。
- **重复定义符号**：检查是否把 `Drivers/` 加进编译，或删掉了 `platformio.ini` 里
  `build_src_filter = +<*> -<system_stm32g4xx.c>` 的排除规则。
- **链接脚本不对 / Flash 溢出**：确认 `boards/genericSTM32G474VE.json` 的 `mcu`
  是 `stm32g474vet6`，`maximum_size` / `maximum_ram_size` 为 524288 / 131072。
- **生成后外设没生效**：CubeMX 对 `.ioc` 的错误是**非致命**的，信号名写错只会留一行
  `Cannot map signal` 然后静默丢弃外设。核验生成日志无 `Cannot map`、
  `main.c` 有对应 `MX_xxx_Init()`，且手写参数已进 `IPParameters` / `GPIOParameters`
  白名单。见 [CubeMX 工具实测](docs/TOOLING-cubemx.md)。
- **CubeMX 无头生成卡住**：多半是 `.ioc` 的 `ProjectManager.AskForMigrate=true`
  弹了模态迁移框；本仓 `.ioc` 已设为 `false`（无头生成前必须），若被 CubeMX
  生成 / 保存写回 `true`，再改回 `false` 即可。见 [占位符清单](docs/PLACEHOLDERS.md) D7。
- **clangd 大面积报红**：`compile_commands.json` 过期或缺失，执行 `vpr clangd`
  重新生成；改过 `platformio.ini`、`build_flags`、board 或源文件集合后都要重跑。
- **提交被拒绝**：提交信息不符合 Conventional Commits 格式，按上面的「提交规范」
  改写；也可用 `pnpm exec commitlint --edit .git/COMMIT_EDITMSG` 复查。
- **`vpr` 打印 "This project does not use vite-plus"**：可忽略，仓库只用到它的
  任务运行器与 Hook 转发器。

## 模板维护与验证

规约地基见 [工程规约](AGENTS.md)（12 条硬约束、L0–L5 验证关卡、与 commitlint 的分工），
复制模板后必查项见 [占位符清单](docs/PLACEHOLDERS.md)，工具实测结论见
[CubeMX 工具实测](docs/TOOLING-cubemx.md)，自维护库的封装形式见
[库封装范式](docs/LIB-PATTERN.md)。

- **版本锁定**：`ststm32@20.0.0`、`framework-stm32cubeg4@1.6.3`、
  `toolchain-gccarmnoneeabi@1.70201.0`、`tool-ldscripts-ststm32@0.3.0`、
  `tool-openocd@3.1200.7`。升级须同步验证「生成代码 + 构建」，不要单独升 HAL。
- **编译选项**：工程源码用 C11 + `-Wall -Wextra -Werror`（`build_src_flags`），
  严格选项不施加于 HAL/CMSIS。
- **链接期可见性**（`build_flags`）：`-fdata-sections` / `-ffunction-sections`
  配合 `-Wl,--gc-sections` 去掉未用段；`-Wl,--print-memory-usage` 每次链接直接
  打印 Flash / RAM 占用；`-Wl,-Map=${BUILD_DIR}/firmware.map` 产出
  `.pio/build/genericSTM32G474VE/firmware.map`，便于查段与符号布局。
- **`board_build.stm32cube.custom_config_header = yes`** 保证使用
  `Core/Inc/stm32g4xx_hal_conf.h`；缺 `HAL_XXX_MODULE_ENABLED` 时在 `.ioc`
  加外设再生成，不要手改这个头文件。
- **浮点 printf**：newlib-nano 默认不含 `%f`，需要时在 `platformio.ini` 加
  `-Wl,-u,_printf_float` 并重新构建（会增加 Flash/RAM 占用）。
- **验证关卡**：L0 未验证 / L1 静态 / L2 主机回归 / L3 `pio run` /
  L4 CubeMX 生成核验（任何 `.ioc` 改动必做）/ L5 实板。提交信息里如实写明做到
  的等级，纯文档改动明确写「未运行构建」。
- **构建通过不等于能用**：影响运行时行为的改动必须实板确认，结论里区分
  「编译通过」与「实板验证过」，不要复用旧构建数字充当本次证据。
