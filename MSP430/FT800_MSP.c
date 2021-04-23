/*
 * FT800_MSP.c
 *
 *  Created on: 6 mar. 2019
 *      Author: ManoloP
 */



#include "FT800.h"
#include "MSP_general.h"

/*
 * ======== Standard MSP430 includes ========
 */
#include <msp430.h>


unsigned int t;
//extern int Fin_Rx;
char Buffer_Rx;

#define dword long
#define byte char


char chipid=0 ;                        // Holds value of Chip ID read from the FT800

unsigned long cmdBufferRd=0;         // Store the value read from the REG_CMD_READ register
unsigned long cmdBufferWr=0;         // Store the value read from the REG_CMD_WRITE register
unsigned int CMD_Offset;
unsigned long POSX, POSY, BufferXY;

const long REG_CAL[6]={21696,-78,-614558,498,-17021,15755638};
const long REG_CAL5[6]={32146, -1428, -331110, -40, -18930, 18321010};


// ############################################################################################################
// Hardware-Specific Functions
// ############################################################################################################

void HAL_Configure_MCU(void)
{

        WDTCTL = WDTPW | WDTHOLD;
        BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;

        if (CALBC1_16MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ;     /* Set DCO to 16MHz */
            DCOCTL = CALDCO_16MHZ;
        }
        BCSCTL1 |= XT2OFF | DIVA_0;
        BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;

            P1SEL2 |= BIT5 | BIT6 | BIT7;
            P1SEL |= BIT5 | BIT6 | BIT7;
            P2DIR |= BIT4+BIT5;
            P2OUT|= BIT4+BIT5;


            UCB0CTL1 |= UCSWRST;
            UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;
            UCB0CTL1 = UCSSEL_2 | UCSWRST;
            UCB0BR0 = 2;       //8MHz
            UCB0CTL1 &= ~UCSWRST;

}



// ============================================================================================================
// Writes a byte to the MCU's SPI hardware and reads the byte received at the same time
// ============================================================================================================
unsigned char HAL_SPI_ReadWrite(unsigned char data)
{
    //Fin_Rx=0;
    UCB0TXBUF = data;
    while (UCB0STAT&UCBUSY);
    Buffer_Rx=UCB0RXBUF;

    return Buffer_Rx;
}

// ============================================================================================================
// Puts Chip Select pin Low
// ============================================================================================================

void HAL_SPI_CSLow(void)
{
    P2OUT&=~BIT5;                   // Write the value back to the port
}

// ============================================================================================================
// Puts Chip Select pin High
// ============================================================================================================

void HAL_SPI_CSHigh(void)
{
   P2OUT|=BIT5;                 // Write the value back to the port
}

// ============================================================================================================
// Puts FT800 Power_Down_# pin Low
// ============================================================================================================

void HAL_SPI_PDlow(void)
{
    P2OUT&=~BIT4;                    // Write the value back to the port
}

// ============================================================================================================
// Puts FT800 Power_Down_# pin high
// ============================================================================================================

void HAL_SPI_PDhigh(void)
{
    P2OUT|=BIT4;                    // Write the value back to the port
}

// ============================================================================================================
// General Functions
// ============================================================================================================

void Delay(void)
{
    unsigned int outer_count = 0x0000;
    unsigned int inner_count = 0x0000;

    outer_count = 0x0004;
    inner_count = 0xFFFF;

    while(outer_count > 0x0000)
    {
        while (inner_count > 0x0000)
        {
            inner_count --;
            asm(" NOP");
        }
    outer_count --;
    }
}




// ############################################################################################################
// FT800 SPI Functions
// ############################################################################################################



// ============================================================================================================
// Send address of a register which is to be Written
// ============================================================================================================

// This function sends an address of a register which is to be written. It sets the MSBs to 10 to indicate a write
//
// Pass in:         00000000 xxxxxxxx yyyyyyyy zzzzzzzz     address as double word 32-bit value, only lower 3 bytes used
//
// Sends:           10xxxxxx                                bits 23 - 16 of the address, with bit 23/22 forced to 10
//                  yyyyyyyy                                bits 15 - 8  of the address
//                  zzzzzzzz                                bits 7 -  0  of the address
//
// Returns:         N/A

void FT800_SPI_SendAddressWR(dword Memory_Address)
{
    byte SPI_Writebyte = 0x00;

    // Write out the address. Only the lower 3 bytes are sent, with the most significant byte sent first
    SPI_Writebyte = ((Memory_Address & 0x00FF0000) >> 16);  // Mask off the first byte to send
    SPI_Writebyte = (SPI_Writebyte & 0xBF | 0x80);          // Since this is a write, the MSBs are forced to 10
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte                                                   //

    SPI_Writebyte = ((Memory_Address & 0x0000FF00) >> 8);   // Mask off the next byte to be sent
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte

    SPI_Writebyte = (Memory_Address & 0x000000FF);          // Mask off the next byte to be sent (least significant byte)
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte

}


