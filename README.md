# template-mcu-minimal-STM32G474VET6-XPulse

面向 STM32CubeMX + PlatformIO + VS Code 的最小 STM32G474VET6 起步模板。

## 目标平台

- 芯片：STM32G474VET6（LQFP100 封装，512 KB Flash，128 KB RAM）
- 开发板：XPulse / Penguins-G474
- 调试与烧录：ST-Link（SWD 接口）

## 依赖

必需：

- STM32CubeMX（改外设、生成代码）
- PlatformIO Core（`pio`，编译 / 烧录 / 调试）
- VS Code + 扩展：PlatformIO IDE、Cortex-Debug、clangd

可选，缺了照样能编译烧录，只影响辅助功能：

- Git —— 提交信息校验（commitlint、`vpr hooks:*`）
- Node.js ≥ 20 + 下列包管理器之一 —— 跑 `scripts/` 里的快捷命令
  - [Vite+](https://viteplus.dev/) 的 `vp` / `vpr`（推荐，需在 PATH 上）
  - pnpm
  - npm

> `vpr` 每次会打印一行「This project does not use vite-plus」，可忽略：仓库用到的是它的任务运行器与 Hook 转发器，没有引入 `vite-plus` 依赖。

## 初始化（重命名项目）

模板默认名是 `template-mcu-minimal-STM32G474VET6-XPulse`，首次使用先换掉：

```bash
vpr init
```

```bash
pnpm run init
```

```bash
npm run init
```

不带参数时交互式询问新名字，也可以直接给：

```bash
vpr init my-board --yes       # 非交互，跳过确认
vpr init my-board --dry-run   # 只打印计划，不写文件
```

脚本会重命名根目录的 `.ioc`、改掉 `.ioc` 里的 `ProjectManager.ProjectFileName` 与
`ProjectManager.ProjectName`，并把 `README.md`、`.vscode/tasks.json`、`package.json`、
`scripts/cubemx-generate.mx` 里的旧名一并替换（后者硬编码了 `.ioc` 名，改名后
必须同步）。目录名脚本不管，需要时自己改。npm 传参要加 `--`：
`npm run init -- my-board --yes`。

项目名与当前 `.ioc` 一致时脚本不再早退，仍会继续执行后续操作。

<!-- BEGIN EXAMPLES -->
### 示例目录与一键删除

模板自带 `examples/`：14 个板载例程的总索引、3 篇高频接入配方，以及一份可在主机
GCC 上跑的 L2 回归样例。它只是**可选参考资料**，不参与固件构建，可以整目录删除。
初始化脚本的改名目标里也包含 `examples/INDEX.md`（该索引刻意不硬编码模板名，
通常是空操作），保留示例时改名不会让索引失效。

初始化时脚本会在打印计划之后、总确认之前**单独询问**：

```text
是否删除 examples/ 目录？[y/N]
```

默认 **N = 保留**；只有回答 `y` / `yes` 才删除。也可以直接用 CLI 参数决定，
此时不再询问：

```bash
vpr init --no-examples      # 直接删除 examples/，不改项目名
vpr init --keep-examples    # 明确保留，不询问
vpr init my-board --no-examples --yes
node scripts/init-project.mjs --no-examples --dry-run   # 只打印「将删除 examples/」
```

`--no-examples` 与 `--keep-examples` 互斥，同时传入会报错。`--yes` 只跳过确认，
**不会**顺带删除示例：未指定 examples 参数时一律按默认保留，非交互环境也不会等待输入。
`--dry-run` 只显示计划（含「将删除 `examples/`」），不写文件、不删目录。

不想改名、只想删示例，用独立命令（无需项目名，命令本身即显式删除意图）：

```bash
vpr examples:remove
pnpm run examples:remove
npm run examples:remove
node scripts/remove-examples.mjs --dry-run   # 只预览，不删
```

删除只针对仓库根目录下的 `examples/`，会同步移除本 README 中带
`BEGIN/END EXAMPLES` HTML 标记的整个区块，避免留下死链；
`docs/XPulse/例程/` 下的 14 个原始 Keil 工程**不会**被触碰。

示例资料：

- [示例总索引（14 个例程）](examples/INDEX.md)
- [示例目录说明与统一接入流程](examples/README.md)
- 配方：[GPIO 三色灯与按键](examples/recipes/GPIO.md)、
  [Timer_PWM 呼吸灯](examples/recipes/Timer_PWM.md)、
  [USART1 中断回显](examples/recipes/USART.md)
- [L2 主机回归样例](examples/stdio_port/README.md)
<!-- END EXAMPLES -->

## 编译

```bash
pio run
```

## 烧录

```bash
pio run -t upload
```

## 修改外设

在 STM32CubeMX 中打开 `template-mcu-minimal-STM32G474VET6-XPulse.ioc` → 调整时钟、引脚或外设 → 点 `Generate Code` 生成代码 → 执行 `pio run` 重新编译。

> CubeMX 生成代码时会顺带产出 `Drivers/` 和 `.mxproject`，这两个已在 `.gitignore` 中忽略，不会污染版本库；HAL 头文件实际取自 PlatformIO 的 `framework-stm32cubeg4` 包。

## 默认引脚与时钟

- 板载三色灯（LED2，共阳接 3V3、经 `RN1` 2 kΩ 限流，**低电平点亮**）：
  PC0 = 蓝、PC1 = 绿、PC2 = 红
- 用户按键 KEY1：PB8（即 BOOT0 脚，按下为高电平）；KEY2：硬件复位键（拉 NRST）
- 调试口 SWD：PA13 = SWDIO、PA14 = SWCLK（P1 排针 4 / 2 脚）
- 串口：PA9 = USART1_TX、PA10 = USART1_RX（P1 排针 1 / 3 脚，115200 8N1）
- 默认时钟：HSI 16 MHz（未启用 PLL）；板上还有 8 MHz HSE（PF0/PF1）与
  32.768 kHz LSE（PC14/PC15），需要时在 CubeMX 的 RCC 里启用

> 仓库自带的 `.ioc` 已默认把 **PC2** 配成 `GPIO_Output`（`GPIO_Label=LED_R`），
> 对应板载三色灯的红色通道，**开箱即可闪板载红灯**：`Core/Src/main.c` 在
> `USER CODE BEGIN WHILE` 块里用 `xpulse.h` 的 `XPULSE_LED_R_ON()` / `XPULSE_LED_R_OFF()`
> 交替点灯，上电先置 `XPULSE_LED_OFF`（灭）。板载三色灯仍是 PC0 / PC1 / PC2 =
> 蓝 / 绿 / 红、共阳**低电平点亮**，引脚与电平宏（`XPULSE_LED_ON` = `GPIO_PIN_RESET`
> 亮、`XPULSE_LED_OFF` = `GPIO_PIN_SET` 灭）统一来自 `Core/Inc/xpulse.h`；PC13 只引到
> P2/P3 扩展排针，并没有 LED。

### 可选：170 MHz 进阶配方（默认不启用）

默认 `.ioc` 是 **HSI 16 MHz 最小骨架**（3 IP / 4 引脚，`FLASH_LATENCY_0`），
下面是**按需进阶**的时钟配方，不会自动生效。要跑到 170 MHz，在 CubeMX 的 RCC 里
按下面顺序改并重新生成：

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
PLLQ=DIV6 方案——那会把主频压到 144 MHz。板上 HSE 是 PF0/PF1 的 8 MHz 晶振，
LSE 是 PC14/PC15 的 32.768 kHz 晶振。

> 改 `.ioc` 属于 **L4 生成核验**：生成后必须核对日志无 `Cannot map`、且 `main.c` 里
> 出现对应的 `MX_xxx_Init()`；改完再 `pio run`（L3），影响运行时的部分最终要 L5 实板。
> 详见 [工程规约](AGENTS.md)、[占位符清单](docs/PLACEHOLDERS.md) 与
> [CubeMX 工具实测](docs/TOOLING-cubemx.md)。

## 板级引脚映射（`Core/Inc/xpulse.h`）

板上接线是固定的，所以把"信号 → 引脚"集中写进 `Core/Inc/xpulse.h`，代码里不再散落
`GPIO_PIN_x` / `GPIOx` 魔法数字。它是普通头文件，CubeMX **既不生成也不覆盖**；而对它的
`#include` 放在 `main.h` 的 `/* USER CODE BEGIN Includes */` 里，重新生成后依然保留。

命名沿用 CubeMX 的 `<NAME>_Pin` / `<NAME>_GPIO_Port` 形式，但加 `XPULSE_` 前缀，避免以后在
CubeMX 里给同一引脚另起 `GPIO_Label` 时撞名。常用外设：

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

> `xpulse.h` 只负责"引脚在哪"，引脚模式/AF/上下拉仍由 `.ioc` 决定：例如要用 SPI2，
> 先在 CubeMX 里把 PB13/PB14/PB15 配成 SPI2，再到代码里引用 `XPULSE_EXT_SPI2_*`。

## 提交规范（commitlint）

仓库用 [commitlint](https://commitlint.js.org/) 约束提交信息，规范为 **Angular / Conventional
Commits**（规则集 `@commitlint/config-conventional`）。校验点在 Git 的 `commit-msg` 阶段，
由 Vite+ 的 Hook 转发器调用仓库内的 `.vite-hooks/commit-msg`，不合规的提交会被直接拒绝。

> 分工：commitlint 钩子只管**提交信息格式**；[AGENTS.md](AGENTS.md) 管**代码与 CubeMX
> 生成边界**、中文 Conventional Commits 约定和 L0–L5 验证关卡，两者互补。

### 提交信息格式

```text
<type>(<scope>): <subject>
```

- `type`：`feat` `fix` `docs` `style` `refactor` `perf` `test` `build` `ci` `chore` `revert`
- `scope`：可选，本仓库惯用 `core` / `board` / `scripts` / `docs` / `xpulse` / `deps`
- `subject`：祈使句式、结尾不加句号，中英文均可
- 破坏性变更写成 `feat(core)!: ...`，或在 footer 加 `BREAKING CHANGE: ...`

```text
feat(core): 点亮 PC2 板载红灯
fix(scripts): 修正 clangd 脚本的路径转义
docs: 补充烧录说明
```

### 首次启用

依赖只用于提交校验，**不参与固件编译**，不装也能正常 `pio run`。

推荐使用 `vp` / `pnpm` 进行 Node 包管理。

1. 安装依赖

```bash
vp install
```

```bash
pnpm install
```

```bash
npm install
```

2. 启用 Hook 转发器

```bash
vpr hooks:install
```

```bash
pnpm run hooks:install
```

```bash
npm run hooks:install
```

之后提交时转发器会自动调用已入库的 `.vite-hooks/commit-msg`。

> 仓库提交的是 `pnpm-lock.yaml`；用 npm 会另外生成 `package-lock.json`，两种 lockfile 不要同时提交。

### 手动校验 / 临时跳过

复查上一次提交信息（或任意消息文件）：

```bash
vpx commitlint --edit .git/COMMIT_EDITMSG
```

```bash
npx --no-install commitlint --edit .git/COMMIT_EDITMSG
```

```bash
pnpm exec commitlint --edit .git/COMMIT_EDITMSG
```

> 直接校验一段文本时使用管道输入语法：`echo "feat: xxx" | pnpm exec commitlint`

临时禁用 Git Hook：

```bash
VP_GIT_HOOKS=0 git commit -m "Not Linted: 临时跳过校验"   # HUSKY=0 同样生效
```

```powershell
$env:VP_GIT_HOOKS=0; git commit -m "Not Linted: 临时跳过校验"   # PowerShell 不支持行内赋值
```

> `$env:VP_GIT_HOOKS` 会持续整个 PowerShell 会话，复位用 `Remove-Item Env:VP_GIT_HOOKS`。

### 卸载 commitlint

按顺序操作，否则 Hook 会因找不到 commitlint 让之后每次提交都失败。

1）拆除转发器（unset `core.hooksPath`、删除 `.vite-hooks/_`、记录本地禁用偏好）：

```bash
vpr hooks:uninstall
```

```bash
pnpm run hooks:uninstall
```

```bash
npm run hooks:uninstall
```

2）移除依赖：

```bash
vp remove @commitlint/cli @commitlint/config-conventional
```

```bash
pnpm remove @commitlint/cli @commitlint/config-conventional
```

```bash
npm uninstall @commitlint/cli @commitlint/config-conventional
```

3）删除 Hook 脚本与配置：

```bash
git rm -r .vite-hooks commitlint.config.mjs
```

（尚未提交过这些文件时，直接 `rm -rf .vite-hooks commitlint.config.mjs` 即可。）

若不再需要 Node 工具链，再删掉 `package.json`、lockfile、`node_modules/` 以及 `.gitignore` 里的 `node_modules/` 条目。只想在本机停用而不卸载，执行第 1 步即可（`vp hooks status` 查看状态）。

## clangd 初始化

`pio run -t compiledb` 会在项目根目录生成 `compile_commands.json`，clangd 靠它拿到每个
源文件的真实编译参数（含 `arm-none-eabi-gcc` 的 `--sysroot` / `-mcpu` 等），否则 STM32
的 HAL 头文件会大面积报红。

生成后的目录结构：

```text
<project>/
├── compile_commands.json
├── .clangd
├── platformio.ini
├── Core/
├── Drivers/
└── ...
```

> `Drivers/` 由 CubeMX 生成，已被 `.gitignore` 忽略；HAL 头文件实际来自 PlatformIO 的
> `framework-stm32cubeg4` 包，`--query-driver` 就是让 clangd 去问本机的
> `arm-none-eabi-gcc` 要这些内置头文件路径。
>
> 仓库自带的 `.vscode/settings.json` 已经预置了 clangd 配置，扩展列表里也推荐了
> `vscode-clangd`。

### 初始化

```bash
vpr clangd
```

```bash
pnpm run clangd
```

```bash
npm run clangd
```

脚本会跑 `pio run -t compiledb`、写 `.clangd`，并在 `.vscode/settings.json` 缺失时补一份
完整配置（已存在则不动）。完成后在命令面板执行 `clangd: Restart language server`。

### 什么时候需要重新生成

每当 `platformio.ini`、board、framework、`build_flags`、依赖或源文件集合发生变化时，
`compile_commands.json` 就会过期（表现为 clangd 报头文件找不到、宏对不上），重新执行：

```text
pio run -t compiledb
```

`compile_commands.json` 和 `.clangd` 都是生成物，已在 `.gitignore` 中忽略，不必提交。

### 禁用 clangd（交回 PlatformIO IntelliSense）

```bash
vpr clangd:disable
```

```bash
pnpm run clangd:disable
```

```bash
npm run clangd:disable
```

脚本删除 `.vscode/settings.json` 里的 `C_Cpp.*` 与 `clangd.*` 配置（原文件备份为
`settings.json.bak`）以及 `.clangd`，`compile_commands.json` 保留。之后在扩展面板把
`clangd` 设为 Disable (Workspace)，PlatformIO 会重建 `c_cpp_properties.json`。

恢复 clangd：`vpr clangd`。

## 脚本本体（不走 Node）

Windows 上脚本未签名，须绕过执行策略：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\init-clangd.ps1
```

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\disable-clangd.ps1
```

```bash
./scripts/init-clangd.sh
```

```bash
./scripts/disable-clangd.sh
```

> 从别处拷来的脚本可能带 `Zone.Identifier` 标记，先 `Unblock-File .\scripts\init-clangd.ps1`。
> `scripts/*.sh` 需要真正的 bash（Git Bash / WSL）；PATH 上的 `bash` 若是 STM32CubeCLT 自带的精简 sh，会报 `bad substitution`。

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
- **提交被拒绝**：提交信息不符合 Angular / Conventional Commits 格式，
  按上面的「提交规范」改写；也可用 `pnpm exec commitlint --edit .git/COMMIT_EDITMSG` 复查。
- **`vpr` 打印 "This project does not use vite-plus"**：可忽略，仓库只用到它的
  任务运行器与 Hook 转发器。

## 模板维护与验证

规约地基见 [工程规约](AGENTS.md)（12 条硬约束、L0–L5 验证关卡、与 commitlint 的分工），
复制模板后必查项见 [占位符清单](docs/PLACEHOLDERS.md)，工具实测结论见
[CubeMX 工具实测](docs/TOOLING-cubemx.md)，自维护库的封装形式见
[库封装范式](docs/LIB-PATTERN.md)。

- **版本锁定**：`ststm32@20.0.0`、`framework-stm32cubeg4@1.6.3`、
  `toolchain-gccarmnoneeabi@1.70201.0`、`tool-ldscripts-ststm32@0.3.0`、
  `tool-openocd@3.1200.7`。CubeG4 1.6.3 与本机 CubeMX 的 FW_G4 V1.6.3 同版本；
  升级须同步验证「生成代码 + 构建」，不要单独升 HAL。
- **编译选项**：工程源码用 C11 + `-Wall -Wextra -Werror`（`build_src_flags`），
  严格选项不施加于 HAL/CMSIS。**已开启 `-Werror`**：存量告警已清零并经 L3 复核
  （2026-11-04，/tmp 等价副本跑 `pio run` 零告警通过后落地，详见 `platformio.ini` 注释）。
- **链接期可见性**（`build_flags`）：`-fdata-sections` / `-ffunction-sections`
  配合 `-Wl,--gc-sections` 去掉未用段；`-Wl,--print-memory-usage` 让每次链接
  直接打印 Flash / RAM 占用；`-Wl,-Map=${BUILD_DIR}/firmware.map` 产出
  `.pio/build/genericSTM32G474VE/firmware.map`，便于查段与符号布局。
- **`board_build.stm32cube.custom_config_header = yes`** 保证使用
  `Core/Inc/stm32g4xx_hal_conf.h`；缺 `HAL_XXX_MODULE_ENABLED` 时在 `.ioc`
  加外设再生成，不要手改这个头文件。
- **浮点 printf**：newlib-nano 默认不含 `%f`，需要时在 `platformio.ini` 加
  `-Wl,-u,_printf_float` 并重新构建（会增加 Flash/RAM 占用）。
- **验证关卡**：L0 未验证 / L1 静态 / L2 主机回归（主机 GCC 编译不依赖 HAL 的纯 C，
  不需要开发板）/ L3 `pio run` / L4 CubeMX 生成核验（任何 `.ioc` 改动必做）/
  L5 实板。提交信息里如实写明做到的等级，纯文档改动明确写「未运行构建」。
- **构建通过不等于能用**：影响运行时行为的改动必须实板确认，结论里区分
  「编译通过」与「实板验证过」，不要复用旧构建数字充当本次证据。


