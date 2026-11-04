# L2 主机回归样例：newlib I/O 桩

> **这是样例，不是固件的一部分**：`stdio_port.c` / `stdio_port_test.c` 不进 `Core/`、
> 不进 `main.c`、不参与 `pio run`。它们只用来演示「业务逻辑与 HAL 解耦」后，
> 用主机 GCC 做 [L2 回归](../../AGENTS.md) 的写法。

## 它测什么

`stdio_port.c` 覆盖 newlib 的弱符号 `_read` / `_write`，不依赖任何 HAL 头文件，
因此可以直接在主机上编译：

- 无后端（`__io_putchar` / `__io_getchar` 未实现）时返回 `-1` 并置 `errno = ENOSYS`；
- 非法 fd、非法长度返回 `-1` 并置 `EBADF` / `EINVAL`；
- 有后端时逐字符转发，后端报错时置 `EIO` 并返回已处理的字节数。

`stdio_port_test.c` 用 `-DTEST_BACKEND` 编译出第二组用例，覆盖后端成功与失败路径。
两组命令都通过时程序返回 0，`assert` 失败会直接非零退出。

## 两条 gcc 命令

在**仓库根目录**执行（需要主机 GCC，不需要开发板）：

```bash
mkdir -p .pio/host-test
gcc -std=c11 -Wall -Wextra -Werror examples/stdio_port/stdio_port.c examples/stdio_port/stdio_port_test.c -o .pio/host-test/stdio-test && ./.pio/host-test/stdio-test
gcc -std=c11 -Wall -Wextra -Werror -DTEST_BACKEND examples/stdio_port/stdio_port.c examples/stdio_port/stdio_port_test.c -o .pio/host-test/stdio-backend-test && ./.pio/host-test/stdio-backend-test
```

Windows（MinGW / Git Bash）下给输出加 `.exe` 后缀：

```bash
mkdir -p .pio/host-test
gcc -std=c11 -Wall -Wextra -Werror examples/stdio_port/stdio_port.c examples/stdio_port/stdio_port_test.c -o .pio/host-test/stdio-test.exe && ./.pio/host-test/stdio-test.exe
gcc -std=c11 -Wall -Wextra -Werror -DTEST_BACKEND examples/stdio_port/stdio_port.c examples/stdio_port/stdio_port_test.c -o .pio/host-test/stdio-backend-test.exe && ./.pio/host-test/stdio-backend-test.exe
```

`.pio/` 已被 `.gitignore` 忽略，产物不需要清理也不需要入库。
上面两条主机回归命令里的 `-Werror` 是命令自带参数，与 `platformio.ini` 的
`build_src_flags` 相互独立；固件构建现已同样开启 `-Werror`（见
[README 模板维护与验证](../../README.md)），零告警是两者的共同前提。

## 想把这套 I/O 桩用到固件里

1. 把 `stdio_port.c` 放进 `lib/<name>/src/`，配一份
   [library.json 范式](../../docs/LIB-PATTERN.md)。
2. 在独立的用户源码里实现 `int __io_putchar(int ch)` 与 `int __io_getchar(void)`：
   成功返回字符（0..255），失败返回负数；例如转发到 `HAL_UART_Transmit()`。
3. 按 [USART 配方](../recipes/USART.md) 在 `.ioc` 里启用 USART1，再 `pio run`；
   影响运行时行为，最终仍需 **L5 实板**确认。

> 本目录属可选资料，`vpr examples:remove` 会随 `examples/` 一起删除。
