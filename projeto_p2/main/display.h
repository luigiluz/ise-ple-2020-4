#ifndef DISPLAY_H_
#define DISPLAY_H_

#define DISPLAY_MSG_MAX_LEN     16

enum task_id {
    DISPLAY_TASK_ID = 0,
    GET_FROM_UART_TASK_ID,
    KEYPAD_TASK_ID,
    READ_ADC_TASK_ID,
    SEND_TO_UART_TASK_ID
};

typedef struct {
    enum task_id tsk_id;
    uint16_t cursor_row;
    uint16_t cursor_col;
    char msg[DISPLAY_MSG_MAX_LEN];
    uint16_t msg_len;
} display_params;

extern void init_display_message_queue(void);

extern void display_task(void *pvParameters);

extern BaseType_t send_to_display_message_queue(display_params *params_to_send);

extern void init_display_semph(void);

extern void display_semphr_take(void);

extern void display_semphr_give(void);

#endif