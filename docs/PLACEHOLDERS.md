# 模板占位符与初始条件（PLACEHOLDERS）

本文件列出**复制本模板后必须人工核对的占位符**，以及无法靠代码消除的「初始条件」。
新建工程时请逐条过一遍；agent 拿到本模板后也应以本文件为清单先做核对。

> 约定：本模板中**没有任何**指向原作者机器的绝对路径是「可用的」——凡是绝对路径都属于占位符，
> 换机器必须重新确认，不要照抄。

---

## A. `scripts/init-project.mjs` 已经自动处理（本文件不重复）

工程名相关的替换由 `vpr init` / `pnpm run init` 一次性完成：根目录 `.ioc` 文件名、
`.ioc` 里的 `ProjectManager.ProjectName` / `ProjectManager.ProjectFileName`，以及
`README.md`、`.vscode/tasks.json`、`package.json`（slug）、`examples/INDEX.md` 里的引用。
具体用法见 [README 初始化章节](../README.md)。**本文件只补脚本覆盖不到的项**，不要在这里
再手工改一遍工程名，否则两处会打架。

目录名、`git remote`、探针、板级参数、CubeMX 安装路径、工具版本，脚本一律不管，见 §B。

---

## B. 必须人工核对（脚本未覆盖）

| # | 位置 | 当前值 / 占位符 | 应改成 | 说明 |
| - | ---- | -------------- | ------ | ---- |
| B1 | `platformio.ini` 的 `upload_protocol` / `debug_tool` | `stlink` / `stlink` | 换 CMSIS-DAP 时两项都改成 `cmsis-dap` | 默认探针是 ST-Link + SWD。本模板**刻意不入库** `launch.json`（已被 `.gitignore` 忽略），换探针时要自己配调试器的 `configFiles`，不要指望仓库里有现成文件。 |
| B2 | `boards/genericSTM32G474VE.json` 的 `mcu` / `maximum_size` / `maximum_ram_size` | `stm32g474vet6` / 524288 / 131072 | 换芯片型号时三处一起改 | `mcu` 同时决定启动文件与链接脚本的自动匹配，只改容量会烧错。还要同步 `.ioc` 的 `Mcu.CPN`、`Mcu.Name`、`Mcu.Package`、`ProjectManager.DeviceId` 并重新生成。 |
| B3 | `boards/*.json` 的 `f_cpu` | `16000000L` | 启用 [170 MHz 进阶配方](../README.md) 时改为 `170000000L` | 只影响 `F_CPU` 等工具链宏，**不改变 `.ioc` 里的真实时钟**；真实主频以 `.ioc` 的 RCC 为准，别只改这里就以为超频了。 |
| B4 | `.vscode/tasks.json` 的 `windows.command` / `linux.command` | `STM32CubeMX.exe` / `STM32CubeMX`（依赖 PATH） | 本机 CubeMX 不在 PATH、或安装到非默认目录时改成绝对路径 | 本机实测装在 `C:/ST/STM32CubeMX/STM32CubeMX.exe`（**不是**官方安装器默认的 `C:/Program Files/STMicroelectronics/...`）；这是本机值，换机器要重新确认。定位方法见 [CubeMX 工具实测](TOOLING-cubemx.md)。 |
| B5 | `platformio.ini` 的版本锁定 | `ststm32@20.0.0`、`framework-stm32cubeg4@1.6.3`、`toolchain-gccarmnoneeabi@1.70201.0`、`tool-ldscripts-ststm32@0.3.0`、`tool-openocd@3.1200.7` | 升级时要一起升级 | 与 CubeMX 的 FW_G4 组成验证组合，升级须**同时**验证「生成代码 + `pio run`」，不要单独升 HAL。 |
| B6 | `.ioc` 与 CubeMX 版本 | `.ioc` 写回 `MxCube.Version=6.18.1`、`MxDb.Version=DB.6.0.181` | 以 exe 属性 / 运行日志为准 | 版本戳可能是历史继承值；迁移后审阅工具写回并验证生成 + 构建。 |
| B7 | 仓库自身 | 默认仓库名 `template-mcu-STM32G474VET6-XPulse` | 你的仓库名 | `git remote -v` 仍指向模板仓库时，提交会推到模板去；新建仓库后确认 remote。 |
| B8 | `README.md` 第 1 行 | 模板默认名 | 你的项目名 | `vpr init` 会自动替换；若手工改过工程名，记得同步这一行。 |

