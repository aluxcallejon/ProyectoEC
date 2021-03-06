#include <msp430.h> 
#include "MSP_general.h"
#include "FT800.h"


/**
 * main.c
 */


unsigned int Yp=135, Xp=200;
unsigned int modo=0;


int main(void)
{
    HAL_Configure_MCU();    //Configura periféricos MC
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	conf_reloj(16);
	inicia_uart();
	inicia_ADC(BIT0+BIT3);
	Inicia_pantalla();      //Iniciar la pantalla


	Conf_Pin(1, 3, P_IN_UP);

	Nueva_pantalla(0x10,0x10,0x10);
	    ComColor(21,160,6);
	    ComRect(10, 10, 470, 262, 1);
	    ComColor(65,202,42);
	    ComRect(12, 12, 468, 260, 1);
	    ComColor(0xff,0xff,0xff);
	    ComTXT(240,50, 22, OPT_CENTERX,"PROYECTO E.CONS 2021 ");
	    ComTXT(240,130, 22, OPT_CENTERX,"Matilde Nieto y Luis Callejon");
	    ComTXT(240,200, 20, OPT_CENTERX," Toca la pantalla para seguir ");
	    ComRect(40, 40, 440, 232, 0);


	    Dibuja();
	    Espera_pant();


	while(1){


	    Xp=lee_ch(0);   //Xp entre 0 y 1023
	    Xp=Xp>>2;        //Xp entre 0 y 256
	    Yp=lee_ch(3);   //Yp entre 0 y 1023
	    Yp=Yp>>2;        //Yp entre 0 y 256

	    if(enviar==1)
	    {
	      uart_putc(modo);
          uart_putc(Xp);
          uart_putc(Yp);

          enviar=0;

	    }




	}
	
}

