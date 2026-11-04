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
`ProjectManager.ProjectName`，并把 `README.md`、`.vscode/tasks.json`、`package.json`
里的旧名一并替换。目录名脚本不管，需要时自己改。npm 传参要加 `--`：
`npm run init -- my-board --yes`。

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

> 仓库自带的 `.ioc` 目前只把 PC13 配成了 `GPIO_Output`（`LED_RED`），这是 CubeMX
> 默认模板留下的占位配置——XPulse 板的 PC13 只引到 P2/P3 扩展排针，并没有 LED。
> 想让示例真正闪板载灯，请在该 `.ioc` 里把 PC2 设为 `GPIO_Output`（可选 label
> `LED_R`），生成代码后把 `main()` 里的 `LED_RED_*` 换成 `XP_LED_R_*` 即可。

## 板级引脚映射（`Core/Inc/xpulse.h`）

板上接线是固定的，所以把"信号 → 引脚"集中写进 `Core/Inc/xpulse.h`，代码里不再散落
`GPIO_PIN_x` / `GPIOx` 魔法数字。它是普通头文件，CubeMX **既不生成也不覆盖**；而对它的
`#include` 放在 `main.h` 的 `/* USER CODE BEGIN Includes */` 里，重新生成后依然保留。

命名沿用 CubeMX 的 `<NAME>_Pin` / `<NAME>_GPIO_Port` 形式，但加 `XP_` 前缀，避免以后在
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

XP_LED_ALL_OFF();          /* 先把三色灯全灭 */
XP_LED_R_ON();             /* 点亮红灯（PC2） */
HAL_GPIO_TogglePin(XP_LED_G_GPIO_Port, XP_LED_G_Pin);
```

> `xpulse.h` 只负责"引脚在哪"，引脚模式/AF/上下拉仍由 `.ioc` 决定：例如要用 SPI2，
> 先在 CubeMX 里把 PB13/PB14/PB15 配成 SPI2，再到代码里引用 `XP_EXT_SPI2_*`。

## 提交规范（commitlint）

仓库用 [commitlint](https://commitlint.js.org/) 约束提交信息，规范为 **Angular / Conventional
Commits**（规则集 `@commitlint/config-conventional`）。校验点在 Git 的 `commit-msg` 阶段，
由 Vite+ 的 Hook 转发器调用仓库内的 `.vite-hooks/commit-msg`，不合规的提交会被直接拒绝。

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