---

## C. 环境前提（不是文件，但缺了就跑不起来）

| # | 前提 | 检查方式 | 缺失后果 |
| - | ---- | -------- | -------- |
| C1 | STM32CubeMX 6.x 已安装 | 看 `tasks.json` 里的命令能否启动；本机在 `C:/ST/STM32CubeMX/STM32CubeMX.exe` | 无法改 `.ioc`、无法生成 `Core/` |
| C2 | 已安装 FW_G4 包（V1.6.3，与 PlatformIO 包同版本） | CubeMX → Help → Manage embedded software packages；本机通常在 `<用户目录>/STM32Cube/Repository/STM32Cube_FW_G4_V1.6.3` | `project generate` 失败 |
| C3 | PlatformIO Core | `pio --version` | 无法构建 / 烧录 |
| C4 | 主机 GCC | `gcc --version` | L2 主机回归跑不了；`pio run` 不受影响 |
| C5 | Node.js ≥ 20 + pnpm / npm / `vpr` 之一 | `node --version`、`pnpm --version` | `scripts/` 里的初始化、clangd、删除示例命令与 commitlint 校验跑不了；**不参与固件编译** |
| C6 | 默认 ST-Link 走 SWD；目标板正确供电、共地，VTref 接线按探针定义 | 看探针型号与接线 | 探针与固件不匹配或目标无电会连接失败 |
| C7 | 首次 `pio run` 需联网下载 ststm32 平台与 CubeG4 包 | — | 离线首次构建失败 |

---

## D. 已知陷阱（不改也会踩）