// ============================================================================================================
// Send address of a register which is to be Read
// ============================================================================================================

// This function sends an address of a register which is to be read. It clears the two MS bits to indicate a read
//
// Pass in:         00000000 xxxxxxxx yyyyyyyy zzzzzzzz     address as double word 32-bit value, only lower 3 bytes used
//
// Sends:           00xxxxxx                                bits 23 - 16 of the address, with bits 23/22 forced to 0
//                  yyyyyyyy                                bits 15 - 8  of the address
//                  zzzzzzzz                                bits 7 -  0  of the address
//                  00000000                                dummy byte 0x00 written as described in FT800 datasheet
//
// Returns:         No return value

void FT800_SPI_SendAddressRD(dword Memory_Address)
 {
    long porsi;
    porsi=Memory_Address;
    byte SPI_Writebyte = 0x00;

    // Write out the address. Only the lower 3 bytes are sent, with the most significant byte sent first
    SPI_Writebyte = (char)((porsi & 0x00FF0000) >> 16);  // Mask off the first byte to send
    SPI_Writebyte = (SPI_Writebyte & 0x3F);                 // Since this is a read, the upper two bits are forced to 00
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte

    SPI_Writebyte = ((Memory_Address & 0x0000FF00) >> 8);   // Mask off the next byte to be sent
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte

    SPI_Writebyte = (Memory_Address & 0x000000FF);          // Mask off the next byte to be sent (least significant byte)
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Call the low-level SPI routine for this MCU to send this byte

    // Send dummy 00 as required in the FT800 datasheet when doing a read
    SPI_Writebyte = 0x00;                                   // Write dummy byte of 0
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //

 }


// ============================================================================================================
// Write a 32-bit value
// ============================================================================================================

// This function writes a 32-bit value. Call FT800_SPI_SendAddressWR first to specify the register address
//
// Pass in:         pppppppp qqqqqqqq rrrrrrrr ssssssss             32-bit value to be written to the register
//
// Sends:           ssssssss         least signicant byte first
//                  rrrrrrrr
//                  qqqqqqqq
//                  pppppppp         most significant byte last
//
// Returns:         N/A

void FT800_SPI_Write32(dword SPIValue32)
{
    byte SPI_Writebyte = 0x00;

    SPI_Writebyte = (SPIValue32 & 0x000000FF);              //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Write the first (least significant) byte

    SPI_Writebyte = ((SPIValue32 & 0x0000FF00) >> 8);       //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //

    SPI_Writebyte = (SPIValue32 >> 16);                     //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //

    SPI_Writebyte = ((SPIValue32 & 0xFF000000) >> 24);      //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Write the last (most significant) byte

}


// ============================================================================================================
// Write a 16-bit value
// ============================================================================================================

// This function writes a 16-bit value. Call FT800_SPI_SendAddressWR first to specify the register address
//
// Pass in:         rrrrrrrr ssssssss               16-bit value to be written to the register
//
// Sends:           ssssssss                        least significant byte first
//                  rrrrrrrr                        most significant byte last
//
// Returns:         N/A

void FT800_SPI_Write16(unsigned int SPIValue16)
{
    byte SPI_Writebyte = 0x00;

    SPI_Writebyte = (SPIValue16 & 0x00FF);                  //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Write the first (least significant) byte

    SPI_Writebyte = ((SPIValue16 & 0xFF00) >> 8);           //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Write the last (most significant) byte
}


// ============================================================================================================
// Write an 8-bit value
// ============================================================================================================

// This function writes an 8-bit value. Call FT800_SPI_SendAddressWR first to specify the register address
//
// Pass in:         ssssssss          16-bit value to be written to the register
//
// Sends:           ssssssss          sends the specified byte
//
// Returns:         N/A

void FT800_SPI_Write8(byte SPIValue8)
{
    byte SPI_Writebyte = 0x00;

    // Write out the data
    SPI_Writebyte = (SPIValue8);                            //
    HAL_SPI_ReadWrite(SPI_Writebyte);                       // Write the data byte
}


// ============================================================================================================
// Read a 32-bit register
// ============================================================================================================

// This function reads the actual 8-bit data from a register. Call FT800_SPI_SendAddressRD first to specify the address
//
// Pass In:         N/A
//
// Sends:           00000000        dummy byte whilst clocking in the Least Significant byte (e.g. aaaaaaaa) from register
//                  00000000        dummy byte whilst clocking in the next byte (e.g. bbbbbbbb) from register
//                  00000000        dummy byte whilst clocking in the next byte (e.g. cccccccc) from register
//                  00000000        dummy byte whilst clocking in the Most Significant byte (e.g. dddddddd) from register
//
// Returns:         dddddddd cccccccc bbbbbbbb aaaaaaaa

