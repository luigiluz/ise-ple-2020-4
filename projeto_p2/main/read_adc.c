#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "display.h"

static const char *TAG = "READ_ADC";

#define ADC_BUFFER_SIZE             4

#define ADC_DISPLAY_MSG_CURSOR_ROW  1 
#define ADC_DISPLAY_MSG_CURSOR_COL  0
#define ADC_DISPLAY_MSG_LEN         4

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

    mean_adc_value = sum >> 2;

    return mean_adc_value;
}

void read_adc_task()
{
    uint16_t adc_data[ADC_BUFFER_SIZE];
    uint32_t mean_adc_value;
    int i;
    display_params params;
    char mean_adc_value_str[ADC_DISPLAY_MSG_LEN];

	params.cursor_row = ADC_DISPLAY_MSG_CURSOR_ROW;
	params.cursor_col = ADC_DISPLAY_MSG_CURSOR_COL;
	params.msg_len = ADC_DISPLAY_MSG_LEN;

    while (1) {
        // TODO: Only iterate this in 2 milisseconds interval
        for (i = 0; i < ADC_BUFFER_SIZE; i++) {
            if (ESP_OK == adc_read(&adc_data[i])) {
                ESP_LOGI(TAG, "adc read: %d", adc_data[i]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));

        mean_adc_value = get_mean_adc_value(adc_data);
        sprintf(mean_adc_value_str, "%d", mean_adc_value);
        strncpy(params.msg, mean_adc_value_str, ADC_DISPLAY_MSG_LEN);

        BaseType_t DisplaySendReturn = send_to_display_message_queue(&params);
        if (DisplaySendReturn == pdTRUE) {
            ESP_LOGI(TAG, "Mensagem enviada pela fila com sucesso");
        } else {
            ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila");
        }	
    }
}