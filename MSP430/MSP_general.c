/*
 * MSP_general.c
 *
 *  Created on: 30 abr. 2018
 *      Author: ManoloP
 */



#include <msp430.h>
#include "MSP_general.h"


char FinRx=0, idx=0;
char mensaje[MENS_MAX];
int enviar;


void espera_boton(void)
{
   while((Lee(1,3)));
//   espera(20);
   while(!(Lee(1,3)));
 //  espera(20);
}
void conf_reloj(char VEL){
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    switch(VEL){
    case 1:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;
    case 8:

        if (CALBC1_8MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_8MHZ;      /* Set DCO to 8MHz */
            DCOCTL = CALDCO_8MHZ;
        }
        break;
    case 12:
        if (CALBC1_12MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_12MHZ;     /* Set DCO to 12MHz */
            DCOCTL = CALDCO_12MHZ;
        }
        break;
    case 16:
        if (CALBC1_16MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ;     /* Set DCO to 16MHz */
            DCOCTL = CALDCO_16MHZ;
        }
        break;
    default:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
            DCOCTL = CALDCO_1MHZ;
        }
        break;

    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;

}


void espera(unsigned int ms)
{
    while(ms)
    {
        __delay_cycles(16000);
        ms--;
    }
}

int lee_ch(char canal){
    ADC10CTL0 &= ~ENC;                  //deshabilita el ADC
    ADC10CTL1&=(0x0fff);                //Borra canal anterior
    ADC10CTL1|=canal<<12;               //selecciona nuevo canal
    ADC10CTL0|= ENC;                    //Habilita el ADC
    ADC10CTL0|=ADC10SC;                 //Empieza la conversión
    while(ADC10CTL1&BUSY);              //Espera fin
    return(ADC10MEM);                   //Devuelve valor leido
    }

void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;      //deshabilita ADC
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0; //enciende ADC, S/H lento, REF:VCC, SIN INT
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    //Modo simple, reloj ADC, sin subdivision, Disparo soft, Canal 0
    ADC10AE0 = canales; //habilita los canales indicados
    ADC10CTL0 |= ENC; //Habilita el ADC
}


void Conf_Pin(char p, char bit, char modo)
{
    switch(modo){
    case P_IN:
        if(p==1) P1DIR&=~(1<<bit);
        if(p==2) P2DIR&=~(1<<bit);
        break;
    case P_OUT:
        if(p==1) P1DIR|=(1<<bit);
        if(p==2) P2DIR|=(1<<bit);
        break;
    case P_IN_UP:
        if(p==1){
            P1DIR&=~(1<<bit);
            P1REN|=(1<<bit);
            P1OUT|=(1<<bit);
        }
        if(p==2){
            P2DIR&=~(1<<bit);
            P2REN|=(1<<bit);
            P2OUT|=(1<<bit);
        }
        break;
    case P_IN_DOWN:
        if(p==1){
            P1DIR&=~(1<<bit);
            P1REN|=(1<<bit);
            P1OUT&=~(1<<bit);
        }
        if(p==2){
            P2DIR&=~(1<<bit);
            P2REN|=(1<<bit);
            P2OUT&=~(1<<bit);
        }
        break;

    }

}

void Enciende(char p, char bit)
{
    if(p==1) P1OUT|=(1<<bit);
    if(p==2) P2OUT|=(1<<bit);}

void Apaga(char p, char bit)
{
    if(p==1) P1OUT&=~(1<<bit);
    if(p==2) P2OUT&=~(1<<bit);
        }

int Lee(char p, char bit){
    if(p==1 &&(P1IN&(1<<bit))) return 1;
    if(p==2 &&(P2IN&(1<<bit))) return 1;
    return 0;
}



/**********************
 * Función para configurar el pin P2.6 como PWM para manejar un servo
 * void Conf_servo(void)
 * supuesto Clock= 16.
 *
 **********************/

void Conf_servo(void)
{
    //Configurar pin
    P2DIR |= BIT6;
    P2SEL |=BIT6;
    P2SEL2 &= ~(BIT6+BIT7);
    P2SEL &=~BIT7;
    //Conf. Timer
    TA0CCTL0=0;       //CCIE=1
    TA0CCTL1=OUTMOD_7;  //OUTMOD=7
    TA0CTL=TASSEL_2|ID_3|MC_1;  //SMCLK, DIV=8, UP 2MHz
    TA0CCR0=40000;      //periodo=40000: 20ms
    TA0CCR1=3000;       //duty cycle inicial 1.5ms

}

/********************
 * Mueve_servo(char duty)
 * duty debe ser un número entre 0 y 100.
 *
 ********************/
#define MIN_SERVO 1300
#define MAX_SERVO 5100
void Mueve_servo(int duty)
{
    if(duty>=0 && duty<=100)
    {
        TA0CCR1=MIN_SERVO +duty*((MAX_SERVO-MIN_SERVO)/100);
    }
}

void inicia_uart(void)
{


    //Ponemos el P1.1 y el P1.2 como RXD y TXD respectivamente
        P1SEL2 |= BIT1 | BIT2;
        P1SEL |= BIT1 | BIT2;
        P2DIR |= BIT1;
        P2DIR &= ~BIT0;




 //Ponemos el reloj
        UCA0CTL1 |= UCSWRST;
        UCA0CTL1 = UCSSEL_2 | UCSWRST;
        //Elegimos modulacion y baudrate
        UCA0MCTL = UCBRF_0 | UCBRS_6;
        UCA0BR0 = 130;  // SUPUESTOS 16MHZ y que queremos 9600
        UCA0BR1 = 6;
        //Preescalado=(UCA0BR0+UCABR1*256)=1666
        //baudrate=Reloj/Preescalado=16MHz/256=9600 baudios

        UCA0CTL1 &= ~UCSWRST;
        IFG2 &= ~(UCA0RXIFG);
        IE2 |= UCA0RXIE;
        P2OUT|=BIT1;
        __bis_SR_register(GIE);

}


void uart_putc(unsigned char c)
{
    while (!(IFG2&UCA0TXIFG)); // Espera Tx libre
    UCA0TXBUF = c;             // manda dato
}
void uart_puts(const char *str)
{
     while(*str) uart_putc(*str++); //repite mientras !=0
}
void uart_putfr(const char *str)
{
     while(*str) uart_putc(*str++);
     uart_puts("\n\r");     //termina con CR+LF
}



#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    char mens;
    mens=UCA0RXBUF;
    if(mens=='r');
    enviar=1;
}


