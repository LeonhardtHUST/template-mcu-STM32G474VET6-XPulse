# 示例目录说明与接入流程

本目录是**只读配方的整理层**：把 `docs/XPulse/例程/` 下 14 个厂商 Keil 工程的配置要点
提炼成可操作的步骤，方便往本模板（HSI 16 MHz 最小骨架）里搬外设。

- 本目录**不参与固件构建**：`platformio.ini` 的 `src_dir = Core/Src`，
  `examples/` 完全在编译范围之外。
- 本目录**不复制例程源码**，源码原件仍在 `docs/XPulse/例程/`。
- 本目录是**可选资料**，可以整目录删除，见文末「删除示例」。

## 目录结构

```text
examples/
├── INDEX.md              # 14 个厂商例程的总表（外设 / 复杂度 / 验证等级 / 链接）
├── README.md             # 本文件：说明 + 统一接入流程 + 配方模板
├── recipes/              # 高频外设配方，固定 4 栏
│   ├── GPIO.md
│   ├── Timer_PWM.md
│   └── USART.md
└── stdio_port/           # L2 主机回归样例（纯 C，不依赖 HAL）
    ├── stdio_port.c
    ├── stdio_port_test.c
    └── README.md
```

骨架 `Core/` 始终保持 CubeMX 生成的 **10 个文件、零功能代码**；任何例程的业务代码都不得
塞进 `Core/Src/main.c` 或覆盖 `Core/Inc/xpulse.h`。

## 统一接入路径（5 步）

所有配方都走同一条路，不要跳过第 1 步直接拷 `.c`：

1. **把外设配置抄进本仓根目录的 `.ioc`**（`Mcu.IP*`、`Mcu.Pin*`、`<Pin>.Signal`、
   `<IP>.IPParameters`、`<Pin>.GPIOParameters` 白名单一起补），然后在 CubeMX 里
   `project generate`。这是 [工程规约](../AGENTS.md) §2.1 / §2.3 要求的 L4 变更。
2. **核验生成物**：日志无 `Cannot map`、`Core/Src/main.c` 里出现对应的 `MX_xxx_Init()`。
3. **只拷源码进 `lib/<name>/`**，并配一份 `library.json`（显式声明 `srcDir` /
   `includeDir` / `srcFilter` / `includeDirs`），范式见
   [库封装范式](../docs/LIB-PATTERN.md)。Keil 样板文件一律丢掉。
4. **`pio run`**（L3），记录 Flash / RAM；编译告警不要靠 `-Werror` 硬压。
5. **按 L2 / L3 / L5 验证**：能主机编译的纯 C 先跑 L2；影响运行时的改动最终必须
   **L5 实板**确认（LED / 串口 / 波形），「编译通过」不等于「能用」。

## 配方模板

每篇配方固定 4 栏，按顺序读：

| 栏 | 内容 | 为什么 |
| --- | --- | --- |
| ① 对应 IP | 从例程自带 `.ioc` 的 `Mcu.IP*` / `Mcu.Pin*` 提取，IP 名以 CubeMX 重写后为准 | 决定往本仓 `.ioc` 里抄哪些键，白名单漏一个参数就静默失效 |
| ② 可复用文件清单 | 哪些 `.c` / `.h` 值得拷进 `lib/<name>/`，哪些是 Keil 样板要丢 | 避免把 `startup`、`system_stm32g4xx.c`、`.uvprojx` 混进构建 |
| ③ 与 PlatformIO 的差异 | 例程是 MDK-ARM；`.mxproject` / `Drivers` 均为 0 入库；**不能** `config load` 后直接生成、**不能** `pio run` | 防止把例程当可构建工程 |
| ④ 接入步骤 + 已知坑 | 上面 5 步的具体化，加陷阱文档链接 | 让每一步都能独立复核 |

## 与 PlatformIO 的结构差异（先看这个）

| 项 | 厂商例程（`docs/XPulse/例程/`） | 本模板 |
| --- | --- | --- |
| 工具链 | Keil MDK-ARM（`.uvprojx` / `.uvoptx`） | PlatformIO + GCC ARM（`platformio.ini`） |
| CubeMX 工具链选项 | `MDK-ARM V5.32` | `Makefile`（只为产出 `Core/`） |
| `ProjectManager.CoupleFile` | `true`（`gpio.c` / `tim.c` / `usart.c` 分文件） | `false`（`MX_xxx_Init()` 集中在 `main.c`） |
| 固件包 | FW_G4 V1.5.1（部分例程 6.9.1 生成） | `framework-stm32cubeg4@1.6.3`（PlatformIO 包） |
| `.mxproject` / `Drivers/` | 例程目录里可能有，但**本仓 0 入库** | 两者都在 `.gitignore` 中，HAL/CMSIS 由 framework 提供 |
| 构建命令 | 用 Keil 打开 `.uvprojx` | `pio run`（**对例程目录无效**） |

**两个明确禁止**：

- 不要 `config load docs/XPulse/例程/<X>/<X>.ioc` 后直接 `project generate`——旧版
  `.ioc` 会触发迁移改写、且路径/工程名与本仓不一致，生成物会散落别处（见
  [CubeMX 工具实测](../docs/TOOLING-cubemx.md) §3.2、§3.10）。
- 不要对 `docs/XPulse/例程/` 里的任何目录执行 `pio run`——它们没有 `platformio.ini`，
  也没有 PlatformIO 需要的板级定义与 framework。

## 已知坑速查

- [占位符清单 D2–D5](../docs/PLACEHOLDERS.md)：`.ioc` 行尾、参数白名单、`S_` 信号前缀、
  CubeMX 错误非致命会静默丢外设。
- [CubeMX 工具实测 §3.9 / §3.10](../docs/TOOLING-cubemx.md)：行尾混用导致键解析错乱、
  `QUADSPI1` 这类带实例号的 IP 名。
- [工程规约 §3](../AGENTS.md)：L0–L5 验证关卡与「构建通过不等于能用」。
- [板级引脚定义](../Core/Inc/xpulse.h)：`XPULSE_*` 宏只描述「信号 → 引脚」，模式 / AF /
  时钟归 `.ioc` 管。

## 删除示例

本目录是可选资料，删除只影响 `examples/` 自身：

```bash
vpr examples:remove
pnpm run examples:remove
npm run examples:remove
node scripts/remove-examples.mjs --dry-run   # 只看计划，不删
```

`vpr init` 时也会单独询问是否删除（默认 **N = 保留**），或用
`vpr init --no-examples` / `--keep-examples` 直接决定。删除脚本会同步清理
根目录 `README.md` 里 `<!-- BEGIN EXAMPLES -->` / `<!-- END EXAMPLES -->` 标记的整个区块，
避免留下死链。`docs/XPulse/例程/` 下的 14 个原始工程**不会**被触碰。
