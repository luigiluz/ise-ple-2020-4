#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "driver/hw_timer.h"

#include "esp_log.h"

#include "display.h"
#include "mqtt_task.h"

static const char *TAG = "READ_ADC";

#define ADC_BUFFER_SIZE             4

#define ADC_SAMPLING_TIME           200000 /* In useg */

#define ADC_DISPLAY_MSG_CURSOR_ROW  1 
#define ADC_DISPLAY_MSG_CURSOR_COL  0
#define ADC_DISPLAY_MSG_LEN         3

static uint8_t timer_flag = 0;

void read_adc_init()
{
    adc_config_t adc_config;

    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8;
    ESP_ERROR_CHECK(adc_init(&adc_config));
}

uint32_t get_mean_adc_value(uint16_t *adc_data)
{
    uint32_t sum = 0;
    uint32_t mean_adc_value;
    int i;

    for (i = 0; i < ADC_BUFFER_SIZE; i++) {
        sum += adc_data[i];
    }

    mean_adc_value = sum / ADC_BUFFER_SIZE;

    return mean_adc_value;
}

uint32_t convert_adc_value(uint32_t adc_value)
{
    return (adc_value * 255) / 1023;
}

void hw_timer_callback(void *arg)
{
    timer_flag = 1;
}

mqtt_msg_t build_adc_mqtt_msg(uint16_t adc_value, uint32_t timestamp)
{
    mqtt_msg_t mqtt_msg;
    static char adc_json_msg[41];

    sprintf(adc_json_msg, "{ \"ADC\": \"%d\", \"ts\": \"%d\" }\n", adc_value, timestamp);
    
    mqtt_msg.tsk_id = READ_ADC_TASK_ID;
    mqtt_msg.msg = adc_json_msg;
    mqtt_msg.msg_len = sizeof(adc_json_msg) / sizeof(adc_json_msg[0]);

    return mqtt_msg;
}

display_msg_t build_adc_display_msg(uint32_t adc_value)
{
    display_msg_t display_msg;
    static char adc_value_str[ADC_DISPLAY_MSG_LEN];

    sprintf(adc_value_str, "%d", adc_value);

    display_msg.tsk_id = READ_ADC_TASK_ID;
    display_msg.cursor_row = ADC_DISPLAY_MSG_CURSOR_ROW;
    display_msg.cursor_col = ADC_DISPLAY_MSG_CURSOR_COL;
    display_msg.msg_len = ADC_DISPLAY_MSG_LEN;
    strncpy(display_msg.msg, adc_value_str, ADC_DISPLAY_MSG_LEN);

    return display_msg;
}

void read_adc_task()
{
    uint16_t adc_data[ADC_BUFFER_SIZE];
    uint32_t mean_adc_value;
    static int i = 0;
    display_msg_t display_msg;
    mqtt_msg_t mqtt_msg;
    static uint32_t timestamp = 0;

    ESP_LOGI(TAG, "Initialize hw_timer for callback");
    hw_timer_init(hw_timer_callback, NULL);
    ESP_LOGI(TAG, "Set hw_timer timing time %d useg with reload", ADC_SAMPLING_TIME);
    hw_timer_alarm_us(ADC_SAMPLING_TIME, pdTRUE);

    while (1) {
        if (timer_flag) {
            if (adc_read(&adc_data[i]) != ESP_OK) {
                ESP_LOGI(TAG, "Leitura do adc falhou");
            }
            i++;
            timer_flag = 0;
        }

        if (i == ADC_BUFFER_SIZE) {
            timestamp = timestamp + 1;
            mean_adc_value = get_mean_adc_value(adc_data);
            mean_adc_value = convert_adc_value(mean_adc_value);

            display_msg = build_adc_display_msg(mean_adc_value);
            BaseType_t DisplaySendReturn = display_append_to_message_queue(&display_msg);
            if (DisplaySendReturn != pdTRUE) {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila do display");
            }

            mqtt_msg = build_adc_mqtt_msg(mean_adc_value, timestamp);

            BaseType_t MQTTReturn = mqtt_task_append_to_message_queue(&mqtt_msg);
            if (MQTTReturn != pdTRUE) {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila do mqtt");
            }

            i = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}