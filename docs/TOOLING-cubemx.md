# STM32CubeMX 命令行与 G4 实测结论

本文记录用 CLI / AI 改 `.ioc` 时**实际观察到的行为**，供任何 agent 复用同一套结论，
不必重新试错。结论针对本仓库（B）的 CubeMX 6.18.1 + FW_G4 V1.6.3 组合；
路径类结论已与本仓 `.vscode` / `.gitignore` 现状对齐。未验证的推测会明确标注。

> 本仓默认 `.ioc` 是 HSI 16 MHz 最小骨架（3 IP / 4 引脚）。文中的 170 MHz 配方只是
> **可选进阶**（见 [README](../README.md)），不是当前默认值，不要按它去反推 `.ioc` 现状。

---

## 1. 工具现状（本机实测值，换机器需重新确认）

| 项 | 值 |
| -- | -- |
| CubeMX 可执行文件 | `C:/ST/STM32CubeMX/STM32CubeMX.exe`（**不在** `C:/Program Files/STMicroelectronics/...`） |
| 实际版本 | exe 自报 **6.18.1**；`.ioc` 写回 `MxCube.Version=6.18.1`、`MxDb.Version=DB.6.0.181` |
| 自带 JRE | `C:/ST/STM32CubeMX/jre/bin/java.exe` |
| G4 固件包 | `<用户目录>/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.3` |
| 器件数据库 | `C:/ST/STM32CubeMX/db/mcu/STM32G474V(B-C-E)Tx.xml` 等，**信号名以此为准** |
| PlatformIO | Core 6.2.0；`ststm32@20.0.0`；`framework-stm32cubeg4@1.6.3`（与 FW_G4 同版本） |
| 主机 GCC | MinGW gcc 15.2.0（仅用于 L2 主机回归） |

> 判断 CubeMX 版本**看 exe 属性或运行日志**，不要相信 `.ioc` 里的版本戳：
> 旧模板会把自己的历史戳原样继承下来。上表路径都是本机值，属于
> [占位符](PLACEHOLDERS.md) §B，不是可直接照抄的常量。

## 2. 直接调 CLI

```powershell
& "C:/ST/STM32CubeMX/jre/bin/java.exe" -jar "C:/ST/STM32CubeMX/STM32CubeMX.exe" -q "<脚本路径>.mx"
```

`.mx` 脚本每行一条命令，本模板会用到的最小集合：

```text
load STM32G474VETx                 # 从 MCU 裸建（不带 .ioc）
config load <相对或绝对 .ioc 路径>  # 加载已有配置
project name my-board
project toolchain Makefile
project path <工程根目录>
project generate
exit
```

对**已存在 `.ioc`** 的工程，用 `config load` + `project generate` 原地生成即可，
不要从 `load <MCU>` 重建（会丢掉全部引脚与时钟配置）。

### 2.1 仓库自带的 `.mx` / OpenOCD 脚本

`scripts/` 下提供开箱即用的脚本，工作目录均为项目根目录（CubeMX 的 `config load`
用相对路径时按当前工作目录解析）：

| 脚本 | 内容 | 用途 |
| -- | -- | -- |
| `scripts/cubemx-generate.mx` | `config load <当前 .ioc>` + `SetStructure Advanced` + `project generate` + `exit` | 无头重新生成 `Core/`，等价于 §2 的 `config load` + `project generate` |
| `scripts/cubemx-refresh.mx` | `swmgr refresh` + `exit` | 刷新固件包管理器索引（安装 / 升级 FW_G4 后跑一次），不生成代码 |
| `scripts/openocd-stlink.cfg` / `scripts/openocd-cmsis-dap.cfg` | `source [find interface/...]` + `transport select swd` + `source [find target/stm32g4x.cfg]` | Cortex-Debug 的 OpenOCD 探针配置，见 [Cortex-Debug 调试示例](DEBUG-CORTEX.md) |

调用示例（PowerShell）：

```powershell
& "C:/ST/STM32CubeMX/jre/bin/java.exe" -jar "C:/ST/STM32CubeMX/STM32CubeMX.exe" -q "scripts/cubemx-generate.mx"
```

- **改名后由 init 脚本同步**：`cubemx-generate.mx` 里硬编码了当前 `.ioc` 文件名，已加入
  `scripts/init-project.mjs` 的 `targets`（`mode: 'replace'`、`optional: true`）。执行
  `vpr init <新名字>` 时会和 `README.md` / `.vscode/tasks.json` / `package.json` 一起替换
  旧名，改名后脚本不会失效；手工只改 `.ioc` 名而漏改这里会指向不存在的文件。
- 不提供 `cubemx-init.mx`：那是「从裸 MCU 建工程」用的，含 `__PROJECT_DIR__` 绝对路径
  占位符，本仓已有完整 `.ioc`，不需要重建。
