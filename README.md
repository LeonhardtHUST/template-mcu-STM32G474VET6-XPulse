# template-mcu-minimal-STM32G474VET6-XPulse

Minimal STM32G474VET6 starter template for CubeMX + PlatformIO + VS Code.

## Target

- MCU: STM32G474VET6 (LQFP100, 512 KB Flash, 128 KB RAM)
- Board: XPulse / Penguins-G474
- Debug/flash: ST-Link over SWD

## Required tools

- STM32CubeMX
- PlatformIO Core (`pio`)
- VS Code + PlatformIO + Cortex-Debug

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

## Modify peripherals

Open `template-mcu-minimal-STM32G474VET6-XPulse.ioc` in STM32CubeMX → change clocks/pins/peripherals → Generate Code → run `pio run`.

## Default pins and clock

- PC13: red LED (active low)
- PA13: SWDIO
- PA14: SWCLK
- Default clock: HSI 16 MHz (no PLL)
- Board also provides 24 MHz HSE, which you can enable later in CubeMX

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
> `vscode-clangd`，所以正常情况只要跑一次脚本（或下面两条命令）即可。

### 一键脚本（推荐）

Windows PowerShell —— 脚本未做代码签名，需要绕过执行策略，否则会报
`无法加载文件 ... 因为在此系统上禁止运行脚本`：

```powershell
# 方式一：只对这一次进程生效，不改系统设置
powershell -ExecutionPolicy Bypass -File .\scripts\init-clangd.ps1

# 方式二：在当前窗口解除限制后再运行
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\scripts\init-clangd.ps1
```

如果脚本是从别的机器/压缩包拷来的，可能还带 `Zone.Identifier` 标记，先解除锁定：

```powershell
Unblock-File .\scripts\init-clangd.ps1
```

Linux / macOS / Git Bash：

```bash
./scripts/init-clangd.sh
```

### 手动执行 — Windows PowerShell

```powershell
pio run -t compiledb
@'
CompileFlags:
CompilationDatabase: .
'@ | Set-Content -Encoding utf8 .clangd
New-Item -ItemType Directory -Force .vscode | Out-Null
@'
{
"C_Cpp.intelliSenseEngine": "disabled",
"clangd.arguments": [
"--compile-commands-dir=${workspaceFolder}",
"--query-driver=**/arm-none-eabi-gcc.exe,**/arm-none-eabi-g++.exe",
"--background-index",
"--clang-tidy"
],
"clangd.restartAfterCrash": true
}
'@ | Set-Content -Encoding utf8 .vscode/settings.json
```

### 手动执行 — Linux Bash

```bash
pio run -t compiledb
cat > .clangd <<'EOF'
CompileFlags:
CompilationDatabase: .
EOF
mkdir -p .vscode
cat > .vscode/settings.json <<'EOF'
{
"C_Cpp.intelliSenseEngine": "disabled",
"clangd.arguments": [
"--compile-commands-dir=${workspaceFolder}",
"--query-driver=**/arm-none-eabi-gcc,**/arm-none-eabi-g++",
"--background-index",
"--clang-tidy"
],
"clangd.restartAfterCrash": true
}
EOF
```

完成后在 VS Code 执行 `clangd: Restart language server`（命令面板 `Ctrl+Shift+P`）。

> 手动执行时注意：上面的写法会**整体覆盖** `.vscode/settings.json`，把仓库原有的
> `files.exclude` / `search.exclude` / `files.associations` 冲掉。想保留就只手工追加
> `clangd.*` 这几项，或者直接用 `scripts/` 里的脚本（它在文件已存在时不会覆盖）。

### 什么时候需要重新生成

每当 `platformio.ini`、board、framework、`build_flags`、依赖或源文件集合发生变化时，
`compile_commands.json` 就会过期（表现为 clangd 报头文件找不到、宏对不上），重新执行：

```text
pio run -t compiledb
```

`compile_commands.json` 和 `.clangd` 都是生成物，已在 `.gitignore` 中忽略，不必提交。