dword FT800_SPI_Read32()
 {
    byte SPI_Writebyte = 0x00;
    byte SPI_Readbyte = 0x00;
    dword SPI_DWordRead = 0x00000000;
    dword DwordTemp = 0x00000000;

    // Read the first data byte (this is the least significant byte of the register). SPI writes out dummy byte of 0x00
    SPI_Writebyte = 0x00;                                   // Write a dummy 0x00
    SPI_Readbyte = HAL_SPI_ReadWrite(SPI_Writebyte);        // Get the byte which was read by the low-level MCU routine
    DwordTemp = SPI_Readbyte;                               // Put received byte into a temporary 32-bit value
    SPI_DWordRead = DwordTemp;                              // Put the byte into a 32-bit variable in the lower 8 bits

    // Read the actual data byte. We pass a 0x00 into the SPI routine since it always writes when it reads
    SPI_Writebyte = 0x00;                                   // Write a dummy 0x00
    SPI_Readbyte = HAL_SPI_ReadWrite(SPI_Writebyte);        // Get the byte which was read by the low-level MCU routine
    DwordTemp = SPI_Readbyte;                               // Put received byte into a temporary 32-bit value
    SPI_DWordRead |= (DwordTemp << 8);                      // Put the byte into a 32-bit variable (shifted 8 bits up)

    // Read the actual data byte. We pass a 0x00 into the SPI routine since it always writes when it reads
    SPI_Writebyte = 0x00;                                   // Write a dummy 0x00
    SPI_Readbyte = HAL_SPI_ReadWrite(SPI_Writebyte);        // Get the byte which was read by the low-level MCU routine
        DwordTemp = SPI_Readbyte;                           // Put received byte into a temporary 32-bit value
    SPI_DWordRead |= (DwordTemp << 16);                     // Put the byte into a 32-bit variable (shifted 16 bits up)

    // Read the actual data byte. We pass a 0x00 into the SPI routine since it always writes when it reads
    SPI_Writebyte = 0x00;                                   // Write a dummy 0x00
    SPI_Readbyte = HAL_SPI_ReadWrite(SPI_Writebyte);        // Get the byte which was read by the low-level MCU routine
        DwordTemp = SPI_Readbyte;                           // Put received byte into a temporary 32-bit value
    SPI_DWordRead |= (DwordTemp << 24);                     // Put the byte into a 32-bit variable (shifted 24 bits up)

    // Return the byte which we read
    return(SPI_DWordRead);                                  //
}


// ============================================================================================================
// Read an 8-bit register
// ============================================================================================================

// This function reads the actual 8-bit data from a register. Call FT800_SPI_SendAddressRD first to specify the address
//
// Pass in:         N/A
//
// Sends:           00000000                                Dummy write 0x00 whilst clocking in the byte
//
// Returns:         aaaaaaaa                                Byte which was clocked in

byte FT800_SPI_Read8()
 {
    byte SPI_Writebyte = 0x00;
    byte SPI_Readbyte = 0x00;

    // Read the data byte. We pass a 0x00 into the SPI routine since it always writes when it reads
    SPI_Writebyte = 0x00;                                   // Write a dummy 0x00
    SPI_Readbyte = HAL_SPI_ReadWrite(SPI_Writebyte);        // Get the byte which was read by the low-level MCU routine

    // Return the byte which we read
    return(SPI_Readbyte);                                   //
}


// ============================================================================================================
// Write a Host Command
// ============================================================================================================

// This function is specifically designed to send a Host Command
//
// Pass in:         xxhhhhhh             8-bit host command where upper 2 bits are dont-care
//
// Sends:           [Chip Select Low]
//                  01hhhhhh         Host command with bit 7 forced to 0 and bit 6 forced to 1
//                  00000000         Dummy 0x00
//                  00000000         Dummy 0x00
//                  [Chip Select High]
//
// Returns:         N/A

void FT800_SPI_HostCommand(byte Host_Command)
{
    byte SPI_Writebyte = 0x00;

    // Chip Select Low
    HAL_SPI_CSLow();

    SPI_Writebyte = (Host_Command & 0x3F | 0x40);           // This is the command being sent
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //

    SPI_Writebyte = 0x00;                                   // Sending dummy byte
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //


    SPI_Writebyte = 0x00;                                   // Sending dummy byte
    HAL_SPI_ReadWrite(SPI_Writebyte);                       //


    // Chip Select High
    HAL_SPI_CSHigh();
}


// ============================================================================================================
// Host Command Dummy Read
// ============================================================================================================

// This function is specifically designed to send three 0x00 bytes to wake up the FT800
//
// Pass in:         N/A
//
// Sends:           [Chip Select Low]
//                  00000000         Dummy 0x00
//                  00000000         Dummy 0x00
//                  00000000         Dummy 0x00
//                  [Chip Select High]
//
// Returns:         N/A