- 生成前确认 `.ioc` 的 `ProjectManager.AskForMigrate=false`（§3.4、[占位符](PLACEHOLDERS.md)
  D7），生成后 CubeMX 会写回 `true`，下次无头生成前记得改回 `false`。
- `.mx` 脚本里的路径若含空格，`config load` 的参数必须加双引号（§3.8）。

---

## 3. 十二个实测小节

### 3.1 CLI 直调（`-q` 脚本模式）

`-q` 是无头脚本模式：每行一条命令、顺序执行、结尾必须 `exit`。单次生成的耗时主要花在
JVM 启动与 CMSIS 拷贝上，本机约 **1 分钟量级**，超时阈值不要设太小。脚本执行返回 0
**不代表配置正确**——见 §3.5、§3.9：CubeMX 对 `.ioc` 的错误是非致命的，
`project generate` 照样返回成功。判定永远以**生成物 + 日志关键字**为准。

### 3.2 `project path` 会再追加一层工程名

`project path D:/scratch/gen` + `project name my-board` → 产物落在
`D:/scratch/gen/my-board/`。想让文件落在仓库根目录，要么生成后上移，
要么在 `.ioc` 已存在时用 `config load` + `project generate`（后者按 `.ioc` 所在目录生成）。
本仓用后者。

### 3.3 原地生成让 `.mxproject` 变成相对路径

先 `project path` 指到别处生成，`.mxproject` 的 `[PreviousUsedMakefileFiles]` 会写入
**绝对路径**；把 `.ioc` 放进仓库根目录后执行 `config load` + `project generate`，同文件变成
`Core\Src\main.c` 这类相对路径。本仓把 `.mxproject` 列入 `.gitignore`（它是生成物的元数据、
含本机路径），因此不入库；但若临时需要检查，确认里面没有盘符路径（`grep "D:"` 应为 0）。

### 3.4 `AskForMigrate` 会被写回 `true`

模板里若提交为 `ProjectManager.AskForMigrate=false`，CubeMX 每次生成 / 保存都会改回
`true`。本仓 `.ioc` 已设为 `false`（D1 修复时一并改，便于无头生成）；被写回 `true`
时再改回 `false`。无头模式如果卡在 `Begin LoadConfig()` 后不再输出，先把它改成
`false` 再跑（这是模态迁移框在等人点）。详见 [占位符](PLACEHOLDERS.md) D7。

### 3.5 `syscalls.c` / `sysmem.c` 会被写成叠加路径

生成日志出现：

```text
FileStreamsReadnWrite: java.io.FileNotFoundException:
  <工程目录>\D:\...\Core\Src\syscalls.c (拒绝访问。)
```

这两个文件因此**不会更新**（其余文件正常）。它们是标准 newlib 桩，改动很少；
要更新就从 `STM32Cube_FW_G4_V1.6.3` 的模板工程里取，或先删除再生成。

### 3.6 GPIO 参数要写进 `GPIOParameters` 白名单

让某个引脚「输出 + 初始高电平」生效，必须是**白名单行 + 值行一起写**：

```ini
PC2.GPIOParameters=PinState,GPIO_Label
PC2.PinState=GPIO_PIN_SET
PC2.GPIO_Label=LED_R
PC2.Signal=GPIO_Output
```

重新生成后 `main.c` 才会出现对应的 `HAL_GPIO_WritePin(...)`。同理，外设参数要进
`<IP>.IPParameters`、引脚清单要同步 `Mcu.Pin*` 与 `Mcu.PinsNb`，否则被静默忽略。

> 本仓默认 `.ioc` 已把 **PC2** 配成 `GPIO_Output`（`PC2.GPIOParameters=GPIO_Label`
> + `PC2.GPIO_Label=LED_R`），对应板载三色灯的红色通道；板载三色灯是 PC0 / PC1 /
> PC2（见 `Core/Inc/xpulse.h`，共阳低电平点亮）。**PC13 在 X Pulse 板上只引到扩展排针，
> 没有 LED**，旧的 `LED_RED` / PC13 占位配置已在 D1 中移除。改这类配置属于 L4 变更，
> 改动后须按 §4 判定清单做生成核验。

### 3.7 G474 最小工程也会有一条 `VP_RIF_VS_RIF1` 诊断

即使 `.ioc` 里只有 NVIC / RCC / SYS，生成日志仍可能打印：

```text
[INFO] ImportTextPane:234 - (OptionalMessage_ERROR) Pin5 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU
```

- 它**不是** `Cannot map signal`，与本工程的引脚配置无关；
- 加 / 减普通 GPIO 不会让它消失；
- 判定「配置是否生效」仍以**生成物**为准：日志无 `Cannot map` + `main.c` 里有对应
  `MX_xxx_Init()`。来源未定位，不要为它删外设。

