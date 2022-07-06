#include "mbed.h"
#include <EthernetInterface.h>

#define TIMEOUT 100000000
#define OFFSET  0xd000

FlashIAP flash;

void writeIAP(int count, char* buffer){
    const static uint32_t page_size = flash.get_page_size();
    //Program the buffer into the flash memory
    flash.erase(count + OFFSET, flash.get_sector_size(count + OFFSET));
    flash.program(buffer, count + OFFSET, page_size * 2);
}

void bootloader(void)
{
    int remaining;
    int rcount;
    char *p;
    nsapi_size_or_error_t result;
    EthernetInterface interface;
    interface.connect();
    
    // Show the network address
    const char *ip = interface.get_ip_address();
    printf("IP address is: %s\n", ip ? ip : "No IP");
    printf("Bootloader\r\n");
    
    // Open a socket on the network interface, and create a TCP connection to ifconfig.io
    TCPSocket socket;
    
    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\nHost: ifconfig.io\r\nConnection: close\r\n\r\n";
    nsapi_size_t size = strlen(sbuffer);
    result = socket.open(&interface);
    if (result != 0) {
        printf("Error! socket.open() returned: %d\n", result);
        while(true);
    }
    result = socket.connect("ifconfig.io", 80);
    if (result != 0) {
        printf("Error! socket.connect() returned: %d\n", result);
        while(true);
    }
    
    // Loop until whole request sent
    while(size) {
        result = socket.send(sbuffer+result, size);
        if (result < 0) {
            printf("Error! socket.send() returned: %d\n", result);
            while(true);
        }
        size -= result;
        printf("sent %d [%.*s]\n", result, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);
    }

    // Receieve an HTTP response and print out the response line
    remaining = 256;
    rcount = 0;
    
    flash.init();
    const static uint32_t page_size = flash.get_page_size();
    char * buffer = new char[page_size * 2];
    
    p = buffer;
    while (remaining > 0 && 0 < (result = socket.recv(p, remaining))) {
        p += result;
        rcount += result;
        remaining -= result;
    }
    if (result < 0) {
        printf("Error! socket.recv() returned: %d\n", result);
        while(true);
    }
    
    // the HTTP response code
    printf("recv %d [%.*s]\n", rcount, strstr(buffer, "\r\n") - buffer, buffer);
    
    uint32_t count = 0;
    uint32_t buffercount = 0;
    uint32_t timeout = 0;
    
    //Data receive loop
    while(true) {
        //Check if there is new data
        if (UART0->S1 & UART_S1_RDRF_MASK) {
            //Place data in buffer
            buffer[buffercount] = UART0->D;
            buffercount++;
            
            //Reset timeout
            timeout = 0;

            //We write per BUFFER_SIZE chars
            if (buffercount == page_size * 2) {
                writeIAP(count, buffer);
                buffercount = 0;
                count += page_size * 2;
            }
        } else {
            //No new data, increase timeout
            timeout++;
            
            //We have received no new data for a while, assume we are done
            if (timeout > TIMEOUT) {
                //If there is data left in the buffer, program it
                if (buffercount != 0) {
                    for (int i = buffercount; i < page_size * 2; i++) {
                        buffer[i] = 0xff;
                    }
                    writeIAP(count, buffer);
                }
                break;          //We should be done programming :D
            }
        }
    }

    delete[] buffer;

    flash.deinit();
    
    // Close the socket to return its memory and bring down the network interface
    socket.close();

    // Bring down the ethernet interface
    interface.disconnect();
    printf("Done\n");
}