void FT800_SPI_HostCommandDummyRead(void)
 {
    byte SPI_Writebyte = 0x00;

    // Chip Select Low
    HAL_SPI_CSLow();

    // Read/Write
    SPI_Writebyte = 0x00;                                   // Sending dummy 00 byte
    HAL_SPI_ReadWrite(SPI_Writebyte);


    SPI_Writebyte = 0x00;                                   // Sending dummy 00 byte
    HAL_SPI_ReadWrite(SPI_Writebyte);


    SPI_Writebyte = 0x00;                                   // Sending dummy 00 byte
    HAL_SPI_ReadWrite(SPI_Writebyte);


    // Chip Select High
    HAL_SPI_CSHigh();
}

// ============================================================================================================
// Increment Command Offset
// ============================================================================================================

// This function is used when adding commands to the Command FIFO.
// After adding a command to the FIFO, this function will calculate the FIFO address offset where the next command
// should be written to.
// In general, the next command would be written to [previous offset plus the length of the last command] but due
// to the circular FIFO used, a command list may need to wrap round from an offset of 4095 to 0.
//
// Pass in:         N/A
//
// Sends:           [Chip Select Low]
//                  00000000         Dummy 0x00
//                  00000000         Dummy 0x00
//                  00000000         Dummy 0x00
//                  [Chip Select High]
//
// Returns:         N/A


unsigned int FT800_IncCMDOffset(unsigned int Current_Offset, byte Command_Size)
{
    unsigned int New_Offset;

    New_Offset = Current_Offset + Command_Size;

    if(New_Offset > 4095)
    {
        New_Offset = (New_Offset - 4096);
    }

    return New_Offset;
}

// ############################################################################################################
// ############################################################################################################

void ComEsperaFin(void){
do
{
    HAL_SPI_CSLow();                            //
    FT800_SPI_SendAddressRD(REG_CMD_WRITE);     //
    cmdBufferWr = FT800_SPI_Read32();           // Read the value of the REG_CMD_WRITE register
    HAL_SPI_CSHigh();                           //
    HAL_SPI_CSLow();                            //
    FT800_SPI_SendAddressRD(REG_CMD_READ);      //
    cmdBufferRd = FT800_SPI_Read32();           // Read the value of the REG_CMD_READ register
    HAL_SPI_CSHigh();                           //
} while(cmdBufferWr != cmdBufferRd);


// Check the actual value of the current WRITE register (pointer)

CMD_Offset = cmdBufferWr;
}


void EscribeRam32(unsigned long dato){
        HAL_SPI_CSLow();
        FT800_SPI_SendAddressWR(RAM_CMD + CMD_Offset);  // Writing to next available location in FIFO (FIFO base address + offset)
        FT800_SPI_Write32(dato);                  // Vertex 2F    01XXXXXX XXXXXXXX XYYYYYYY YYYYYYYY
        HAL_SPI_CSHigh();

        CMD_Offset = FT800_IncCMDOffset(CMD_Offset, 4); // Move the CMD Offset since we have just added 4 bytes to the FIFO
}

void EscribeRam16(unsigned int dato){
        HAL_SPI_CSLow();
        FT800_SPI_SendAddressWR(RAM_CMD + CMD_Offset);  // Writing to next available location in FIFO (FIFO base address + offset)
        FT800_SPI_Write16(dato);                  // Vertex 2F    01XXXXXX XXXXXXXX XYYYYYYY YYYYYYYY
        HAL_SPI_CSHigh();

        CMD_Offset = FT800_IncCMDOffset(CMD_Offset, 2); // Move the CMD Offset since we have just added 4 bytes to the FIFO
}

void EscribeRam8(char dato){
        HAL_SPI_CSLow();
        FT800_SPI_SendAddressWR(RAM_CMD + CMD_Offset);  // Writing to next available location in FIFO (FIFO base address + offset)
        FT800_SPI_Write8(dato);                  // Vertex 2F    01XXXXXX XXXXXXXX XYYYYYYY YYYYYYYY
        HAL_SPI_CSHigh();

        CMD_Offset = FT800_IncCMDOffset(CMD_Offset, 1); // Move the CMD Offset since we have just added 4 bytes to the FIFO
}



void EscribeRamTxt(char *cadena)
{
    while(*cadena) EscribeRam8(*cadena++);
    EscribeRam8(0);
}

void Comando(unsigned long COMM){
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(RAM_CMD + CMD_Offset);  // Writing to next available location in FIFO (FIFO base address + offset)
FT800_SPI_Write32(COMM);                  // Write the DL_START command
HAL_SPI_CSHigh();                               //

CMD_Offset = FT800_IncCMDOffset(CMD_Offset, 4); // Move the CMD Offset since we have just added 4 bytes to the FIFO

}

