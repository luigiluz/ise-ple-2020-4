#ifndef GEY_FROM_UART_H_
#define GEY_FROM_UART_H_

#define DISPLAY_MSG_MAX_LEN     16

typedef struct {
    int cursor;
    char msg[DISPLAY_MSG_MAX_LEN];
} display_msg;

extern void get_from_uart_init_queue(void);

extern BaseType_t get_from_uart_read_queue(display_msg *received_msg);

extern void get_from_uart_task(void *pvParameters);

#endif