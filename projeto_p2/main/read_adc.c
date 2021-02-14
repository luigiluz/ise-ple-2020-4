#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "driver/hw_timer.h"

#include "esp_log.h"

#include "display.h"
#include "send_to_uart.h"

static const char *TAG = "READ_ADC";

#define ADC_BUFFER_SIZE             4

#define ADC_SAMPLING_TIME           2000 /* In useg */

#define ADC_DISPLAY_MSG_CURSOR_ROW  1 
#define ADC_DISPLAY_MSG_CURSOR_COL  0
#define ADC_DISPLAY_MSG_LEN         3

// TaskHandle_t AdcTimerHandle;

static uint8_t timer_flag = 0;

void read_adc_init()
{
    // 1. init adc
    adc_config_t adc_config;

    // Depend on menuconfig->Component config->PHY->vdd33_const value
    // When measuring system voltage(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
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

// void adc_timer_callback(TimerHandle_t xTimer)
// {
//     timer_flag = 1;
// }

void hw_timer_callback(void *arg)
{
    timer_flag = 1;
}

void read_adc_task()
{
    uint16_t adc_data[ADC_BUFFER_SIZE];
    uint32_t mean_adc_value;
    static int i;
    display_params params;
    uart_msg uart;
    char adc_json_msg[19];
    char mean_adc_value_str[ADC_DISPLAY_MSG_LEN];

	params.cursor_row = ADC_DISPLAY_MSG_CURSOR_ROW;
	params.cursor_col = ADC_DISPLAY_MSG_CURSOR_COL;
	params.msg_len = ADC_DISPLAY_MSG_LEN;

    ESP_LOGI(TAG, "Initialize hw_timer for callback");
    hw_timer_init(hw_timer_callback, NULL);
    ESP_LOGI(TAG, "Set hw_timer timing time %d useg with reload", ADC_SAMPLING_TIME);
    hw_timer_alarm_us(ADC_SAMPLING_TIME, pdTRUE);

    // AdcTimerHandle = xTimerCreate("AdcTimer", pdMS_TO_TICKS(ADC_SAMPLING_TIME), pdTRUE, 0, adc_timer_callback);
    // if (AdcTimerHandle != NULL) {
    //     ESP_LOGI(TAG, "AdcTimer criado com sucesso");
    //     xTimerStart(AdcTimerHandle, 0);
    // }

    while (1) {
        // ESP_LOGI(TAG, "Executando a read_adc_task");
        if (timer_flag) {
            if (ESP_OK == adc_read_fast(&adc_data[i], 1)) {
                // ESP_LOGI(TAG, "adc read: %d", adc_data[i]);
            } else {
                ESP_LOGI(TAG, "leitura do adc falhou");
            }
            i++;
            timer_flag = 0;
        }

        if (i == ADC_BUFFER_SIZE - 1) {
            mean_adc_value = get_mean_adc_value(adc_data);
            sprintf(mean_adc_value_str, "%d", mean_adc_value);
            strncpy(params.msg, mean_adc_value_str, ADC_DISPLAY_MSG_LEN);

            display_semphr_take();

            BaseType_t DisplaySendReturn = send_to_display_message_queue(&params);
            if (DisplaySendReturn == pdTRUE) {
                // ESP_LOGI(TAG, "Mensagem enviada pela fila com sucesso");
            } else {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila");
            }

            display_semphr_give();

            sprintf(adc_json_msg, "{ \"ADC\": \"0x%02X\" }\n", (mean_adc_value & 0xFF));
			
			uart.msg = adc_json_msg;
			uart.msg_len = sizeof(adc_json_msg) / sizeof(adc_json_msg[0]);

            send_to_uart_semphr_take();

			BaseType_t SendToUartReturn = append_to_send_to_uart_queue(&uart);
            if (SendToUartReturn == pdTRUE) {
                ESP_LOGI(TAG, "Mensagem enviada pela fila da uart com sucesso");
            } else {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila da uart");
            }

            send_to_uart_semphr_give();

            i = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}