void ComVertex2ff(int x,int y){
long coord;
#ifdef VM800B35
#ifdef ESCALADO
    x=(x*2)/3;
    y=(y*15)/17;
#endif
#endif

        coord=x;
        coord=coord<<19;
        coord=coord+ (y<<4);
        HAL_SPI_CSLow();                                //
        FT800_SPI_SendAddressWR(RAM_CMD + CMD_Offset);  // Writing to next available location in FIFO (FIFO base address + offset)
        FT800_SPI_Write32(0x40000000+coord);                  // Vertex 2F    01XXXXXX XXXXXXXX XYYYYYYY YYYYYYYY
        HAL_SPI_CSHigh();                               //

        CMD_Offset = FT800_IncCMDOffset(CMD_Offset, 4); // Move the CMD Offset since we have just added 4 bytes to the FIFO
}

void ComColor(int R, int G, int B){
    long color;
        color=R;
        color=color<<8;
        color+=G;
        color=color<<8;
        color+= B;

    // Color RGB   00000100 BBBBBBBB GGGGGGGG RRRRRRRR  (B/G/R = Colour values)
    Comando(0x04000000+color);
}

void ComFgcolor(int R, int G, int B){
    long color;
        color=R;
        color=color<<8;
        color+=G;
        color=color<<8;
        color+= B;

    EscribeRam32 (CMD_FGCOLOR);
    EscribeRam32 (color);

}


void ComBgcolor(int R, int G, int B){
    long color;
        color=R;
        color=color<<8;
        color+=G;
        color=color<<8;
        color+= B;
    EscribeRam32 (CMD_BGCOLOR);
    EscribeRam32 (color);

}

void ComTXT(int x, int y, int fuente, int ops, char *cadena)
{
#ifdef VM800B35
#ifdef ESCALADO

    x=(x*2)/3;
    y=(y*15)/17;
#endif
#endif

            EscribeRam32(CMD_TEXT);
            EscribeRam16(x);
            EscribeRam16(y);
            EscribeRam16(fuente);
            EscribeRam16(ops);
            while(*cadena) EscribeRam8(*cadena++);
            EscribeRam8(0);
            PadFIFO();


}

void ComNum(int x, int y, int fuente, int ops, unsigned long Num)
{
#ifdef VM800B35
#ifdef ESCALADO

    x=(x*2)/3;
    y=(y*15)/17;
#endif
#endif

            EscribeRam32(CMD_NUMBER);
            EscribeRam16(x);
            EscribeRam16(y);
            EscribeRam16(fuente);
            EscribeRam16(ops);
            EscribeRam32(Num);
            PadFIFO();
}
void ComTeclas(int x, int y, int w, int h, int fuente, unsigned int ops, char *Keys)
{
#ifdef VM800B35
#ifdef ESCALADO

    x=(x*2)/3;
    y=(y*15)/17;
#endif
#endif

    EscribeRam32(CMD_KEYS);
    EscribeRam16(x);
    EscribeRam16(y);
    EscribeRam16(w);
    EscribeRam16(h);
    EscribeRam16(fuente);
    EscribeRam16(ops);
    while(*Keys) EscribeRam8(*Keys++);
    EscribeRam8(0);
PadFIFO();

}

void Ejecuta_Lista(void){
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(REG_CMD_WRITE);         //
FT800_SPI_Write16(CMD_Offset);                  //
HAL_SPI_CSHigh();                               //
}

void PadFIFO(void){
    int i, pad;
    pad=CMD_Offset&0x0003;
    if(pad>0){
        for(i=0;i<4-pad;i++) EscribeRam8(0);
    }
}



void Dibuja(void){
            Comando(CMD_DISPLAY);
            Comando(CMD_SWAP);
            Ejecuta_Lista();
}



