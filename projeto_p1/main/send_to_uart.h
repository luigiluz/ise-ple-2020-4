#ifndef SEND_TO_UART_H_
#define SEND_TO_UART_H_

/* This function is used to init UART for both send_to_uart_task and get_from_uart_task */
extern void uart_init(void);

extern void send_to_uart_task(void *pvParameters);

#endif