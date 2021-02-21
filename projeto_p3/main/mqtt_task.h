#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include "display.h"

typedef struct {
    enum task_id tsk_id;
    char *msg;
    uint32_t msg_len;
} mqtt_msg_t;

extern void mqtt_task(void *pvParameters);

extern void mqtt_task_semph_init(void);

extern void mqtt_task_message_queue_init(void);

extern BaseType_t mqtt_task_append_to_message_queue(mqtt_msg_t *msg);

#endif