void Inicia_pantalla(void){
    char i;
HAL_SPI_PDhigh();                               // Put the Power Down # pin high to wake FT800

Delay();                                        // Delay for power up of regulator

FT800_SPI_HostCommandDummyRead();               // Read location 0 to wake up FT800

FT800_SPI_HostCommand(FT_GPU_EXTERNAL_OSC);     // Change the PLL to external clock - optional

FT800_SPI_HostCommand(FT_GPU_PLL_48M);          //  Ensure configured to 48 MHz

// Note: Could now increase SPI clock rate up to 30MHz SPI if preferred

Delay();                                        // Delay

FT800_SPI_HostCommand(FT_GPU_CORE_RESET);       // Reset the core

Delay();                                        // Delay

FT800_SPI_HostCommand(FT_GPU_ACTIVE_M);         // Read address 0 to ensure FT800 is active

chipid = 0x00;                                  // Read the Chip ID to check comms with the FT800 - should be 0x7C
    while(chipid != 0x7C)
{
    HAL_SPI_CSLow();                            // CS low
    FT800_SPI_SendAddressRD(REG_ID);            // Send the address
    chipid = FT800_SPI_Read8();                 // Read the actual value
    HAL_SPI_CSHigh();
    Delay();// CS high
}


// =======================================================================
// Write the display registers on the FT800 for your particular display
// =======================================================================

    HAL_SPI_CSLow();                                // CS low                   Write REG_HCYCLE to 548
    FT800_SPI_SendAddressWR(REG_HCYCLE);            // Send the address
    FT800_SPI_Write16(HCYCLE);                         // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_HOFFSET to 43
    FT800_SPI_SendAddressWR(REG_HOFFSET);           // Send the address
    FT800_SPI_Write16(HOFFSET);                          // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_HSYNC0 to 0
    FT800_SPI_SendAddressWR(REG_HSYNC0);            // Send the address
    FT800_SPI_Write16(HSYNC0);                           // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_HSYNC1 to 41
    FT800_SPI_SendAddressWR(REG_HSYNC1);            // Send the address
    FT800_SPI_Write16(HSYNC1);                          // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_VCYCLE to 292
    FT800_SPI_SendAddressWR(REG_VCYCLE);            // Send the address
    FT800_SPI_Write16(VCYCLE);                         // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_VOFFSET to 12
    FT800_SPI_SendAddressWR(REG_VOFFSET);           // Send the address
    FT800_SPI_Write16(VOFFSET);                          // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_VSYNC0 to 0
    FT800_SPI_SendAddressWR(REG_VSYNC0);            // Send the address
    FT800_SPI_Write16(VSYNC0);                           // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_VSYNC1 to 10
    FT800_SPI_SendAddressWR(REG_VSYNC1);            // Send the address
    FT800_SPI_Write16(VSYNC1);                          // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_SWIZZLE to 2
    FT800_SPI_SendAddressWR(REG_SWIZZLE);           // Send the address
#ifdef VM800B50
    FT800_SPI_Write16(0);                           // Send the 16-bit value
#endif
#ifdef VM800B35
    FT800_SPI_Write16(2);                           // Send the 16-bit value
#endif


    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_PCLK_POL to 1
    FT800_SPI_SendAddressWR(REG_PCLK_POL);          // Send the address
    FT800_SPI_Write16(PCLK_POL);                           // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_HSIZE to 480
    FT800_SPI_SendAddressWR(REG_HSIZE);             // Send the address
    FT800_SPI_Write16(HSIZE);                         // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_VSIZE to 272
    FT800_SPI_SendAddressWR(REG_VSIZE);             // Send the address
    FT800_SPI_Write16(VSIZE);                         // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high

    HAL_SPI_CSLow();                                // CS low                   Write REG_PCLK to 5
    FT800_SPI_SendAddressWR(REG_PCLK);              // Send the address
    FT800_SPI_Write16(PCLK);                           // Send the 16-bit value
    HAL_SPI_CSHigh();                               // CS high


// =======================================================================
// Configure the touch screen
// =======================================================================

HAL_SPI_CSLow();                                // CS low                   Write the touch threshold
FT800_SPI_SendAddressWR(REG_TOUCH_RZTHRESH);    // Send the address
FT800_SPI_Write16(1200);                        // Send the 16-bit value
HAL_SPI_CSHigh();                               // CS high


// =======================================================================
// Calibrate the Touchscreen with default values (depending on model)
// =======================================================================


#ifdef VM800B35
for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
#endif
#ifdef VM800B50
for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL5[i]);
#endif

// =======================================================================
// Send an initial display list which will clear the screen to a black colour
// =======================================================================

// Set the colour which is used when the colour buffer is cleared
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(RAM_DL + 0);            // Write first entry in Display List (first location in DL Ram)
FT800_SPI_Write32(0x02000000);                  // Clear Color RGB   00000010 BBBBBBBB GGGGGGGG RRRRRRRR  (B/G/R = Colour values)
HAL_SPI_CSHigh();                               //

// Clear the Colour, Stencil and Tag buffers. This will set the screen to the 'clear' colour set above.
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(RAM_DL + 4);            // Write next entry in display list (each command is 4 bytes)
FT800_SPI_Write32(0x26000007);                  // Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear)
HAL_SPI_CSHigh();                               //

// Display command ends the display list
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(RAM_DL + 8);            // Write next entry in display list
FT800_SPI_Write32(0x00000000);                  // DISPLAY command 00000000 00000000 00000000 00000000
HAL_SPI_CSHigh();                               //

// Writing to the DL_SWAP register tells the Display Engine to render the new screen designed above.
HAL_SPI_CSLow();                                //
FT800_SPI_SendAddressWR(REG_DLSWAP);            // Writing to the DL_SWAP register...value 10 means render after last frame complete
FT800_SPI_Write32(0x00000002);                  // 00000000 00000000 00000000 000000SS  (SS bits define when render occurs)
HAL_SPI_CSHigh();                               // CS high



