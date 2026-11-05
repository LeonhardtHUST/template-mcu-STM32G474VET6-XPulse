# PlatformIO 预处理脚本：把 Cortex-M4F（FPv4-SP）硬件浮点参数补进汇编与链接阶段。
#
# 为什么需要它：
#   `platformio.ini` 的 `build_flags` 里已经写了 `-mfpu=fpv4-sp-d16` 与
#   `-mfloat-abi=hard`，但 PlatformIO 用 SCons 的 ParseFlags 解析 build_flags，
#   只有以 `-Wl,` 开头的项会进 LINKFLAGS，其余（含这两个）只进 CCFLAGS。
#   于是所有 .o 都是硬浮点，链接命令行却仍是驱动默认的软浮点：ld 会报
#   "<xxx>.o uses VFP register arguments, firmware.elf does not"，
#   并去链软浮点版 libgcc / newlib。这里显式补齐 ASFLAGS 与 LINKFLAGS，
#   让编译、汇编、链接三个 scope 使用同一套 ABI。
#
# 为什么必须是 pre: 前缀（`extra_scripts = pre:scripts/pio-hard-fpu.py`）：
#   脚本要在 ststm32 平台的 framework 脚本之前执行；HAL / CMSIS 静态库在
#   framework 脚本里克隆 env，晚于此时追加就赶不上，HAL 会退回软浮点。
#
# 参照：PlatformIO 官方论坛 "object uses VFP register arguments" 系列答复
#   （maxgerhardt），做法即 pre: extra_script + env.Append(CCFLAGS/LINKFLAGS)。
Import("env")

_FPU_FLAGS = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]

env.AppendUnique(ASFLAGS=_FPU_FLAGS, LINKFLAGS=_FPU_FLAGS)
