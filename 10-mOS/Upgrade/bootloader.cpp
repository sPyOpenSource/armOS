#include "mbed.h"

//Could be nicer, but for now just erase all preceding sectors
#define NUM_SECTORS 15
#define TIMEOUT     10000000
#define BUFFER_SIZE 16

void setupserial();
void write(char *value);

__attribute__((section(".ARM.__at_0x10000"))) void bootloader(void){
    setupserial();
    write("Bootloader\r\n");
    write("Continue? (y/n)");
    
    //Wait until data arrived, if it is 'y', continue
    while(!(UART0->S1 & UART_S1_RDRF_MASK));
    if (UART0->D != 'y')
        return;
    
    //Disable IRQs, enable ones have been removed from FreescaleIAP
    __disable_irq();
    
    //Erase all sectors we use for the user program
    write("Erasing sectors!\r\n");
    for (int i = 0; i < NUM_SECTORS; i++)
        erase_sector(SECTOR_SIZE * i);

    write("Done erasing, send file!\r\n");
    
    char buffer[BUFFER_SIZE];
    uint32_t count = 0;
    uint8_t buffercount = 0;
    uint32_t timeout = 0;
    
    //Wait until data is sent
    while(!(UART0->S1 & UART_S1_RDRF_MASK));
    
    //Data receive loop
    while(1) {
        //Check if there is new data
        if (UART0->S1 & UART_S1_RDRF_MASK) {
            //Place data in buffer
            buffer[buffercount] = UART0->D;
            buffercount++;
            
            //Reset timeout
            timeout = 0;

            //We write per BUFFER_SIZE chars
            if (buffercount == BUFFER_SIZE) {
                //NMI Handler is at bytes 8-9-10-11, we overwrite this to point to bootloader function
                if (count == 0) {
                    buffer[8] = 0x01;
                    buffer[9] = 0x00;
                    buffer[10] = 0x01;
                    buffer[11] = 0x00;
                }
                
                //Program the buffer into the flash memory
                if (program_flash(count, buffer, BUFFER_SIZE) != 0) {
                    write("Error!\r\n");   
                    break;
                }
                
                //Reset buffercount for next buffer
                write("#");
                buffercount = 0;
                count += BUFFER_SIZE;
            }
        } else {
            //No new data, increase timeout
            timeout++;
            
            //We have received no new data for a while, assume we are done
            if (timeout > TIMEOUT) {
                //If there is data left in the buffer, program it
                if (buffercount != 0) {
                    for (int i = buffercount; i < BUFFER_SIZE; i++) {
                        buffer[i] = 0xFF;
                    }
                    program_flash(count, buffer, BUFFER_SIZE);
                }
                break; //We should be done programming :D
            }
        }
    }
    write("Done programming!\r\n");
    NVIC_SystemReset();
}

__attribute__((section(".ARM.__at_0x10080"))) static void setupserial(void) {
        //Setup USBTX/USBRX pins (PTB16/PTB17)
        SIM->SCGC5 |= 1 << SIM_SCGC5_PORTB_SHIFT;
        PORTB->PCR[16] = (PORTB->PCR[16] & 0x700) | (3 << 8);
        PORTB->PCR[17] = (PORTB->PCR[17] & 0x700) | (3 << 8);

        //Setup UART (ugly, copied resulting values from mbed serial setup)
        SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;

        UART0->BDH = 3;
        UART0->BDL = 13;
        UART0->C4 = 8;
        UART0->C2 = 12; //Enables UART
    }

__attribute__((section(".ARM.__at_0x100A0"))) static void write(char *value){
        int i = 0;
        //Loop through string and send everything
        while(*(value + i) != '\0') {
            while(!(UART0->S1 & UART_S1_TDRE_MASK));
            UART0->D = *(value + i);
            i++;
        }
    }
