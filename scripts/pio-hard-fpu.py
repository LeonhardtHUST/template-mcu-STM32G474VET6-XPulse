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
# 什么必需、什么不必需：
#   必需的是「把同一对 flag 补进 ASFLAGS / LINKFLAGS」这件事本身：整行删掉
#   `extra_scripts` 后链接退出码 1，92 个 .o 全部报
#   "uses VFP register arguments, firmware.elf does not"（已实测）。
#   不必需的是 `pre:` 前缀：编译侧的硬浮点来自 build_flags→CCFLAGS，与本脚本的
#   执行顺序无关，而 ASFLAGS / LINKFLAGS 在汇编 / 链接时才被读取。实测把
#   `pre:` 去掉（`extra_scripts = scripts/pio-hard-fpu.py`）产物完全一致
#   （SUCCESS、RAM 1584 B / FLASH 3740 B、HAL 仍 Tag_FP_arch: VFPv4-D16）；
#   保留 pre: 只是稳妥的书写习惯，不是链接成功的前提。
#
# 参照：PlatformIO 官方论坛 "object uses VFP register arguments" 系列答复
#   （maxgerhardt），做法即 pre: extra_script + env.Append(CCFLAGS/LINKFLAGS)。
Import("env")

_FPU_FLAGS = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]

env.AppendUnique(ASFLAGS=_FPU_FLAGS, LINKFLAGS=_FPU_FLAGS)
