# 目录结构与代码归属

> 本文说明「代码该写在哪里」以及 `platformio.ini` 为什么这样配。
> `lib/<name>/` 的封装范式另见 [库封装范式](LIB-PATTERN.md)，两者编译模型不同。

## 1. 谁拥有哪些目录

| 路径 | 归属 | 放什么 | 参与编译的方式 |
| --- | --- | --- | --- |
| `.ioc` | 人 + CubeMX | 引脚 / 时钟 / 外设配置，**唯一事实源** | — |
| `Core/` | CubeMX | 生成物（`main.c`、`*_it.c`、`*_msp.c` 等）；USER CODE 块内只留转调胶水 | `Core/Src/*.c` 全编 |
| `Core/Inc/xpulse.h` | 人 | 板级「信号 → 引脚」映射 | 头文件，CubeMX 不生成 |
| `app/` | 人 | **业务代码**：状态机、协议、控制逻辑、点灯 | `app/src/**` 全编，继承 `-Werror` |
| `lib/<name>/` | 人 | 可复用 / 第三方组件 | 仅被 `#include` 链拉入时编译，或由 `lib_deps` 登记 |
| `boards/` `docs/` `scripts/` `examples/` | 人 | 板卡定义、文档、脚本、可选参考资料 | 不参与 |

## 2. 为什么业务代码放 `app/` 而不是 `lib/`

`app/` 在 `src_dir` 内，因此三件事自动成立：源文件**无条件编译**（写坏了立刻报错，
不会静默躺着）、继承 `build_src_flags` 的 `-std=c11 -Wall -Wextra -Werror`、
与 `Core/Inc` 共用一套 include 路径。`lib/` 的按需编译是 PlatformIO 的设计意图，
适合第三方组件，但用来自写业务代码会出现「未被 `#include` 就不编译」的静默盲区，
细节见 [库封装范式](LIB-PATTERN.md) §2.1 / §2.2。

在 `Core/` 的 USER CODE 块内写业务代码**本来是安全的**（`.ioc` 的
`ProjectManager.KeepUserCode=true`，重生成会保留块内内容）。把业务搬到 `app/`
不是消除覆盖风险，而是避免逻辑与生成物混在一起、并在 19 个 USER CODE 块之间
反复找位置；`Core/` 只留三行胶水后，日常改动集中在单一目录。

## 3. `src_dir = .` 的三条硬约束

`platformio.ini` 把 `src_dir` 设为项目根，`app/` 才能与 `Core/Src` 共用
`build_src_flags`。代价是下面三条**不能省**：

1. **`build_src_filter` 必须写 `+<...>` 白名单**。用 `+<*>` 会把项目树深处
   （`docs/`、`node_modules/` 等）的 `.c` 一起拉进编译——已用故意写错的
   `evil_probe.c` 实测复现。
2. **排除项要写全相对路径**：`-<Core/Src/system_stm32g4xx.c>`。沿用旧的
   `-<system_stm32g4xx.c>` 在 `src_dir = .` 下不生效，该文件会与
   `framework-stm32cubeg4` 的 CMSIS 重复定义符号（[工程规约](../AGENTS.md) §2.5）。
3. **头文件路径用相对写法** `-Iapp/inc`。写成 `-I${PROJECT_DIR}/app/inc`
   会被拼成 `...\ststm32\builder\...` 的畸形路径（实测，报找不到头文件）。

`+<app/src/*>` 会递归收进子目录，所以在 `app/src/` 下新增文件**不需要**改
`platformio.ini`。

## 4. `main.c` 里仅有的三处胶水

| USER CODE 块 | 内容 |
| --- | --- |
| `BEGIN Includes` | `#include "app_example.h"` |
| `BEGIN 2` | `app_init();` |
| `BEGIN WHILE` | `while (1) { app_loop(); }` |

业务逻辑全部在 `app/src/app_example.c`。CubeMX 重生成会保留块内内容，
但改完仍应执行 `grep -n "app_" Core/Src/main.c` 确认三处都在；重生成相关的
其它副作用见 [占位符清单](PLACEHOLDERS.md) D10 / D11。

## 5. 改了目录之后

- **必须重生成 `compile_commands.json`**（`vpr clangd` 或 `pio run -t compiledb`）。
  该文件被 `.gitignore` 忽略、不进提交，但 `.clangd` 指向它；不重生成的话
  VS Code 里 `app/` 源码没有补全与跳转。
- 新增 `app/` 源文件后跑一次 `pio run`（L3）；影响运行时行为的改动按
  [工程规约](../AGENTS.md) §3 还需 **L5 实板确认**，构建通过不等于能用。
- 若改用 `lib/<name>/` 存放组件，记得它不继承 `-Werror`、且未被引用的库不参与编译。

## 6. 本仓落地记录（2026-11-06）

`pio run -e genericSTM32G474VE` → SUCCESS。两套体积口径请勿混用：

| 口径 | 改造前 | 改造后 |
| --- | --- | --- |
| `-Wl,--print-memory-usage` | RAM 1584 B / FLASH 3740 B | RAM 1584 B / FLASH 3776 B |
| PlatformIO size 报告 | 44 B / 3260 B | 44 B / 3296 B |

+36 B 是 `app/` 实际链入的代码。`arm-none-eabi-nm` 可见 `app_init` / `app_loop`
符号（示例函数 `app_example_to_hust` 在 `-Os` 下被内联，不作为独立符号出现）。
`compile_commands.json` 由 92 条增至 93 条，新增条目里可核对
`-Wall -Wextra -Werror -Iapp/inc`。

**L3 / L5 状态（按 [工程规约](../AGENTS.md) §3 区分）**：

- **L3 编译通过**：本会话执行 `pio run -e genericSTM32G474VE` → SUCCESS，体积见上表。
- **L5 实板已确认**：使用者于 2026-11-06 连接硬件、烧录后人工确认板载红灯按
  500 ms 间隔闪烁正常。证据来源是使用者实板回报，**本会话未执行烧录，仓内也
  没有烧录命令日志**；引用这一结论时不要写成「已执行 `pio run -t upload`」。

改了 `app/src/app_example.c` 里的运行时逻辑后，仍需重新做 L5，不能复用本次结论。
