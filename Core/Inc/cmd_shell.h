#ifndef CMD_SHELL_H
#define CMD_SHELL_H

/* 主循环周期性调用（非阻塞，USART1 轮询） */
void cmd_poll(void);
/* 外部字节源注入（USB CDC RX 等共用同一命令流） */
void cmd_input_char(char c);
/* 上电打印提示符 */
void cmd_banner(void);

#endif /* CMD_SHELL_H */
