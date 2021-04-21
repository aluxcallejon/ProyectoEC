/*
 * MSP_general.h
 *
 *  Created on: 30 abr. 2018
 *      Author: ManoloP
 */

#ifndef MSP_GENERAL_H_
#define MSP_GENERAL_H_



enum conf_pines{P_IN,P_OUT,P_IN_UP,P_IN_DOWN};


#define ST_PIN BIT3
#define STATE P2IN&ST_PIN


void espera_boton(void);
void conf_reloj(char VEL);
void inicia_ADC(char canales);
int lee_ch(char canal);

void Conf_Pin(char p, char bit, char modo);
void Enciende(char p, char bit);
void Apaga(char p, char bit);
int Lee(char p, char bit);

void Mueve_servo(int duty);
void Conf_servo(void);

void espera(unsigned int ms);

void inicia_uart(void);
void uart_putc(unsigned char c);
void uart_puts(const char *str);
void uart_putfr(const char *str);

#define MENS_MAX 3

extern char FinRx,idx;
extern char mensaje[MENS_MAX];
extern int enviar;


#endif /* MSP_GENERAL_H_ */
