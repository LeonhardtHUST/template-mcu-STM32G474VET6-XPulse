# 库封装范式（`lib/<name>/` + `library.json`）

> **范式说明**：本文只描述「怎么写」的通用模板，仓库里**没有**任何具体外设库实现，
> 也不提供任何第三方驱动。文中的 `<name>`、`src/`、`inc/` 都是占位名，
> 请按你自己的模块替换。所有示例均为范式，不代表本模板已内置这些代码。
>
> **适用范围**：本文只讲 `lib/<name>/`（可复用 / 第三方组件）。自写业务代码放 `app/`，
> 见 [目录结构](LAYOUT.md)；两者编译模型不同，不要混用。

## 1. 为什么需要它

`.ioc` / CubeMX 生成物负责「外设怎么初始化」，PlatformIO 负责「怎么编译链接」。
把某块功能（回显、显示、Flash 驱动、算法……）做成自维护库，可以：

- 让骨架 `Core/` 始终保持 CubeMX 生成的 10 个文件、零功能代码；
- 重新 `project generate` 时不丢业务代码（CubeMX 不会覆盖 `lib/`）；
- 用 `library.json` 精确控制参与编译的源文件与头文件路径，而不是靠
  `src_dir` 全局通配把无关文件拉进来。

## 2. `library.json` 范式

放在 `lib/<name>/library.json`，四个字段都**显式声明**，不要省略让 PlatformIO 去猜：

```json
{
  "name": "<name>",
  "version": "1.0.0",
  "description": "<一句话说明这个库做什么>",
  "license": "MIT",
  "build": {
    "srcDir": "src",
    "includeDir": "inc",
    "srcFilter": [
      "+<*.c>",
      "+<port/*.c>"
    ],
    "includeDirs": [
      "inc",
      "port"
    ]
  }
}
```

| 字段 | 作用 | 注意 |
| --- | --- | --- |
| `srcDir` | 源文件根目录，相对 `library.json` 所在目录 | 默认 `.`；显式写出来便于一眼看清布局 |
| `includeDir` | 对外暴露的头文件目录 | 只有这里的头文件会被加到依赖方的 include 路径 |
| `srcFilter` | 参与编译的源文件白名单 | 用 `+<...>` 精确列出；默认会收进目录下所有 `.c` / `.cpp`，容易把测试文件、Keil 样板一起编进去 |
| `includeDirs` | 库**内部**额外的头文件搜索路径 | 多个子目录（如 `port/`）要逐条列出，PlatformIO 不会递归搜 |

常见变体：

- 源文件与头文件都在库根目录时：`"srcDir": "."`、`"includeDir": "."`，
  `"srcFilter": ["+<foo.c>", "+<bar.c>"]`。
- 只做头文件库时：省略 `srcFilter`，保留 `"includeDir": "inc"`。
- 目录里混了不该编译的源码时，用 `-<...>` 排除，例如
  `"srcFilter": ["+<*.c>", "-<*_test.c>"]`。

> 别把 `Drivers/`、`Middlewares/`、CubeMX 的 `startup_*.s`、
> `system_stm32g4xx.c` 或 Keil 的 `MDK-ARM/**` 放进 `lib/`：
> 前两类由 PlatformIO 的 `framework-stm32cubeg4` 提供，
> 后几类会造成重复定义或无关告警。

### 2.1 `lib/` 不继承 `-Werror`

`platformio.ini` 的 `build_src_flags` 只作用于 `src_dir` 下的源码，`lib/` 里的
`.c` **不会**拿到 `-std=c11` / `-Wall` / `-Wextra` / `-Werror`。把代码从
`Core/Src` 挪到 `lib/`，等于悄悄撤掉严格告警门禁。

补救：在 `library.json` 的 `build` 里显式声明 `flags`：

```json
"build": {
  "flags": ["-std=c11", "-Wall", "-Wextra", "-Werror"],
  "srcDir": "src",
  "includeDir": "inc"
}
```