// =======================================================================
// Set the GPIO pins of the FT800 to enable the display now
// =======================================================================

// note: Refer to the GPIO section in the FT800 datasheet and also the connections to the GPIO
// used on the particular development module being used

// Bit 7 enables the LCD Display. It is set to output driving high
// Bit 1 enables the Audio Amplifier. It is set to output driving low to shut down the amplifier since audio not used here
// Bit 0 is unused but set to output driving high.

HAL_SPI_CSLow();                                // CS low                   Write REG_GPIO_DIR
FT800_SPI_SendAddressWR(REG_GPIO_DIR);          // Send the address
FT800_SPI_Write8(0x83);                         // Send the 8-bit value
HAL_SPI_CSHigh();                               // CS high

HAL_SPI_CSLow();                                // CS low                   Write REG_GPIO
FT800_SPI_SendAddressWR(REG_GPIO);              // Send the address
FT800_SPI_Write8(0x81);                         // Send the 8-bit value
HAL_SPI_CSHigh();                               // CS high

}



void Nueva_pantalla(int R, int G, int B)
{
    long color;
    color=R;
    color=color<<8;
    color+=G;
    color=color<<8;
    color+= B;
    ComEsperaFin();
    Comando(CMD_DLSTART);
    Comando(0x02000000+color);  // Clear Color RGB   00000010 BBBBBBBB GGGGGGGG RRRRRRRR  (B/G/R = Colour values)
    Comando(0x26000007);                            // Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear))
}


void Lee_pantalla(void){
            HAL_SPI_CSLow();                                //
            FT800_SPI_SendAddressRD(REG_TOUCH_SCREEN_XY);   //
            BufferXY = FT800_SPI_Read32();           // Lee valor pantalla tactil
            HAL_SPI_CSHigh();
            POSX=(BufferXY&0xffff0000)>>16;
            POSY=BufferXY&0x0000FFFF;
#ifdef VM800B35
#ifdef ESCALADO
    if(POSX!=0x8000) POSX=(POSX*3)/2;
    if(POSY!=0x8000)  POSY=(POSY*17)/15;
#endif
#endif

}

unsigned long Lee_Reg(unsigned long dir){
    unsigned long Buff_temp;
        HAL_SPI_CSLow();                        //
        FT800_SPI_SendAddressRD(dir);   //
        Buff_temp = FT800_SPI_Read32();           // Lee valor
        HAL_SPI_CSHigh();
return(Buff_temp);
}

void Esc_Reg(unsigned long dir, unsigned long valor){
    HAL_SPI_CSLow();
        FT800_SPI_SendAddressWR(dir);
        FT800_SPI_Write32(valor);
        HAL_SPI_CSHigh();
}



void ComLineWidth(int width)
{
    Comando(CMD_LINEWIDTH+16*width);
}

void ComPointSize(int size)
{
    Comando(CMD_POINTSIZE+size*16);
}


void ComScrollbar(int x, int y, int w, int h, int ops, int val, int size, int range)
{
#ifdef VM800B35
#ifdef ESCALADO

    x=(x*2)/3;
    y=(y*15)/17;
#endif
#endif

        EscribeRam32(CMD_SCROLLBAR);
        EscribeRam16(x);
        EscribeRam16(y);
        EscribeRam16(w);
        EscribeRam16(h);
        EscribeRam16(ops);
        EscribeRam16(val);
        EscribeRam16(size);
        EscribeRam16(range);

}



void ComButton(int x, int y, int w, int h, int font, int ops, char *cadena)
{
#ifdef VM800B35
#ifdef ESCALADO

    x=(x*2)/3;
    y=(y*15)/17;
    w=(w*2)/3;
    h=(h*15)/17;

#endif
#endif

        EscribeRam32(CMD_BUTTON);
        EscribeRam16(x);
        EscribeRam16(y);
        EscribeRam16(w);
        EscribeRam16(h);
        EscribeRam16(font);
        EscribeRam16(ops);
        while(*cadena) EscribeRam8(*cadena++);
        EscribeRam8(0);
        PadFIFO();

}


void Calibra_touch(void){
ComEsperaFin();
    Comando(CMD_DLSTART);
    Comando(0x02f7f7f7);  // Clear Color RGB   00000010 BBBBBBBB GGGGGGGG RRRRRRRR  (B/G/R = Colour values)
    Comando(0x26000007);                            // Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear))
    ComColor(0x00,0x00,0x00);

        ComTXT(60,30,27,0,"Pulsa en el punto");
        EscribeRam32(CMD_CALIBRATE);

        Dibuja();

        espera(500);
        ComEsperaFin();
        Comando(CMD_DLSTART);
        Comando(0x02ffff00);  // Clear Color RGB   00000010 BBBBBBBB GGGGGGGG RRRRRRRR  (B/G/R = Colour values)
        Comando(0x26000007);                            // Clear 00100110 -------- -------- -----CST  (C/S/T define which parameters to clear))
        Dibuja();
}

