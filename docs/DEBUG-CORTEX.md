# Cortex-Debug 调试配置示例（ST-Link / CMSIS-DAP）

本仓库**忽略 `.vscode/launch.json`**（见 `.gitignore`：它由 PlatformIO / STM32 VS Code
扩展生成，CubeMX `project generate` 也会覆盖它，且含本机绝对路径）。需要 Cortex-Debug
调试时，请按下面的示例**自行创建** `.vscode/launch.json`，不要把它提交进版本库。

> 示例里的路径全部用 `${workspaceFolder}`、`${env:USERPROFILE}`（Windows）与
> `${env:HOME}`（Linux/macOS）变量，不写死绝对路径。前提是 PlatformIO 的包目录在默认
> 位置（`%USERPROFILE%\.platformio` / `~/.platformio`）；若设置了 `PLATFORMIO_CORE_DIR`，
> 把下面几处 `.platformio` 前缀一起改掉。

## 前置条件

- 已执行 `pio run`，产物为 `${workspaceFolder}/.pio/build/genericSTM32G474VE/firmware.elf`；
- 已安装 Cortex-Debug 扩展，且 `tool-openocd@3.1200.7` 已随 `pio run` 安装；
- SWD 接线：SWDIO = PA13、SWCLK = PA14、共地、目标板独立供电。

## launch / attach 双配置

把以下内容存为 `.vscode/launch.json`（`jsonc`，允许注释）：

```jsonc
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug (OpenOCD / ST-Link)",
      "type": "cortex-debug",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/.pio/build/genericSTM32G474VE/firmware.elf",
      "device": "STM32G474VE",
      "servertype": "openocd",
      "serverpath": "${env:HOME}/.platformio/packages/tool-openocd/bin/openocd",
      "configFiles": ["${workspaceFolder}/scripts/openocd-stlink.cfg"],
      "svdFile": "${env:HOME}/.platformio/platforms/ststm32/misc/svd/STM32G474xx.svd",
      "runToEntryPoint": "main",
      "preLaunchTask": {
        "type": "PlatformIO",
        "task": "Pre-Debug"
      },
      "armToolchainPath": "${env:HOME}/.platformio/packages/toolchain-gccarmnoneeabi/bin",
      "searchDir": ["${env:HOME}/.platformio/packages/tool-openocd/openocd/scripts"],
      "windows": {
        "serverpath": "${env:USERPROFILE}/.platformio/packages/tool-openocd/bin/openocd.exe",
        "armToolchainPath": "${env:USERPROFILE}/.platformio/packages/toolchain-gccarmnoneeabi/bin",
        "searchDir": ["${env:USERPROFILE}/.platformio/packages/tool-openocd/openocd/scripts"],
        "svdFile": "${env:USERPROFILE}/.platformio/platforms/ststm32/misc/svd/STM32G474xx.svd"
      }
    },
    {
      "name": "Attach (OpenOCD / ST-Link)",
      "type": "cortex-debug",
      "request": "attach",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/.pio/build/genericSTM32G474VE/firmware.elf",
      "device": "STM32G474VE",
      "servertype": "openocd",
      "serverpath": "${env:HOME}/.platformio/packages/tool-openocd/bin/openocd",
      "configFiles": ["${workspaceFolder}/scripts/openocd-stlink.cfg"],
      "svdFile": "${env:HOME}/.platformio/platforms/ststm32/misc/svd/STM32G474xx.svd",
      "armToolchainPath": "${env:HOME}/.platformio/packages/toolchain-gccarmnoneeabi/bin",
      "searchDir": ["${env:HOME}/.platformio/packages/tool-openocd/openocd/scripts"],
      "windows": {
        "serverpath": "${env:USERPROFILE}/.platformio/packages/tool-openocd/bin/openocd.exe",
        "armToolchainPath": "${env:USERPROFILE}/.platformio/packages/toolchain-gccarmnoneeabi/bin",
        "searchDir": ["${env:USERPROFILE}/.platformio/packages/tool-openocd/openocd/scripts"],
        "svdFile": "${env:USERPROFILE}/.platformio/platforms/ststm32/misc/svd/STM32G474xx.svd"
      }
    }
  ]
}
```

- `launch`：先跑 PlatformIO 的 `Pre-Debug` 任务（编译 / 烧录），再复位并停在 `main`
  （`runToEntryPoint`）；改完固件用这一项。
- `attach`：不复位、不烧录，直接挂到正在运行的固件上，适合观察跑飞、低功耗或长时间
  运行后的现场；这一项**没有** `preLaunchTask` 与 `runToEntryPoint`。
- `searchDir` 让 OpenOCD 能解析 `scripts/openocd-*.cfg` 里的 `[find interface/...]`
  与 `[find target/stm32g4x.cfg]`；`svdFile` 提供外设寄存器视图。
- PlatformIO IDE 可能自动追加 `PIO Debug` 系列配置，那些由扩展维护，不要手改。

## 切换 ST-Link ↔ CMSIS-DAP

仓库自带两份通用 OpenOCD 配置，内容只有第一行不同、且都不含绝对路径：

- `scripts/openocd-stlink.cfg`：`source [find interface/stlink.cfg]`
- `scripts/openocd-cmsis-dap.cfg`：`source [find interface/cmsis-dap.cfg]`

两者都是 `transport select swd` + `source [find target/stm32g4x.cfg]`。换探针时
**下面三处必须一起改**，否则会出现「烧录用 A 探针、调试连 B 探针」的错配：

1. `platformio.ini` 的 `upload_protocol`：`stlink` → `cmsis-dap`；
2. `platformio.ini` 的 `debug_tool`：`stlink` → `cmsis-dap`；
3. 上面两个 configuration 的 `configFiles`：
   `${workspaceFolder}/scripts/openocd-stlink.cfg` →
   `${workspaceFolder}/scripts/openocd-cmsis-dap.cfg`。

改完重新 `pio run`（或 `pio run -t upload`）并在 VS Code 里重启调试会话。
探针相关占位符见 [占位符清单 B1 / B6](PLACEHOLDERS.md)。

## 相关文档

- [工程规约 §2.2 / §3](../AGENTS.md)：USER CODE 边界与 L0–L5 验证关卡
- [CubeMX 工具实测 §3.11](TOOLING-cubemx.md)：生成会覆盖 `launch.json`
- [README 故障排查](../README.md)：探针连不上 / SWD `No ACK` 的排查顺序