| # | 陷阱 | 现状 | 建议 |
| - | ---- | ---- | ---- |
| D1 | 保存时重排 `Core/Src/main.c` | 本仓 `.vscode/settings.json` 目前**没有**开 `editor.formatOnSave`，但用户级 / 其它工作区设置或 clang-format 插件仍可能开启；一旦开启，保存生成文件会产生与 CubeMX 无关的巨大 diff | 处理 `Core/**` 前确认 formatOnSave 已关，或加 `.clang-format` + `// clang-format off` 保护，只保留与任务相关的 diff |
| D2 | `.ioc` 行尾 | CubeMX 写 **CRLF**；本仓 `.gitattributes` 是 `* text=auto eol=lf`，Git 入库 / checkout 与编辑器统一 LF，`text` 属性在 add / 比对时归一化 | 不必手工转回 CRLF；不要提交纯行尾变化，异常时用 `git ls-files --eol` 核对。混用 LF / CRLF 会让 `.ioc` 键解析错乱（见 [CubeMX 工具实测](TOOLING-cubemx.md) §3.9） |
| D3 | `.ioc` 内的参数白名单 | 每个外设一行 `<IP>.IPParameters=a,b,c`，每个引脚一行 `<Pin>.GPIOParameters=...`；只有列在白名单里的 key 才被识别 | 手改 `.ioc` 加参数时**必须同步补进对应白名单行**，否则 CubeMX 静默忽略。例如初始电平要 `PC2.GPIOParameters=PinState` + `PC2.PinState=GPIO_PIN_SET` 才生效。外设 IP 名以 CubeMX 重写后的写法为准（QSPI 是 `QUADSPI1`） |
| D4 | 信号名是 `S_` 前缀的枚举 | PA0 上 TIM2 通道 1 的正确写法是 `S_TIM2_CH1_ETR`，写成 `TIM2_CH1_ETR` 会失败 | 信号名一律以本机 CubeMX 器件数据库（`db/mcu/STM32G474V(B-C-E)Tx.xml` 等）为准，先查库再写 |
| D5 | CubeMX 对 `.ioc` 的错误是**非致命**的 | 信号名写错只在日志里留一行 `(OptionalMessage_ERROR) Cannot map signal ...`，`project generate` 依然返回成功，外设被静默丢弃 | 改过 `.ioc` 后**必须**核验：① 日志无 `Cannot map`；② 生成的 `main.c` 里有对应的 `MX_xxx_Init`。见 [工程规约](../AGENTS.md) §2.3 |
| D6 | `Drivers/` 被 git 忽略 | `.gitignore` 一行 `Drivers/` 排除整个目录；HAL / CMSIS 由 PlatformIO framework 提供 | 新克隆的仓库没有 `Drivers/` 是**预期**的。注意 git 不能 re-include 被排除目录下的文件，若确需加占位目录要同时放行目录本身；本模板不需要 |
| D7 | `ProjectManager.AskForMigrate` 被写回 `true` | 本仓 `.ioc` 已设为 `false`（D1 修复时一并改）；CubeMX 每次生成 / 保存都会写回 `true` | 无头生成若卡在 `Begin LoadConfig()` 后不再输出，多半是模态迁移框在等人点；先把它改成 `false` 再跑。见 [CubeMX 工具实测](TOOLING-cubemx.md) §3.4 |
| D8 | G474 最小工程也会有一条 `VP_RIF_VS_RIF1` 诊断 | 即使只保留 NVIC / RCC / SYS，`project generate` 仍可能打印 `(OptionalMessage_ERROR) Pin5 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU` | 与本工程引脚无关，属 CubeMX G4 数据库噪音；只要没有 `Cannot map` 且 `MX_xxx_Init` 齐全即可，不要为它删外设去「修」 |
| D9 | 重复生成会把 `syscalls.c` / `sysmem.c` 写成叠加路径 | 日志报 `FileNotFoundException: <工程目录>\D:\...\Core\Src\syscalls.c`，这两个文件不会更新 | 它们是标准 newlib 桩，改动很少；需要更新时从 FW_G4 模板工程里取，或先删除再生成 |
| D10 | 重新生成后 `Core/Src/main.c` 与 CubeMX 输出漂移 | 本仓这份 `main.c` 是 **120 行**，`USER CODE` 标记只有 **3 个**（`Header`、`WHILE`、`3` 三个块，即 6 行 BEGIN / END 注释），曾被人为精简，已不是当前 CubeMX 的纯输出；用**同一份 `.ioc`**（`TargetToolchain=Makefile` 不变）+ **同一版本 CubeMX 6.18.1** 重新生成，得到 **208 行 / 19 个标记**（2026-11-05 用 CubeMX 6.18.1 + 本仓 `.ioc` + `Makefile` 工具链，无头 `config load` + `project generate` 在临时目录复现：`grep -c "USER CODE BEGIN"` 得 19，仓库版得 3） | 重新生成会**保留** `USER CODE` 块里的用户代码（实测 `XPULSE_LED_R_ON(); HAL_Delay(500); ...` 那段仍在），但会整篇重写注释与空标记块，产生约 90 行与本意无关的 diff。这类 diff 必须**单独成一次提交**，不要和功能改动混在一起（[工程规约](../AGENTS.md) §1.3 原子性）；**不要**为了让 diff 变小去手工删注释 |

---

## E. 推荐的「改配置」流程（含校验）

按优先级：

1. **首选**：用 CubeMX GUI 改 `.ioc`，保存后 `project generate`。
2. **脚本化 / headless**：用 `java -jar STM32CubeMX.exe -q <script>.mx` 加载已有 `.ioc`
   再生成，工作目录必须是项目根目录（命令格式见 [CubeMX 工具实测](TOOLING-cubemx.md)）。
3. **手改 `.ioc`**：仅限小改动，且**必须**做完 D5 的两项核验；改完先在临时副本生成验证，
   别直接信「编辑成功」。
4. **生成后检查 `git status`**：CubeMX 可能覆盖 `.vscode/extensions.json`、
   写出 CRLF 的生成物、或把 `launch.json` 换成自动生成版本（后者本仓已忽略）。

任何 `.ioc` 改动都按 [工程规约](../AGENTS.md) 的 **L4 生成核验** 处理；影响编译的改动再补
L3 `pio run`，影响运行时行为的改动最终要 L5 实板确认。