void ComGradient(int x0,int y0, long color0, int x1, int y1, long color1){
#ifdef VM800B35
#ifdef ESCALADO

    x0=(x0*2)/3;
    y0=(y0*15)/17;
    x1=(x1*2)/3;
        y1=(y1*15)/17;

#endif
#endif

    EscribeRam32(CMD_GRADIENT);
    EscribeRam16(x0);
    EscribeRam16(y0);
    EscribeRam32(color0);
    EscribeRam16(x1);
    EscribeRam16(y1);
    EscribeRam32(color1);

}

void Espera_pant(void)
{


    do{
        Lee_pantalla();

    }while(POSY==0x8000);
    Delay();
    do{
            Lee_pantalla();

        }while(POSY!=0x8000);
}

void VolNota(unsigned char volumen){
    HAL_SPI_CSLow();
                FT800_SPI_SendAddressWR(REG_VOL_SOUND);
                FT800_SPI_Write8(volumen);
                HAL_SPI_CSHigh();

}
void TocaNota( int instr, int nota)
{
    HAL_SPI_CSLow();
            FT800_SPI_SendAddressWR(REG_SOUND);
            FT800_SPI_Write16(instr+(nota<<8));
            HAL_SPI_CSHigh();

            HAL_SPI_CSLow();
                        FT800_SPI_SendAddressWR(REG_PLAY);
                        FT800_SPI_Write8(1);
                        HAL_SPI_CSHigh();

    }

void FinNota(void)
{
    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(REG_SOUND);
    FT800_SPI_Write16(0x0000);
    HAL_SPI_CSHigh();

    HAL_SPI_CSLow();
    FT800_SPI_SendAddressWR(REG_PLAY);
    FT800_SPI_Write8(1);
    HAL_SPI_CSHigh();



}


void Fadeout(void)
{
   int i;

    for (i = 100; i >= 0; i -= 3)
    {
        HAL_SPI_CSLow();
                    FT800_SPI_SendAddressWR(REG_PWM_DUTY);
                FT800_SPI_Write8(i);
                HAL_SPI_CSHigh();

        //Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
        espera(2);

    }
}

/* API to perform display fadein effect by changing the display PWM from 0 till 100 and finally 128 */
void Fadein()
{
    int i;

    for (i = 0; i <=100 ; i += 3)
    {
         HAL_SPI_CSLow();
                        FT800_SPI_SendAddressWR(REG_PWM_DUTY);
                        FT800_SPI_Write8(i);
                        HAL_SPI_CSHigh();
                        espera(2);
    }
    i = 128;
     HAL_SPI_CSLow();
                        FT800_SPI_SendAddressWR(REG_PWM_DUTY);
                        FT800_SPI_Write8(i);
                        HAL_SPI_CSHigh();

}

/************************************************
 *
 * Funciones de más alto nivel, para manejo de macros en pantalla
 *
 * void ComRect(x1,y1,x2,y2,RRGGBB)
 *
 * char Boton(int x, int y, int w, int h, int font, char *cadena)
 */

void ComRect(int x1, int y1, int x2, int y2, char relleno)
{

    if(relleno==1){
    Comando(CMD_LINEWIDTH+80);
        Comando(CMD_BEGIN_RECTS);
        ComVertex2ff(x1,y1);
        ComVertex2ff(x2,y2);
       Comando(CMD_END);
    }
    else{
        Comando(CMD_BEGIN_LINES);
           Comando(CMD_LINEWIDTH+80);
           ComVertex2ff(x1,y1);
           ComVertex2ff(x2,y1);
           ComVertex2ff(x2,y1);
           ComVertex2ff(x2,y2);
           ComVertex2ff(x2,y2);
           ComVertex2ff(x1,y2);
           ComVertex2ff(x1,y2);
           ComVertex2ff(x1,y1);

           Comando(CMD_END);

    }
}

void ComCirculo(int x, int y, int r)
{
    Comando(CMD_POINTSIZE+r*16);// Point Size  00001101 00000000 SSSSSSSS SSSSSSSS  (S = Size value)
        Comando(CMD_BEGIN_POINTS);
        ComVertex2ff(x,y);
        Comando(CMD_END);
}
char Boton(int x, int y, int w, int h, int font, char *cadena)
{
    char resul;
    Lee_pantalla();
    if(POSX>x && POSX<(x+w) && POSY>y && POSY<(y+h)){
        ComButton(x,y,w,h,font,OPT_FLAT,cadena);
        resul=1;
    }else{
        ComButton(x,y,w,h,font,0,cadena);
              resul=0;
    }
    return resul;
}


