/**
  ******************************************************************************
  * @file    app_example.c
  * @brief   应用层起步示例：把模板开箱点灯逻辑从 Core/Src/main.c 搬到这里。
  ******************************************************************************
  */
#include "app_example.h"
#include "xpulse.h"

int app_example_to_hust(int a)
{
  return a + APP_EXAMPLE_HUST;
}

void app_init(void)
{
  /* 给示例函数留一个调用点；-Os 下它可能被内联，最终符号未必出现在 .map。
   * 对照：Core/Src/example.c 的 example_to_hust() 没有调用点，会被 --gc-sections 丢弃。 */
  volatile int probe = app_example_to_hust(1);
  (void)probe;
}

void app_loop(void)
{
  XPULSE_LED_R_ON();
  HAL_Delay(500);
  XPULSE_LED_R_OFF();
  HAL_Delay(500);
}
