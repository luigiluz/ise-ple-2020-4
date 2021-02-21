#ifndef KEYPAD_H_
#define KEYPAD_H_

/* Tamanho do buffer para armazenar os digitos do teclado */
#define BUFFER_SIZE			4

extern void keypad_init(void);

extern void keypad_task(void *pvParameters);

#endif
