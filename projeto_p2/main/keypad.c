#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"

#include "esp_log.h"
#include "esp_system.h"

#include "keypad.h"
#include "display.h"
#include "send_to_uart.h"

static const char *TAG = "KEYPAD";

#define KEYPAD_DISPLAY_MSG_CURSOR_ROW		0
#define KEYPAD_DISPLAY_MSG_CURSOR_COL		12
#define KEYPAD_DISPLAY_MSG_LEN				4

#define TEST_ONE_SHOT       false
#define TEST_RELOAD         true

#define GPIO_OUTPUT_IO_0    16
#define GPIO_OUTPUT_IO_1    0
#define GPIO_OUTPUT_IO_2    2
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1) | (1ULL<<GPIO_OUTPUT_IO_2))

#define GPIO_INPUT_IO_0     14
#define GPIO_INPUT_IO_1     12
#define GPIO_INPUT_IO_2     13

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2))

#define TIMEOUT				15 /* Em segundos */
#define TIMER_ALERT			5  /* Tempo no qual vai enviar o alerta */

#define KEYPAD_ROWS				3
#define KEYPAD_COLS				3

#define HWTIMER_INT_TIME    1000000 /* Em microsegundos */

/* Matriz que define as teclas do teclado */
char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] =
{
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'}
};

/* Variavel que vai armazenar a contagem de segundos */
volatile int seconds_counter = 0;
volatile int timeout_flag = 0;

/*
 * @brief: Inicializa os pinos GPIO a serem utilizados pelo display
 *
 */
void init_gpio(void)
{
    /* Configura os pinos GPIO de saida */
    ESP_LOGI(TAG, "Configurando pinos GPIO");
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    /* Configura os pino GPIO como entrada PULLUP */
    gpio_config_t input_conf;
    input_conf.intr_type = GPIO_INTR_DISABLE;
    input_conf.mode = GPIO_MODE_INPUT;
    input_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    input_conf.pull_down_en = 0;
    input_conf.pull_up_en = 1;
    gpio_config(&input_conf);
}

void keypad_init(void)
{
	init_gpio();
}

/*
 * @brief: Funcao que gera o padrao dos sinais de saida
 * Essa funcao gera a seguinte sequencia nos GPIO de saida:
 * 1110, 1101, 1011, 0111
 * e continua gerando isso
 * Essa funcao tem como objetivo multiplexar as linhas do teclado
 * de membrana, para que seja possivel detectar qual a linha da tecla selecionada.
 */
int generate_output_sequence(void)
{
	static int idx = 0;
    int output_sequence = 0;

    output_sequence = ~(1 << idx);
    idx = (idx + 1) % KEYPAD_ROWS;
    
    return output_sequence;
}

void gpio_write_sequence(int output_sequence)
{	
	int gpio_output_0_bit = 0;
	int gpio_output_1_bit = 0;
	int gpio_output_2_bit = 0;

    gpio_output_0_bit = output_sequence & 0b0001;
    gpio_output_1_bit = output_sequence & 0b0010;
    gpio_output_2_bit = output_sequence & 0b0100;
    
	/* Atribui os niveis aos pinos de saida (linhas do teclado) */
	gpio_set_level(GPIO_OUTPUT_IO_0, gpio_output_0_bit);
	gpio_set_level(GPIO_OUTPUT_IO_1, gpio_output_1_bit);
	gpio_set_level(GPIO_OUTPUT_IO_2, gpio_output_2_bit);
}

int gpio_read_pins(void)
{
	/* Variaveis que armazenam a leitura dos GPIO */
	int gpio_input_0_bit = 0;
	int gpio_input_1_bit = 0;
	int gpio_input_2_bit = 0;
	int gpio_input;

	/* Faz a leitura dos pinos de entrada (colunas do teclado) */
	gpio_input_0_bit = gpio_get_level(GPIO_INPUT_IO_0);
	gpio_input_1_bit = gpio_get_level(GPIO_INPUT_IO_1);
	gpio_input_2_bit = gpio_get_level(GPIO_INPUT_IO_2);
	
	/* Atribui a leitura dos pinos de entrada a uma variavel unica */
	/* O LSB eh a primeira coluna e o MSB eh a ultima coluna */
	gpio_input = (gpio_input_0_bit) | (gpio_input_1_bit << 1) | (gpio_input_2_bit << 2);// | (gpio_input_3_bit << 3);
	
	return gpio_input;
}

