/* Aplicacao de uso de teclado de membrama com hardware timer
 * Descricao: Uma aplicacao na qual o usuario tem 15 segundos para digitar
 * uma senha no teclado de membrana, caso o tempo se esgote, a senha se
 * reseta e deve ser selecionada novamente.
 * Autor: Luigi Luz
 * Data: 10 / 01 / 2021
*/

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

static const char *TAG = "application";

#define TEST_ONE_SHOT       false
#define TEST_RELOAD         true

#define GPIO_OUTPUT_IO_0    16
#define GPIO_OUTPUT_IO_1    5
#define GPIO_OUTPUT_IO_2    4
#define GPIO_OUTPUT_IO_3    0
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1) | (1ULL<<GPIO_OUTPUT_IO_2) | (1ULL<<GPIO_OUTPUT_IO_3))

#define GPIO_INPUT_IO_0     2
#define GPIO_INPUT_IO_1     14
#define GPIO_INPUT_IO_2     12
#define GPIO_INPUT_IO_3     13

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) | (1ULL<<GPIO_INPUT_IO_3))

#define TIMEOUT				15 /* Em segundos */
#define TIMER_ALERT			5  /* Tempo no qual vai enviar o alerta */

#define BUFFER_SIZE			6  /* Tamanho do buffer para armazenar os digitos do teclado */

#define KEYPAD_ROWS			4
#define KEYPAD_COLS			4

/* Matriz que define as teclas do teclado */
char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] =
{
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}
};

/* Variaveis que indicam o valor da saida dos GPIO */
int gpio_output_0_bit = 0;
int gpio_output_1_bit = 0;
int gpio_output_2_bit = 0;
int gpio_output_3_bit = 0;

/* Variaveis que armazenam a leitura dos GPIO */
int gpio_input_0_bit = 0;
int gpio_input_1_bit = 0;
int gpio_input_2_bit = 0;
int gpio_input_3_bit = 0;

/* Buffer que vai receber as leituras do teclado */
char keys_buffer[BUFFER_SIZE];

/* Variavel que vai armazenar a contagem de segundos */
volatile int seconds_counter = 0;

/* Variavel que indica a posicao atual no buffer de leitura */
volatile int buffer_index = 0;

/* Variavel que indica que ocorreu um timeout */
volatile int timeout_flag = 0;

/*
 * @brief: Callback a ser executada pelo hardware timer
 * A cada segundo ela incrementa um no contador de segundos "second_counter",
 * o contador reseta quando a contagem atinge o valor predefinido como TIMEOUT.
 * Quando a contagem atinge o valor de timeout, uma "timeout_flag" vai para nivel
 * alto para indicar esse evento
 */
void hw_timer_callback(void *arg)
{
    seconds_counter = seconds_counter + 1;
    if (seconds_counter == TIMEOUT) {
    	timeout_flag = 1;
    	seconds_counter = 0;
    }
}


/*
 * @brief: Funcao que gera o padrao dos sinais de saida
 * Essa funcao gera a seguinte sequencia nos GPIO de saida:
 * 1110, 1101, 1011, 0111
 * e continua gerando isso
 * Essa funcao tem como objetivo multiplexar as linhas do teclado
 * de membrana, para que seja possivel detectar qual a linha da tecla selecionada.
 */
void generate_output_sequence(void)
{
	static int idx = 0;
    int output_byte = 0;

    output_byte = ~(1 << idx);

    gpio_output_0_bit = output_byte & 0b0001;
    gpio_output_1_bit = output_byte & 0b0010;
    gpio_output_2_bit = output_byte & 0b0100;
    gpio_output_3_bit = output_byte & 0b1000;

    idx = (idx + 1) % KEYPAD_ROWS;
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
		else if ((~read_columns[selected_row] & 0x8)) {
			pressed_key = keypad_keys[selected_row][3];
			break;
		}
		else {
			selected_row = selected_row + 1;
		}
	}

	return pressed_key;
}