干净对照实测（同一份只有 `return-type` 告警、无语法错误的库源码）：带 `flags` →
`error: control reaches end of non-void function [-Werror=return-type]`、构建 FAILED；
不带 → `warning: ... [-Wreturn-type]`、构建 SUCCESS。自写业务代码放 `app/` 则
自动继承 `build_src_flags`，无需这段配置。

### 2.2 未被 `#include` 的库根本不参与编译

默认 LDF 模式（本平台实测 `chain`）只收被源码 `#include` 链拉进来的库。往
`lib/orphan/` 放一份语法完全错误的 `.c`，`pio run` 照样 SUCCESS。这是双刃剑：
不用的库零成本，但写坏的库会静默躺着，直到某天被引用才炸。

- **要全量编译检查**：在 `platformio.ini` 里写 `lib_deps = <name>`（名字取
  `library.json` 的 `name`）。实测该库恒被编译，编译对象数 92 → 93，并生成
  `lib<name>.a`。代价是失去「不用的库零成本」。
- **注意**：`lib_deps` 只管「编不编」，§2.1 的 `flags` 管「门禁严不严」，两者正交，
  要「全量且严格」就得同时配。
- **无效做法**：`lib_ldf_mode = off` 不会把本地 `lib/` 全量编进来。官方定义是
  「只构建 manifest 或 `lib_deps` 指定的库」——指**被指定为依赖**，不是任何带
  `library.json` 的目录；实测加它对 `firmware.elf` / `.bin` 的哈希零影响。

## 3. 「CubeMX 生成物 → PlatformIO 自维护库」流程

```
.ioc（唯一事实源）
   │  CubeMX project generate
   ▼
Core/Src/main.c 里的 MX_xxx_Init()       ← 生成物，不要手改块外
   │
   │  业务代码另起炉灶
   ▼
lib/<name>/src/*.c + inc/*.h + library.json   ← 自维护，CubeMX 不覆盖
   │  PlatformIO 按 library.json 编译
   ▼
pio run（L3）→ L2 主机回归（纯 C 部分）→ L5 实板（运行时行为）
```

逐步说明：

1. **配置先行**：在根目录 `.ioc` 里加外设 / 引脚，改完按 [工程规约](../AGENTS.md) §2.3
   做 L4 生成核验（日志无 `Cannot map`、`main.c` 有 `MX_xxx_Init()`）。
   每个手写参数都要进对应的 `IPParameters` / `GPIOParameters` 白名单。
2. **只搬业务代码**：从例程或自己写的代码里挑出不依赖 Keil / 不重复 HAL 初始化的
   `.c` / `.h`，放进 `lib/<name>/src/` 与 `lib/<name>/inc/`。
   `MX_xxx_Init()`、中断入口、`stm32g4xx_hal_msp.c` 一律留给 CubeMX 生成。
3. **写 `library.json`**：按 §2 的范式显式声明四个字段；`srcFilter` 逐项列出，
   避免把测试文件编进固件。
4. **构建**：`pio run`。若 `srcFilter` 写错，典型症状是「undefined reference」或
   「No such file or directory」，先查 `library.json` 再查代码。
5. **验证**：纯 C、不依赖 HAL 的部分先跑 L2（命令见
   [stdio_port 样例](../examples/stdio_port/README.md)）；影响运行时行为的改动
   最终必须 L5 实板确认。**构建通过不等于能用**。

## 4. 与例程配方的关系

- 具体外设的「抄哪些键、哪些文件值得拷」看 [示例目录说明](../examples/README.md)
  与 `examples/recipes/` 下的三篇配方。
- 本文只给**封装形式**，不含任何具体驱动；`lib/` 目录本身也由使用者自建，
  模板不预先放占位库。
- 已知坑（参数白名单、`S_` 信号前缀、行尾混用、`QUADSPI1` 这类 IP 名）统一见
  [占位符清单](PLACEHOLDERS.md) 与 [CubeMX 工具实测](TOOLING-cubemx.md)。
