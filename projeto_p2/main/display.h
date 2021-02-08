#ifndef DISPLAY_H_
#define DISPLAY_H_

#define DISPLAY_MSG_MAX_LEN     16

typedef struct {
    uint16_t cursor_row;
    uint16_t cursor_col;
    char msg[DISPLAY_MSG_MAX_LEN];
    uint16_t msg_len;
} display_params;

extern void init_display_message_queue(void);

extern void display_task(void *pvParameters);

extern BaseType_t send_to_display_message_queue(display_params *params_to_send);

#endif