/*
 * @brief: app_main contem o core da aplicacao
 *
 */
void app_main(void)
{
    /* Configura os pinos GPIO de saida */
    ESP_LOGI(TAG, "Configurando pinos GPIO \n");
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
    
    /* Configura o hardware timer para emitir alarmes a cada 1 segundo */
    hw_timer_init(hw_timer_callback, NULL);
    hw_timer_alarm_us(1000000, TEST_RELOAD);

    int gpio_input = 0;
    char read_cols[KEYPAD_COLS];

    printf("Voce tem %d segundos para digitar sua senha... \n", TIMEOUT);
    
    /* Loop da aplicacao */
    while (1) {
	char pr_key;
	int i;
	
    vTaskDelay(600 / portTICK_RATE_MS);
	
	/* Laço para iterar entre as linhas do teclado */
	for (i = 0; i < KEYPAD_ROWS; i++) {
	generate_output_sequence();

	/* Atribui os niveis aos pinos de saida (linhas do teclado) */
    gpio_set_level(GPIO_OUTPUT_IO_0, gpio_output_0_bit);
    gpio_set_level(GPIO_OUTPUT_IO_1, gpio_output_1_bit);
	gpio_set_level(GPIO_OUTPUT_IO_2, gpio_output_2_bit);
	gpio_set_level(GPIO_OUTPUT_IO_3, gpio_output_3_bit);

	vTaskDelay(pdMS_TO_TICKS(1));

	/* Faz a leitura dos pinos de entrada (colunas do teclado) */
	gpio_input_0_bit = gpio_get_level(GPIO_INPUT_IO_0);
	gpio_input_1_bit = gpio_get_level(GPIO_INPUT_IO_1);
	gpio_input_2_bit = gpio_get_level(GPIO_INPUT_IO_2);
	gpio_input_3_bit = gpio_get_level(GPIO_INPUT_IO_3);
	
	/* Atribui a leitura dos pinos de entrada a uma variavel unica */
	/* O LSB eh a primeira coluna e o MSB eh a ultima coluna */
	gpio_input = (gpio_input_0_bit) | (gpio_input_1_bit << 1) | (gpio_input_2_bit << 2) | (gpio_input_3_bit << 3);
	read_cols[i] = gpio_input;

	}

	/* Faz o mapeamento da leitura para a tecla do teclado */
	pr_key = get_key_from_keypad(read_cols);

	/* Faz o tratamento da tecla que foi pressionada e adiciona ela no buffer */
	if (pr_key != '\0') {
		keys_buffer[buffer_index] = pr_key;
		buffer_index = buffer_index + 1;
		ESP_LOGI(TAG, "Restam %d digitos \n", BUFFER_SIZE - buffer_index);
	}

	/* Exibe a senha quando o buffer fica completo */
	if (buffer_index == BUFFER_SIZE) {
		printf("\n");
		printf("A senha escolhida foi: ");
		for (int k = 0; k < BUFFER_SIZE; k++) {
			printf("%c", keys_buffer[k]);
		}
		printf("\n");
		buffer_index = 0;
		seconds_counter = 0;
		printf("Senha cadastrada com sucesso!\n");
		printf("Voce tem %d segundos para cadastrar uma nova senha... \n", TIMEOUT);
	}
	
	/* Avisa do tempo restante */
	if (seconds_counter == TIMEOUT - TIMER_ALERT) {
		printf("Restam apenas %d segundos! \n", TIMER_ALERT);
	}
	
	/* Reinicia o buffer quando ocorre um timeout */
	if (timeout_flag) {
		timeout_flag = 0;
		buffer_index = 0;
		printf("\n");
		printf("O tempo foi esgotado! \n"); 
		printf("Voce tem %d segundos para cadastrar uma nova senha... \n", TIMEOUT);
	}
	
    }
}