/*
 * @brief: Funcao que faz o mapeamento da tecla pressionada com o digito do teclado
 * Essa funcao confere a leitura das colunas e das linhas do teclado para identificar
 * qual a linha e coluna da tecla pressionada.
 */
char get_key_from_keypad(char read_columns[KEYPAD_COLS])
{
	char pressed_key = '\0';
	int selected_row = 0;

	while (selected_row < KEYPAD_ROWS) {
	
		if ((~read_columns[selected_row] & 0x1)) {
			pressed_key = keypad_keys[selected_row][0];
			break;
		} 
		else if ((~read_columns[selected_row] & 0x2)) {
			pressed_key = keypad_keys[selected_row][1];
			break;
		}
		else if ((~read_columns[selected_row] & 0x4)) {
			pressed_key = keypad_keys[selected_row][2];
			break;
		}
		else {
			selected_row = selected_row + 1;
		}
	}

	return pressed_key;
}

uart_msg_t build_keypad_uart_msg(char *keys_buffer)
{
	uart_msg_t uart_msg;
	static char keypad_json_msg[60];

	sprintf(keypad_json_msg,
		"{ \"key_0\": \"%c\", \"key_1\": \"%c\", \"key_2\": \"%c\", \"key_3\": \"%c\" }\n",
		keys_buffer[0], keys_buffer[1], keys_buffer[2], keys_buffer[3]);
	
	uart_msg.msg = keypad_json_msg;
	uart_msg.msg_len = sizeof(keypad_json_msg) / sizeof(keypad_json_msg[0]);

	return uart_msg;
}

void keypad_task(void *pvParameters)
{
	display_params params;

	params.tsk_id = KEYPAD_TASK_ID;
	params.cursor_row = KEYPAD_DISPLAY_MSG_CURSOR_ROW;
	params.cursor_col = KEYPAD_DISPLAY_MSG_CURSOR_COL;
	params.msg_len = KEYPAD_DISPLAY_MSG_LEN;

	while(1) {
		char keys_buffer[BUFFER_SIZE];
    	char read_cols[KEYPAD_COLS];
    	char pr_key;
    	int output_sequence;
   		int i;
		static int buffer_index = 0;
		uart_msg_t uart_msg;

		vTaskDelay(600 / portTICK_RATE_MS);
	
		/* Laço para iterar entre as linhas do teclado */
		for (i = 0; i < KEYPAD_ROWS; i++) {
			output_sequence = generate_output_sequence();
			gpio_write_sequence(output_sequence);

			vTaskDelay(pdMS_TO_TICKS(1));

			read_cols[i] = gpio_read_pins();
		}

		/* Faz o mapeamento da leitura para a tecla do teclado */
		pr_key = get_key_from_keypad(read_cols);

		/* Faz o tratamento da tecla que foi pressionada e adiciona ela no buffer */
		if (pr_key != '\0') {
			keys_buffer[buffer_index] = pr_key;
			buffer_index = buffer_index + 1;
			ESP_LOGI(TAG, "Restam %d digitos", BUFFER_SIZE - buffer_index);
		}

		/* Exibe a senha quando o buffer fica completo */
		if (buffer_index == BUFFER_SIZE) {
			char tmp_keys_buffer[BUFFER_SIZE + 1];

			strncpy(tmp_keys_buffer, keys_buffer, BUFFER_SIZE);

			/* Adiciona o string terminator */
			tmp_keys_buffer[BUFFER_SIZE] = '\0';
			strncpy(params.msg, tmp_keys_buffer, BUFFER_SIZE);

            BaseType_t DisplaySendReturn = display_append_to_message_queue(&params);
            if (DisplaySendReturn != pdTRUE) {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila do display");
            }

			uart_msg = build_keypad_uart_msg(keys_buffer);

			BaseType_t SendToUartReturn = send_to_uart_append_to_message_queue(&uart_msg);
            if (SendToUartReturn != pdTRUE) {
                ESP_LOGI(TAG, "Falha ao enviar a mensagem pela fila da uart");
            }

			buffer_index = 0;
			seconds_counter = 0;
		}
		vTaskDelay(pdMS_TO_TICKS(300));
	}
}