### 3.8 `.mx` 脚本里带空格的路径必须加双引号

```text
config load D:/work/my board/xxx.ioc
```

会在**第一个空格处截断**，CubeMX 弹出 `D:/work/my doesn't exist` 对话框。正确写法：

```text
config load "D:/work/my board/xxx.ioc"
```

本仓 `docs/XPulse/例程/` 路径本身不含空格，但从别处引用参考资料时很容易踩到。
另外 `config saveas` 会同时改 `ProjectName` / `ProjectFileName`，想原地规范化用
`config save`。

### 3.9 行尾混用会让 `.ioc` 键解析错乱、参数静默丢弃

CubeMX 写出的 `.ioc` 是 **CRLF**；用编辑工具往里插入 **LF 行尾**的键值行后，
加载阶段可能出现 `(OptionalMessage_ERROR) Unknown IP (QUADSPI11)`（键名多出垃圾后缀），
该外设的全部 `IPParameters` 被静默丢弃，生成物退回默认值，而 `project generate`
依然返回 OK。本仓 `.gitattributes` 是 `* text=auto eol=lf`，Git 入库 / checkout 与编辑器
统一 LF，`text` 属性在 add / 比对时归一化，**不要手工转回 CRLF**。

**教训：对 `.ioc` 做文本 patch 时，要么整文件重写并保持单一行尾，要么 patch 后先在
临时副本生成验证一遍，别直接信「编辑成功」。**

### 3.10 外设 IP 名以 CubeMX 重写后的写法为准

G474 的 QSPI 在 `Mcu.IP*` 清单里必须写 **`QUADSPI1`**（带实例号），参数行前缀同样是
`QUADSPI1.*`；写成 `Mcu.IP1=QUADSPI` 会报 `Unknown IP (QUADSPI)` 并被改写。
旧版本（6.9.1）例程 `.ioc` 加载时会触发 "Adapt Old IOC" 迁移规则自动修正键名，
而自标新版版本戳的文件不走迁移、按严格路径解析——同一个键在两条路径下宽容度不同。
手写 `.ioc` 时若拿不准，先让 CubeMX 加载一次旧版例程 `.ioc` 再看它重写保存的结果。
本仓 `docs/XPulse/例程/QSPI_W25Q_Flash/QSPI_W25Q_Flash.ioc` 里就是 `Mcu.IP1=QUADSPI1`。

### 3.11 CubeMX 生成会覆盖 `launch.json` 与 `extensions.json`

`project generate` 后，STM32 VS Code 扩展格式的自动生成文件会替换 Cortex-Debug 风格的
`launch.json`（注释标记 `AUTOMATICALLY GENERATED FILE`），`extensions.json` 被重排且可能把
`cpptools-extension-pack` 挪进 `unwantedRecommendations`。**与本仓现状对齐的结论**：

- `.vscode/launch.json` 已在 `.gitignore` 中忽略、本仓刻意不入库（本机那份就是
  PlatformIO 自动生成的）；被覆盖**不会污染版本库**，不需要 `git checkout` 恢复。
- `.vscode/extensions.json` **是入库文件**；被重排时用
  `git checkout -- .vscode/extensions.json` 恢复。
- `.vscode/c_cpp_properties.json` 也已忽略；`Makefile`、startup、链接脚本、
  `Drivers/` 的再生成产物同样被忽略，无需处理。
- 生成后照例检查 `git status`，确认只有预期的 `.ioc` / `Core/` 变更。

### 3.12 RCC 频率信息行的告警属无害噪音

`IP (RCC) : Invalid parameter (FamilyName)`、`Parameter (RNGFreq_Value/USBFreq_Value) has
invalid value (170000000)` 会在加载含 170 MHz 时钟树的 `.ioc` 时出现。它们是频率信息行的
校验告警（RNG / USB 内核时钟上限低于 170 MHz，但最小骨架未启用这两个外设），
生成物不受影响；判定仍以「日志无 `Cannot map` + `main.c` 有对应 `MX_xxx_Init`
且参数正确」为准。默认 HSI 16 MHz 骨架通常不会出现这些行。

---

## 4. 判定清单（每次生成后都过一遍）

1. 日志无 `Cannot map` / 与本次改动相关的 `OptionalMessage_ERROR`（§3.7、§3.12 的噪音除外）。
2. 生成的 `Core/Src/main.c` 里有本次新增外设的 `MX_xxx_Init()`，参数与 `.ioc` 一致。
3. `git status` 只出现预期文件；`.vscode/extensions.json` 没被重排（§3.11）。
4. 任何 `.ioc` 改动按 [工程规约](../AGENTS.md) 记为 **L4 生成核验**，再补 L3 `pio run`。
