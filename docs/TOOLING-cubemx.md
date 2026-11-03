# 用 AI/CLI 修改 STM32CubeMX 配置：本机工具与 G4 实测

本文记录**本机（Leon 的 Windows 开发机）**已有的 CubeMX 自动化能力，以及把本模板
从零生成出来时**实际观察到的行为**。目的是让任何 agent（不限客户端）复用同一套结论，
不必重新试错。未验证的推测会明确标注。

---

## 1. 本机工具现状

| 项 | 值 |
| -- | -- |
| CubeMX 可执行文件 | `C:\ST\STM32CubeMX\STM32CubeMX.exe`（**不在** `C:\Program Files\STMicroelectronics\...`） |
| 实际版本 | exe 自报 **6.18.1**；`.ioc` 写回 `MxCube.Version=6.18.1`、`MxDb.Version=DB.6.0.181` |
| 自带 JRE | `C:\ST\STM32CubeMX\jre\bin\java.exe` |
| G4 固件包 | `C:\Users\Leon\STM32Cube\Repository\STM32Cube_FW_G4_V1.6.3` |
| 器件数据库 | `C:\ST\STM32CubeMX\db\mcu\STM32G474V(B-C-E)Tx.xml` 等，**信号名以此为准** |
| PlatformIO | Core 6.2.0；`ststm32@20.0.0`；`framework-stm32cubeg4@1.6.3`（与 FW_G4 同版本） |
| 主机 GCC | MinGW gcc 15.2.0（仅用于 `test/` 回归） |

> 判断 CubeMX 版本**看 exe 属性或运行日志**，不要相信 `.ioc` 里的版本戳：
> 旧模板会把自己的历史戳原样继承下来。本模板的 6.18.1 是本次生成写回的真实值。

## 2. 直接调 CLI

```powershell
& "C:\ST\STM32CubeMX\jre\bin\java.exe" -jar "C:\ST\STM32CubeMX\STM32CubeMX.exe" -q "D:\path\to\script.mx"
```

脚本每行一条命令，本模板用到的：

```text
load STM32G474VETx                 # 从 MCU 裸建（不带 .ioc）
config load <相对或绝对 .ioc 路径>  # 加载已有配置
project name stm32-g474-template
project toolchain Makefile
project path <工程根目录>
project generate
exit
```

单次生成的耗时主要花在 JVM 启动与 CMSIS 拷贝上，本机约 **1 分钟量级**；超时阈值不要设太小。

## 3. 本次生成模板时实测到的行为

以下每条都在 2026-11-03 的生成日志或生成物里核对过。

> 编注：本文初版基于 Penguins-G474 板的模板首建；2026-11-03 板卡换成 X Pulse 后，
> §3.8–§3.11 是换板 patch 期间的新实测。§3.5–§3.7 中的 PC13 属当时的历史事实。

### 3.1 `project path` 会再追加一层工程名

`project path D:\scratch\gen` + `project name foo` → 产物落在 `D:\scratch\gen\foo\`。
想让文件落在仓库根目录，要么生成后上移，要么在 `.ioc` 已存在时用 `config load` + `project generate`
（后者按 `.ioc` 所在目录生成）。

### 3.2 原地生成让 `.mxproject` 变成相对路径

先 `project path` 指到别处生成，`.mxproject` 的 `[PreviousUsedMakefileFiles]` 会写入**绝对路径**；
把 `.ioc` 放进仓库根目录后执行 `config load` + `project generate`，同文件变成
`Core\Src\main.c` 这类相对路径。**提交前确认 `.mxproject` 里没有盘符路径**（`grep "D:"` 应为 0）。

### 3.3 `AskForMigrate` 会被写回 `true`

模板里提交的是 `ProjectManager.AskForMigrate=false`，但 CubeMX 每次生成/保存都会把它改回
`true`。无头模式如果卡在 `Begin LoadConfig()` 后不再输出，先把它改成 `false` 再跑
（这是模态迁移框在等人点）。

### 3.4 `syscalls.c` / `sysmem.c` 会被写成叠加路径

生成日志出现：

```text
FileStreamsReadnWrite: java.io.FileNotFoundException:
  <工程目录>\D:\Projects\...\Core\Src\syscalls.c (拒绝访问。)
```

这两个文件因此**不会更新**（其余文件正常）。它们是标准的 newlib 桩，本模板从 F103 模板
复制了等价文件；要更新就从 `STM32Cube_FW_G4_V1.6.3` 的模板工程里取。

### 3.5 GPIO 参数要写进 `GPIOParameters` 白名单

本次把 PC13 设成"输出 + 初始高电平"，生效的写法是**两行一起**：

```ini
PC13.GPIOParameters=PinState
PC13.PinState=GPIO_PIN_SET
PC13.Signal=GPIO_Output
```

重新生成后 `main.c` 里确实出现 `HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)`。
同理，外设参数要进 `<IP>.IPParameters`、引脚清单要同步 `Mcu.Pin*` 与 `Mcu.PinsNb`。

### 3.6 G474 最小工程也会有一条 `VP_RIF_VS_RIF1` 诊断

即使 `.ioc` 里只有 NVIC/RCC/SYS，生成日志仍有：

```text
[INFO] ImportTextPane:234 - (OptionalMessage_ERROR) Pin5 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU
```

- 它**不是** `Cannot map signal`，与本工程的 PC13 / SWD 配置无关；
- 加/减普通 GPIO 不会让它消失（本次两次生成都有）；
- 判定"配置是否生效"仍以**生成物**为准：日志无 `Cannot map` + `main.c` 里有对应
  `MX_xxx_Init()`。来源未定位，不要为它删外设。

### 3.7 其它已知噪音

- `Updater plugin not ready yet` / `Updater is busy`：更新器启动噪音，不阻塞生成；
- 首次在新版本上运行可能弹一次迁移/提示框；
- `config saveas` 会同时改 `ProjectName` / `ProjectFileName`，想原地规范化用 `config save`。

### 3.8 `.mx` 脚本里带空格的路径必须加双引号（2026-11-03 实测）

`config load D:\...\X Pulse开发板参考资料\xxx.ioc` 会在**第一个空格处截断**，
CubeMX 弹出 `D:\...\X doesn't exist` 对话框。正确写法：

