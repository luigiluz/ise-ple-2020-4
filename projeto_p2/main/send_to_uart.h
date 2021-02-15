#ifndef SEND_TO_UART_H_
#define SEND_TO_UART_H_

typedef struct {
    char *msg;
    uint32_t msg_len;
} uart_msg;

/* This function is used to init UART for both send_to_uart_task and get_from_uart_task */
extern void uart_init(void);

extern void init_send_to_uart_queue(void);

extern BaseType_t append_to_send_to_uart_queue(uart_msg *msg);

extern void send_to_uart_task(void *pvParameters);

extern void init_send_to_uart_semph(void);

#endif