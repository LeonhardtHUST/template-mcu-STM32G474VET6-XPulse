/* X Pulse 极简串口命令行：USART1 主循环轮询，无中断/DMA 依赖。
 * 主循环以 50ms 步进调用 cmd_poll()，最坏响应延迟 <50ms。
 * 命令不分大小写，参数以空格分隔。 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "stm32g4xx_hal.h"
#include "usbd_cdc_if.h"
#include "cmd_shell.h"

extern UART_HandleTypeDef huart1;
extern volatile uint32_t g_uptime_s;
extern volatile uint32_t g_heartbeat_tx;
extern volatile uint32_t g_key_presses;
extern volatile uint32_t g_jedec_id;
extern volatile uint32_t g_qspi_speed;
extern volatile uint32_t g_led_mode;
extern volatile uint32_t g_sfud_ok;
extern volatile uint32_t g_flash_size;

#define CMD_BUF_LEN 64
#define USB_RING 128

static void cmd_process_char(char c);

static char cmd_buf[CMD_BUF_LEN];
static uint32_t cmd_len = 0;
static uint8_t out_usb = 0; /* 最近输入来源：1=USB CDC 0=USART1 */
/* CDC RX 环形缓冲：USB 中断只入队，命令处理在主循环（避免中断里忙等 CDC 包） */
static volatile char usb_ring[USB_RING];
static volatile uint8_t usb_head = 0, usb_tail = 0;

static void cmd_putc(char c)
{
  uint8_t b = (uint8_t)c;
  if (out_usb)
    (void)CDC_Transmit_FS(&b, 1);
  else
    (void)HAL_UART_Transmit(&huart1, &b, 1, 100);
}

static void cmd_puts(const char *s)
{
  size_t len = strlen(s);
  if (len == 0)
    return;
  if (out_usb) {
    /* 主循环上下文：BUSY 时短重试（心跳包可能占用中） */
    for (int t = 0; t < 20; t++) {
      if (CDC_Transmit_FS((uint8_t *)s, (uint16_t)len) == USBD_OK)
        return;
      HAL_Delay(1);
    }
  } else {
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)len, 100);
  }
}

static void cmd_puts_u(const char *s, uint32_t v)
{
  char line[64];
  int n = snprintf(line, sizeof(line), "%s%lu\r\n", s, (unsigned long)v);
  if (n > 0)
    cmd_puts(line);
}

static void cmd_exec(char *line)
{
  char *argv[4] = {0};
  int argc = 0;
  char *p = line;
  while (*p && argc < 4) {
    while (*p == ' ')
      *p++ = '\0';
    if (*p) {
      argv[argc++] = p;
      while (*p && *p != ' ')
        p++;
    }
  }
  if (argc == 0)
    return;

  if (!strcasecmp(argv[0], "help")) {
    cmd_puts(
        "commands:\r\n"
        "  help          - this text\r\n"
        "  uptime        - seconds since boot\r\n"
        "  key           - key press count\r\n"
        "  mode [0-3]    - led breath mode (0=blue 1=green 2=red 3=all)\r\n"
        "  flash         - flash info (JEDEC, sfud status)\r\n"
        "  flashtest     - erase+write+read selftest 256B @ 0\r\n"
        "  reset         - reboot MCU\r\n");
  } else if (!strcasecmp(argv[0], "uptime")) {
    cmd_puts_u("uptime: ", g_uptime_s);
  } else if (!strcasecmp(argv[0], "key")) {
    cmd_puts_u("keys: ", g_key_presses);
  } else if (!strcasecmp(argv[0], "mode")) {
    if (argc >= 2) {
      uint32_t m = (uint32_t)strtoul(argv[1], 0, 0);
      if (m <= 3) {
        g_led_mode = m;
        cmd_puts_u("mode -> ", g_led_mode);
      } else {
        cmd_puts("mode: 0..3\r\n");
      }
    } else {
      cmd_puts_u("mode: ", g_led_mode);
    }
  } else if (!strcasecmp(argv[0], "flash")) {
    char line2[80];
    int n = snprintf(line2, sizeof(line2),
                     "flash: JEDEC 0x%06lX, prescaler=%lu, sfud=%s, %lu bytes\r\n",
                     (unsigned long)g_jedec_id, (unsigned long)g_qspi_speed,
                     g_sfud_ok == 1 ? "ok" : "FAIL",
                     (unsigned long)g_flash_size);
    if (n > 0)
      cmd_puts(line2);
  } else if (!strcasecmp(argv[0], "flashtest")) {
    extern int flash_selftest(void);
    int r = flash_selftest();
    cmd_puts(r == 0 ? "flashtest: PASS (erase+write+read 256B @ 0)\r\n"
                    : "flashtest: FAIL\r\n");
    cmd_puts_u("  result = ", (uint32_t)r);
  } else if (!strcasecmp(argv[0], "reset")) {
    cmd_puts("resetting...\r\n");
    HAL_Delay(10);
    NVIC_SystemReset();
  } else {
    cmd_puts("unknown: ");
    cmd_puts(argv[0]);
    cmd_puts(" (try 'help')\r\n");
  }
}

void cmd_poll(void)
{
  uint8_t c;
  char uc;
  /* 先消费 USB 环形缓冲（响应走 CDC） */
  while (usb_tail != usb_head) {
    uc = usb_ring[usb_tail];
    usb_tail = (uint8_t)((usb_tail + 1) % USB_RING);
    out_usb = 1;
    cmd_process_char(uc);
  }
  /* 再轮询 USART1（响应走串口） */
  while (HAL_UART_Receive(&huart1, &c, 1, 0) == HAL_OK) {
    out_usb = 0;
    cmd_process_char((char)c);
  }
}

/* USB 中断上下文调用：仅入队，绝不在这里做输出 */
void cmd_input_char(char c)
{
  uint8_t next = (uint8_t)((usb_head + 1) % USB_RING);
  if (next != usb_tail) {
    usb_ring[usb_head] = c;
    usb_head = next;
  }
}

/* 主循环上下文：处理一个字符（echo/回车执行命令） */
static void cmd_process_char(char c)
{
  if (c == '\r' || c == '\n') {
    if (cmd_len > 0) {
      cmd_puts("\r\n");
      cmd_buf[cmd_len] = '\0';
      cmd_exec(cmd_buf);
      cmd_len = 0;
    }
    cmd_puts("x-pulse> ");
  } else if (c == 0x08 || c == 0x7F) { /* backspace */
    if (cmd_len > 0) {
      cmd_len--;
      cmd_puts("\b \b");
    }
  } else if (c >= 0x20 && c < 0x7F && cmd_len < CMD_BUF_LEN - 1) {
    cmd_buf[cmd_len++] = c;
    cmd_putc(c); /* echo */
  }
  /* 其余字符丢弃 */
}

void cmd_banner(void)
{
  cmd_puts("\r\n== x-pulse shell (help) ==\r\nx-pulse> ");
}
