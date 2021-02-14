#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "display.h"

#include "jsmn.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "GET_FROM_UART";

#define BUF_SIZE (256)
#define MAX_JSMN_TOKENS 32

/*
 * Os valores abaixo sao definidos considerando que se espera uma mensagem do seguinte tipo:
 * {"msg": "mensagem"}
 * Dessa forma, o valor da chave "cursor" esta no token 2 e o valor da chave "msg" esta no token 4
 */
#define JSMN_PARSER_EXPECTED_RETURN     3
// #define JSMN_CURSOR_KEY_INDEX           1
// #define JSMN_CURSOR_VALUE_INDEX         2
#define JSMN_MSG_KEY_INDEX              1
#define JSMN_MSG_VALUE_INDEX            2

#define UART_DISPLAY_MSG_LEN            8
#define UART_DISPLAY_MSG_CURSOR_ROW     0
#define UART_DISPLAY_MSG_CURSOR_COL     0

#define JSMN_CURSOR_KEY                 "cursor"
#define JSMN_CURSOR_KEY_LEN             sizeof(JSMN_CURSOR_KEY) / sizeof(char)
#define JSMN_MSG_KEY                    "msg"
#define JSMN_MSG_KEY_LEN                sizeof(JSMN_MSG_KEY) / sizeof(char)


// static QueueHandle_t get_from_uart_mq_handle;

// void get_from_uart_init_queue(void)
// {
// 	if (get_from_uart_mq_handle == NULL)
// 		get_from_uart_mq_handle = xQueueCreate(1, sizeof(display_msg));
// }

// BaseType_t get_from_uart_read_queue(display_msg *received_msg)
// {
// 	BaseType_t GetFromUartQueueReturn;
// 	GetFromUartQueueReturn = xQueueReceive(get_from_uart_mq_handle, received_msg, 0);

// 	return GetFromUartQueueReturn;
// }

int get_int_key_from_token(char *json_msg, jsmntok_t *token, int token_number)
{
    int key_start = token[token_number].start;
    int key_end = token[token_number].end;
    char char_key = 0;
    int int_key;

    // TODO: Add validaton to check if it is a single number
    if (key_end - key_start != 1)
        return -1;

    char_key = json_msg[key_start];

    int_key = ((int)char_key) - ((int)'0');

    return int_key;
}

void get_char_key_from_token(char *json_msg, jsmntok_t *token, int token_number, char *key)
{
    int key_start = token[token_number].start;
    int key_end = token[token_number].end;
    int i;
    int k;

    for (i = key_start; i < key_end; i++) {
        k = i - key_start;
        key[k] = json_msg[i];

        if (k == key_end - 1)
            key[k + 1] = '\0'; 
    }
}

int parse_json_to_display_msg(char *temp, jsmntok_t *token, display_params *params)
{
    int msg_key_ret;
    // int cursor_key_ret;
    // int cursor_value;
    int ret;
    char msg_value[UART_DISPLAY_MSG_LEN];
    // char cursor_key[JSMN_CURSOR_KEY_LEN + 1];
    char msg_key[JSMN_MSG_KEY_LEN + 1];

    // memset(cursor_key, '\0', JSMN_CURSOR_KEY_LEN + 1);
    memset(msg_key, '\0', JSMN_MSG_KEY_LEN + 1);
    memset(msg_value, '\0', UART_DISPLAY_MSG_LEN);

    // get_char_key_from_token(temp, token, JSMN_CURSOR_KEY_INDEX, cursor_key);
    // cursor_key_ret = strncmp(cursor_key, JSMN_CURSOR_KEY, JSMN_CURSOR_KEY_LEN + 1);
    // ESP_LOGI(TAG, "cursor_key : %s", cursor_key);
    // if (cursor_key_ret != 0) {
    //     ret = -1;
    //     return ret;
    // }
    // ESP_LOGI(TAG, "cursor_key_ret: %d", cursor_key_ret);
    // cursor_value = get_int_key_from_token(temp, token, JSMN_CURSOR_VALUE_INDEX);

    get_char_key_from_token(temp, token, JSMN_MSG_KEY_INDEX, msg_key);
    msg_key_ret = strncmp(msg_key, JSMN_MSG_KEY, JSMN_MSG_KEY_LEN + 1);
    if (msg_key_ret != 0) {
        ret = -2;
        return ret;
    }
    ESP_LOGI(TAG, "msg_key_ret: %d", msg_key_ret);
    get_char_key_from_token(temp, token, JSMN_MSG_VALUE_INDEX, msg_value);

    // msg->cursor = cursor_value;
    strncpy(params->msg, msg_value, UART_DISPLAY_MSG_LEN);
    ESP_LOGI(TAG, "msg_value: %s", msg_value);
    // ESP_LOGI(TAG, "cursor: %d", msg->cursor);
    ESP_LOGI(TAG, "msg: %s", params->msg);

    ret = 0;

    return ret;
}

void get_from_uart_task(void *pvParameters)
{
    display_params params;
    params.cursor_row = UART_DISPLAY_MSG_CURSOR_ROW;
    params.cursor_col = UART_DISPLAY_MSG_CURSOR_COL;
    params.msg_len = UART_DISPLAY_MSG_LEN;
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    char temp[64];
    int ret;
    jsmn_parser parser;
    jsmntok_t token[MAX_JSMN_TOKENS];

    while(1) {
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 2000 / portTICK_RATE_MS);
        if (len > 0) {
            data[len] = '\0';
            sprintf(temp, "%s", data);

            jsmn_init(&parser);
            ret = jsmn_parse(&parser, temp, strlen(temp), token, MAX_JSMN_TOKENS);

            if (ret != JSMN_PARSER_EXPECTED_RETURN)
                ESP_LOGI(TAG, "Mensagem com formato invalido");
            
            if (ret == JSMN_PARSER_EXPECTED_RETURN) {
                ESP_LOGI(TAG, "json com tamanho correto");
                ret = parse_json_to_display_msg(temp, token, &params);

                if (ret == 0) {
                    
                    display_semphr_take();

                    BaseType_t DisplaySendReturn = send_to_display_message_queue(&params);
                    if (DisplaySendReturn == pdTRUE) {
                        ESP_LOGI(TAG, "Mensagem enviada pela fila com sucesso");
                    } else {
                        ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila");
                    }

                    display_semphr_give();

			    }	
            } else {
                ESP_LOGI(TAG, "Mensagem no formato invalido");
            }
        }
    }
}