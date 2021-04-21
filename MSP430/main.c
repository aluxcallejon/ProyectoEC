#include <msp430.h> 
#include "MSP_general.h"


/**
 * main.c
 */



int main(void)
{

	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	conf_reloj(16);
	inicia_uart();
	int i=0;
	Conf_Pin(1, 3, P_IN_UP);
	while(1){
	if(enviar==1){

	uart_putc(i);
	i=i+2;
	enviar=0;

	}
	}
	
}