```text
config load "D:\Projects\GitHub\NUEDC_2027\X Pulse开发板参考资料\例程\QSPI_W25Q_Flash\QSPI_W25Q_Flash.ioc"
```

### 3.9 行尾混用会让 `.ioc` 键解析错乱、参数静默丢弃（2026-11-03 实测）

CubeMX 写出的 `.ioc` 是 **CRLF**；用编辑工具往里插入 **LF 行尾**的键值行后，
加载阶段出现 `(OptionalMessage_ERROR) Unknown IP (QUADSPI11)`（键名多出垃圾后缀），
该外设的全部 `IPParameters` 被静默丢弃，生成物退回默认值，而 `project generate` 依然返回 OK。
修复：把整个文件行尾统一（或整文件重写）后重新生成即恢复正常。
**教训：对 `.ioc` 做文本 patch 时，要么整文件重写并保持单一行尾，要么 patch 后先在临时目录
生成验证一遍，别直接信"编辑成功"。**

### 3.10 外设 IP 名以 CubeMX 重写后的写法为准（2026-11-03 实测）

G474 的 QSPI 在 `Mcu.IP*` 清单里必须写 **`QUADSPI1`**（带实例号），
参数行前缀同样是 `QUADSPI1.*`；写成 `Mcu.IP1=QUADSPI` 会报 `Unknown IP (QUADSPI)` 并被改写。
手写 `.ioc` 时若拿不准，先让 CubeMX 加载一次旧版例程 `.ioc` 再看它重写保存的结果。
另外：旧版本（6.9.1）例程 `.ioc` 加载时会触发 "Adapt Old IOC" 迁移规则自动修正键名，
而自标新版版本戳的文件不走迁移、按严格路径解析——同一个键在两条路径下宽容度不同。

### 3.11 CubeMX 生成会覆盖 `.vscode/launch.json` 与 `extensions.json`（2026-11-03 实测）

`project generate` 后，STM32 VS Code 扩展格式的自动生成文件会替换模板自有的
Cortex-Debug `launch.json`（注释标记 "AUTOMATICALLY GENERATED FILE"），
`extensions.json` 被重排且把 `cpptools-extension-pack` 挪进 unwantedRecommendations。
**生成后检查 `git status`**：这两个文件属于模板，被覆盖时用 `git checkout -- .vscode/` 恢复。
Makefile、startup、链接脚本、`Drivers/` 的再生成产物已被 `.gitignore` 覆盖，无需处理。

### 3.12 RCC 频率信息行的告警属无害噪音（2026-11-03 实测）

`IP (RCC) : Invalid parameter (FamilyName)`、`Parameter (RNGFreq_Value/USBFreq_Value) has
invalid value (170000000)` 会在加载含 170 MHz 时钟树的 `.ioc` 时出现。它们是频率信息行的
校验告警（RNG/USB 内核时钟上限低于 170 MHz，但本工程未启用这两个外设），生成物不受影响；
判定仍以「日志无 `Cannot map` + `main.c` 有对应 `MX_xxx_Init` 且参数正确」为准。

## 4. 可选：`vscode-cube-mcp`（本机已全局安装）

| 项 | 值 |
| -- | -- |
| 包 | `vscode-cube-mcp` v0.4.2（MIT，PyPI） |
| 可执行文件 | `C:\Users\Leon\.local\bin\vscode-cube-mcp.exe` |
| 环境变量（用户级） | `ST_CUBEMX_EXE=C:\ST\STM32CubeMX\STM32CubeMX.exe`、`ST_CUBEMX_ALLOWED_ROOTS=D:\Projects;C:\Users\Leon\WorkBuddy`、`ST_CUBEMX_TIMEOUT=300` |

它提供 `cubemx_load`（只读回读）、`cubemx_configure`（带备份写回）、`cubemx_generate`、
`cubemx_new_project`、`cubemx_remove_peripheral` 等工具，并强制 `.ioc` 路径白名单。
`cubemx_configure` 是**纯脚本透传**，不做文本注入；真正能配外设的是 `cubemx_new_project`
的 `commands` 参数。

`ALLOWED_ROOTS` 是安全白名单：换工作目录时要一起改，否则会报"路径不在允许范围内"。

## 5. 不推荐再装的方案

| 方案 | 结论 | 理由 |
| --- | --- | --- |
| `stm32cubemx-mcp`（Wafleem） | 不装 | 代码生成走 STM32CubeIDE，与 CubeMX CLI 不是一回事；能力已被 `vscode-cube-mcp` 覆盖 |
| `stm32pio`（PyPI） | 不装 | 2021 年后未维护，且面向"用 CubeMX 重建 PIO 工程"，会与本模板既有的 `platformio.ini` 冲突 |
| `STM32Cube-MCP`（brovk2008） | 不装 | 大部分工具是烧录/GDB/串口 GUI 自动化，远超"改配置"需求 |
