/* X Pulse 调试串口后端：USART1（PA9=TX, PA10=RX, 115200 8N1）。
 * 板上无 USB-UART 桥，PA9/PA10 经 IO 排针引出；外接 USB-TTL 时
 * TX->RX、RX->TX、共地。printf/scanf 经 stdio_port.c 落到这里。 */
#include <errno.h>
#include "stm32g4xx_hal.h"

/* main.c 在 USER CODE 区生成（CubeMX CoupleFile=false） */
extern UART_HandleTypeDef huart1;

int __io_putchar(int ch)
{
  const UART_HandleTypeDef *u = &huart1;
  /* MX_USART1_UART_Init 之前调用（如静态初始化的输出）时直接丢弃，
   * 避免在实例未配置时阻塞或误报。 */
  if (u->gState == HAL_UART_STATE_RESET) {
    return (unsigned char)ch;
  }
  if (HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100) != HAL_OK) {
    errno = EIO;
    return -1;
  }
  return (unsigned char)ch;
}

int __io_getchar(void)
{
  uint8_t c = 0;
  if (huart1.RxState == HAL_UART_STATE_RESET ||
      HAL_UART_Receive(&huart1, &c, 1, HAL_MAX_DELAY) != HAL_OK) {
    errno = EIO;
    return -1;
  }
  return c;
}
