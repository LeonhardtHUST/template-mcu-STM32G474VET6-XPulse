# AGENTS.md

给在本仓库（以及由本模板派生出的工程）里工作的 AI agent 与协作者的约束与约定。
**这不是建议，是约束**；与本文冲突的做法请先改本文再动手。

- 仓库定位：STM32G474VET6 / X Pulse 板的**最小工程模板**，默认 `.ioc` 是
  HSI 16 MHz 骨架（3 IP / 4 引脚），工作流 = STM32CubeMX + PlatformIO + VS Code。
- 复制模板后必须人工核对的占位符与已知陷阱：[占位符清单](docs/PLACEHOLDERS.md)。
- CubeMX 命令行与 G4 实测坑：[CubeMX 工具实测](docs/TOOLING-cubemx.md)。
- 板级「信号 → 引脚」定义在 `Core/Inc/xpulse.h`，库封装范式见
  [库封装范式](docs/LIB-PATTERN.md)。
- 代码该写在哪、为什么 `src_dir = .`：[目录结构](docs/LAYOUT.md)。

---

## 1. 提交规范（中文 Conventional Commits）

仓库已接入 [commitlint](https://commitlint.js.org/)（规则集
`@commitlint/config-conventional`，另由本仓库 `commitlint.config.mjs` 追加「简述含汉字」校验
（只要求至少一个汉字，不限首字），Git `commit-msg` 钩子由 `vpr hooks:install` 转发到
`.vite-hooks/commit-msg`）。**两者分工互补**：

| 谁 | 管什么 |
| --- | --- |
| `commitlint` 钩子 | **机器校验提交信息的格式**（type / scope / subject 是否合规），不合规直接拒绝 |
| 本文件 `AGENTS.md` | **代码与 CubeMX 生成边界**、验证关卡、目录与版本约定；钩子管不到这些 |

### 1.1 格式

```text
<type>(<scope>): <中文简述>

<可选正文：为什么这么改>
```

- `<type>` 只能是：`feat` `fix` `docs` `chore` `refactor` `test` `build` `ci` `perf` `style` `revert`。
- `<scope>` 可选，用文件名 / 模块名：`ioc` `pio` `app` `board` `vscode` `scripts` `core` `git` `docs` `xpulse` `deps`。
- **简述用中文**（至少含一个汉字；首字不限，`Q4` / `GPIO` / `128K` 开头亦可）、动词开头、不超过 50 字、结尾不加句号；正文说清「为什么」，不要复述 diff。
- 破坏性变更写成 `feat(core)!: ...`，或在 footer 写 `BREAKING CHANGE: ...`。
- 提交信息里如实写明本次改动的**验证等级**（见 §3），没验证就写「未验证」。
- 提交信息里不要加 AI 署名尾注或任何自动生成的署名行。

### 1.2 正例 / 反例

```text
feat(ioc): 启用 HSE 8MHz 与 PLL 得到 170MHz 主频
fix(scripts): 修正初始化脚本删除示例时的路径校验
docs: 补充模板占位符清单与已知陷阱
chore(git): 固定行尾策略，消除 CRLF 整文件 diff
```

```text
update                                  # 没有类型、没有信息量
fix: 修改了一些东西                      # 没说改了什么、为什么
Fix Bug.                                # 类型大小写错、末尾句号、无中文简述
feat: 加了很多功能还顺手格式化全仓库     # 不原子
```

### 1.3 原子性与硬性禁止

- 一个提交只做一件事；**禁止**把「格式化 / 行尾归一」和「功能改动」塞进同一个提交。
- **禁止** `git push --force`（含 `-f`）；确需改写历史必须先问人。
- **禁止** `--no-verify` 跳过钩子。
- **禁止** `--no-gpg-sign` / `-c commit.gpgsign=false`（本机已配置 SSH 提交签名）。

---

## 2. 工程硬约束（12 条）

1. **`.ioc` 是引脚 / 时钟 / 外设的唯一事实来源**。`Core/` 下的生成文件是产物，不要手改成
   「你以为的样子」。`Core/Inc/xpulse.h` 是**板级语义权威**，只写「信号 → 引脚」，
   由人工维护、CubeMX 既不生成也不覆盖；模式 / AF / 上下拉 / 时钟仍归 `.ioc` 管，
   两者分工不合并。
2. **只允许在 `USER CODE BEGIN/END` 块内写用户代码**。写在块外的内容会在下次
   `project generate` 时被覆盖。`xpulse.h` 的 `#include` 放在 `main.h` 的
   `USER CODE BEGIN Includes` 里，重新生成后依然保留。
3. **手改 `.ioc` 必须核验**。CubeMX 对 `.ioc` 的错误是**非致命**的：信号名写错只会在日志
   留一行 `Cannot map signal`，生成仍返回成功，外设被静默丢弃。任何 `.ioc` 改动后必须核验：
   - 生成日志里没有 `Cannot map` / 与本次改动相关的 `OptionalMessage_ERROR`；
   - 生成的 `Core/Src/main.c` 里有对应的 `MX_xxx_Init()`。
   
   参数还必须写进该外设 / 引脚的 `IPParameters` / `GPIOParameters` 白名单，否则同样被忽略
   （细节见 [占位符清单](docs/PLACEHOLDERS.md) D3–D5）。
4. **不要提交 `Drivers/`、`Makefile`、`startup_stm32g474xx.s`、`STM32G474xx_FLASH.ld`**：
   HAL / CMSIS / 启动文件 / 链接脚本由 PlatformIO 的 `framework-stm32cubeg4` 包提供，
   已在 `.gitignore` 中排除。新克隆的仓库没有 `Drivers/` 是正常的。
5. **`Core/Src/system_stm32g4xx.c` 已从构建中排除**（`platformio.ini` 的
   `build_src_filter` 里的 `-<Core/Src/system_stm32g4xx.c>`），不要把它加回去，
   否则与 framework 的 CMSIS 重复定义符号。`src_dir = .` 时排除项必须写全相对
   路径（写成 `-<system_stm32g4xx.c>` 不生效），且 `build_src_filter` 必须用
   `+<...>` 白名单，不可用 `+<*>`（会把 `docs/`、`node_modules/` 等目录下的
   `.c` 全部拉进编译）。
6. **不要改 `Core/Inc/stm32g4xx_hal_conf.h` 里的用户逻辑**。该文件由 CubeMX 覆盖生成；
   缺哪个 `HAL_XXX_MODULE_ENABLED` 就在 `.ioc` 里加对应外设再重新生成。
7. **版本锁定不要单独动**。`ststm32@20.0.0` / `framework-stm32cubeg4@1.6.3` /
   `toolchain-gccarmnoneeabi@1.70201.0` / `tool-ldscripts-ststm32@0.3.0` /
   `tool-openocd@3.1200.7` 与 CubeMX 组成验证组合，升级须重新验证「生成 + 构建」。
   CubeMX 实际版本以 exe 属性 / 运行日志为准，`.ioc` 的版本戳可能是历史值。
8. **业务逻辑与 HAL 解耦**。可在主机上跑的核心算法不要直接依赖 HAL 头文件，
   方便用主机 GCC 做 L2 回归；`Core/` 只放 CubeMX 生成物，其 USER CODE 块只留
   `app_init()` / `app_loop()` 之类的转调胶水、不含业务逻辑；业务代码放 `app/`，
   `lib/<name>/` 留给可复用 / 第三方组件。目录职责见 [目录结构](docs/LAYOUT.md)。
9. **行尾统一 LF**（`.gitattributes` 的 `* text=auto eol=lf`，`.editorconfig` 一致，
   `c/h/ioc/ini/json/md` 用 2 空格缩进）。CubeMX 仍可能把生成文件写成 CRLF；
   `text` 属性在 `add` / `status` / `diff` 时归一化，不要为 CubeMX 目录恢复 `eol=crlf`。
10. **自有目录用小写**（`app/`、`boards/`、`docs/`、`scripts/`、`examples/`）；
    CubeMX 管理的 `Core/`、`Drivers/`、`Middlewares/` 保持工具命名与位置。
11. **编辑 `Core/**` 前关闭 `editor.formatOnSave`**，或用 `// clang-format off` 保护，
    只保留与本次任务相关的 diff；避免保存时整篇重排 `main.c` 造成巨大无关 diff。
12. **板级容量与时钟必须与芯片一致**。`boards/*.json` 的 `mcu`、`maximum_size`、
    `maximum_ram_size` 决定链接脚本与烧录校验，`f_cpu` 决定 `F_CPU` 宏；换型号或改主频时
    这几处一起改，不要只改其中一处，并同步 `.ioc` 的 `Mcu.CPN` / `Mcu.Name` /
    `Mcu.Package` / `ProjectManager.DeviceId`。

---

## 3. 验证关卡（做完才能说「完成」）

按改动类型选最小充分集，并在提交信息里写明做到了哪一级：

| 等级 | 做什么 | 适用 |
| --- | --- | --- |
| L0 未验证 | 只读检查 | 仅限纯文档 |
| L1 静态 | 人工核对文件内容一致性、相对链接可达、`node --check` 每个 `.mjs`、`package.json` 合法 | 文档、配置、脚本 |
| L2 主机回归 | 用主机 GCC 编译运行不依赖 HAL 的纯 C 单元 | 改了可主机编译的纯 C 代码 |
| L3 构建 | `pio run`，记录 PlatformIO 报告的 Flash / RAM 字节数 | 任何影响编译的改动 |
| L4 生成核验 | CubeMX `-q` 重新生成后核对日志无 `Cannot map`、`main.c` 有对应 `MX_xxx_Init()` | 任何 `.ioc` 改动（**必须**，见 §2.3） |
| L5 实板 | 烧录 + 观察现象（LED / 串口 / 波形） | 影响运行时行为的改动 |

L2 的两条主机回归命令由示例目录中的 `stdio_port` 样例给出（该目录属可选资料，
整目录删除后本项不适用于本仓）；它们**不需要开发板**，只需要主机 GCC。

**构建通过不等于能用。** 影响运行时行为的改动必须实板确认（L5），并在结论里区分
「编译通过」与「实板验证过」。提交正文必须包含实际执行的验证命令与结果；
纯文档 / 配置只做 L1 时明确写明未运行构建，不复用旧构建数字作为本次证据。

---

## 4. 工作方式

- 动手前先读 [占位符清单](docs/PLACEHOLDERS.md)，确认改的是「该改的占位符」而不是把模板改脏。
- 改动小而可回滚；一次只推进一件事，做完就说结果。
- 不确定就问，不要猜着改跨机器的绝对路径。
- 结论要能独立读懂：说清**改了什么文件、验证到哪一级、还剩什么风险**。
- 不要把没验证的东西说成已完成。
