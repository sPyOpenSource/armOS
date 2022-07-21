#include "mbed.h"

//Could be nicer, but for now just erase all preceding sectors
#define NUM_SECTORS 15
#define TIMEOUT     10000000
#define BUFFER_SIZE 16

FlashIAP flash;

void writeIAP(int count, char* buffer){
    const static uint32_t page_size = flash.get_page_size();
    //Program the buffer into the flash memory
    flash.erase(count, flash.get_sector_size(count));
    flash.program(buffer, count, page_size * 2);
}

void bootloader(void){
    printf("Bootloader\r\n");
    printf("Continue? (y/n)");
    
    //Wait until data arrived, if it is 'y', continue
    while(!(UART0->S1 & UART_S1_RDRF_MASK));
    if (UART0->D != 'y')
        return;
    
    //Disable IRQs, enable ones have been removed from FreescaleIAP
    __disable_irq();
    
    //Erase all sectors we use for the user program
    printf("Erasing sectors!\r\n");
    for (int i = 0; i < NUM_SECTORS; i++){
        flash.erase(flash.get_sector_size(0) * i, flash.get_sector_size(0));
    }

    printf("Done erasing, send file!\r\n");
    
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
                if (flash.program(buffer, count, BUFFER_SIZE) != 0) {
                    printf("Error!\r\n");   
                    break;
                }
                
                //Reset buffercount for next buffer
                printf("#");
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
                    flash.program(buffer, count, BUFFER_SIZE);
                }
                break; //We should be done programming :D
            }
        }
    }
    printf("Done programming!\r\n");
    NVIC_SystemReset();
}
