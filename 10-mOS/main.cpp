
#include <EthernetInterface.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include "UnitTest.h"
#include "mDNSResponder.h"

#include <string>
#include "UnitTest.h"

#define MAXDATASIZE (1024 * 4)

//#define GRAPHICSMODE

const int PORT = 8000;
static const char* SERVER_IP = "172.26.10.207"; //IP of server board
char screen[3840];

DigitalOut red(LED_RED);
DigitalOut blue(LED_BLUE);
DigitalOut green(LED_GREEN);
FlashIAP   flash;
Serial     pc(USBTX, USBRX);

class PrintfTCPHandler
{
public:
    bool HandleTransmissionControlProtocolMessage(TCPSocket* socket, char* data, uint16_t size){
        if(size > 9
            && data[0] == 'G'
            && data[1] == 'E'
            && data[2] == 'T'
            && data[3] == ' '
            && data[4] == '/'
            && data[5] == ' '
            && data[6] == 'H'
            && data[7] == 'T'
            && data[8] == 'T'
            && data[9] == 'P'
        ){
            unsigned int address = flash.get_flash_size() - 4096; //Write in last sector
            char *data = (char*)address;                          //Read
            socket->send(
                "HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n", 184
            );
            socket->close();
        } else {
            socket->close();
        }

        return true;
    }
};

void write(unsigned int n, unsigned int addressOfX, char X){
    unsigned int address = n * 4096;            //Write in n sector     
    char *data   = (char*)address;              //Read     
    char b[4096];
    b[addressOfX] = X;
    memcpy(b, data, 4096 * sizeof(char));       //int is a POD                           
    flash.erase(address, flash.get_sector_size(address));                
    flash.program(b, address, 4096);
}

void write(char *data, unsigned int size){
    unsigned int address = flash.get_flash_size() - 4096; //Write in last sector                     
    flash.erase(address, flash.get_sector_size(address));                
    flash.program(data, address, size);
}

char read(unsigned int address){
    char *data = (char*)address; //Read
    return data[0];
}

EthernetInterface interface;

void http() {
    PrintfTCPHandler tcphandler;
    TCPSocket server;
    server.open(&interface);
    server.bind(80);
    server.listen(1);
    TCPSocket *client;
    int bufferSize = 512;
    char buffer[bufferSize];

    while (true) {
        client = server.accept();
        int len = client->recv(&buffer, bufferSize);

#ifdef DEBUG
      printf("%d: ", len);
      for (int i = 0; i < len; i++)
          printf("%d ", buffer[i]);
      printf("\r\n");
#endif

        tcphandler.HandleTransmissionControlProtocolMessage(client, (char*)buffer, len);
        wait(0.03333333);
    }
}

void mDNS() {
    mDNSResponder mdns(interface);
    mdns.announce(interface.get_ip_address());
    while (true) {
        mdns.MDNS_process();
        wait(0.03333333);
    }
}

volatile char c = '\0'; // Initialized to the NULL character

void onCharReceived(){
    //c = pc.getc();
    pc.putc(c);
}

int main() {
    //pc.attach(&onCharReceived);
    pc.baud(115200);
    red = 1;
    blue = 0;
    green = 1;
    
    UnitTest unitest;
    
    unitest.assertOn(red, 1);
    
    pc.printf("Hello World! --- http://www.AlgorithMan.de\n\r");
    
    //write("HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n", 184);
    
    Thread task1;
    Thread task2;
    Thread task3;

    //InterruptManager interrupts(0x20, &gdt, &taskManager);

    pc.printf("Initializing Hardware, Stage 1\n\r");
    
    #ifdef GRAPHICSMODE
        myos::gui::Desktop desktop(320, 200, 0x00, 0x00, 0xA8);
    #endif

        #ifdef GRAPHICSMODE
            //KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            //PrintfKeyboardEventHandler kbhandler;
            //KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif


        #ifdef GRAPHICSMODE
            //MouseDriver mouse(&interrupts, &desktop);
        #else
            //MouseToConsole mousehandler;
            //MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        
        #ifdef GRAPHICSMODE
            myos::drivers::VideoGraphicsArray vga;
        #endif
        
    pc.printf("Initializing Hardware, Stage 2\n\r");

    #ifdef GRAPHICSMODE
        vga.SetMode(320, 200, 8);
        /*Window win1(&desktop, 10, 10, 20, 20, 0xA8, 0x00, 0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
        desktop.AddChild(&win2);*/
    #endif

    interface.connect();

    // Show the network address
    const char *ip = interface.get_ip_address();
    pc.printf("IP address is: %s\n\r", ip ? ip : "No IP");

    //interrupts.Activate();

    //UDPSocket sockUDP;
    //sockUDP.open(&interface);
    
    //TransmissionControlProtocolSocket* tcpsocket = tcp.Connect(ip2_be, 1234);
    //tcpsocket->Send((uint8_t*)"Hello TCP!", 10);

    //PrintfUDPHandler udphandler;

    //UserDatagramProtocolSocket* udpsocket = udp.Listen(1234);
    //udp.Bind(udpsocket, &udphandler);
    task1.start(mDNS);
    task2.start(http);
    task3.start(ssh);
    
    string input;
    while (true){
        //sockUDP.sendto(SERVER_IP, PORT, screen, sizeof(screen));
        char c = pc.getc();
        if (c == ';'){
            pc.printf("%s\n\r", input.c_str());
            input.clear();
        } else {
            input += c;
        }
        wait(0.03333333);
    }
}

/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include <wolf/ssl/headers/options.h>
#include <wolf/crypt/headers/sha256.h>
#include <wolf/crypt/headers/coding.h>
#include <wolf/ssh/headers/ssh.h>
#include <wolf/ssh/headers/test.h>
#include <wolf/ssh/headers/misc.h>

unsigned char server_key_rsa_der[] = {
  0x30, 0x82, 0x04, 0xa3, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00,
  0xda, 0x5d, 0xad, 0x25, 0x14, 0x76, 0x15, 0x59, 0xf3, 0x40, 0xfd, 0x3c,
  0xb8, 0x62, 0x30, 0xb3, 0x6d, 0xc0, 0xf9, 0xec, 0xec, 0x8b, 0x83, 0x1e,
  0x9e, 0x42, 0x9c, 0xca, 0x41, 0x6a, 0xd3, 0x8a, 0xe1, 0x52, 0x34, 0xe0,
  0x0d, 0x13, 0x62, 0x7e, 0xd4, 0x0f, 0xae, 0x5c, 0x4d, 0x04, 0xf1, 0x8d,
  0xfa, 0xc5, 0xad, 0x77, 0xaa, 0x5a, 0x05, 0xca, 0xef, 0xf8, 0x8d, 0xab,
  0xff, 0x8a, 0x29, 0x09, 0x4c, 0x04, 0xc2, 0xf5, 0x19, 0xcb, 0xed, 0x1f,
  0xb1, 0xb4, 0x29, 0xd3, 0xc3, 0x6c, 0xa9, 0x23, 0xdf, 0xa3, 0xa0, 0xe5,
  0x08, 0xde, 0xad, 0x8c, 0x71, 0xf9, 0x34, 0x88, 0x6c, 0xed, 0x3b, 0xf0,
  0x6f, 0xa5, 0x0f, 0xac, 0x59, 0xff, 0x6b, 0x33, 0xf1, 0x70, 0xfb, 0x8c,
  0xa4, 0xb3, 0x45, 0x22, 0x8d, 0x9d, 0x77, 0x7a, 0xe5, 0x29, 0x5f, 0x84,
  0x14, 0xd9, 0x99, 0xea, 0xea, 0xce, 0x2d, 0x51, 0xf3, 0xe3, 0x58, 0xfa,
  0x5b, 0x02, 0x0f, 0xc9, 0xb5, 0x2a, 0xbc, 0xb2, 0x5e, 0xd3, 0xc2, 0x30,
  0xbb, 0x3c, 0xb1, 0xc3, 0xef, 0x58, 0xf3, 0x50, 0x94, 0x28, 0x8b, 0xc4,
  0x65, 0x4a, 0xf7, 0x00, 0xd9, 0x97, 0xd9, 0x6b, 0x4d, 0x8d, 0x95, 0xa1,
  0x8a, 0x62, 0x06, 0xb4, 0x50, 0x11, 0x22, 0x83, 0xb4, 0xea, 0x2a, 0xe7,
  0xd0, 0xa8, 0x20, 0x47, 0x4f, 0xff, 0x46, 0xae, 0xc5, 0x13, 0xe1, 0x38,
  0x8b, 0xf8, 0x54, 0xaf, 0x3a, 0x4d, 0x2f, 0xf8, 0x1f, 0xd7, 0x84, 0x90,
  0xd8, 0x93, 0x05, 0x06, 0xc2, 0x7d, 0x90, 0xdb, 0xe3, 0x9c, 0xd0, 0xc4,
  0x65, 0x5a, 0x03, 0xad, 0x00, 0xac, 0x5a, 0xa2, 0xcd, 0xda, 0x3f, 0x89,
  0x58, 0x37, 0x53, 0xbf, 0x2b, 0x46, 0x7a, 0xac, 0x89, 0x41, 0x2b, 0x5a,
  0x2e, 0xe8, 0x76, 0xe7, 0x5e, 0xe3, 0x29, 0x85, 0xa3, 0x63, 0xea, 0xe6,
  0x86, 0x60, 0x7c, 0x2d, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x81, 0xff,
  0x0f, 0x91, 0x1e, 0x06, 0xc6, 0xae, 0xa4, 0x57, 0x05, 0x40, 0x5c, 0xcd,
  0x37, 0x57, 0xc8, 0xa1, 0x01, 0xf1, 0xff, 0xdf, 0x23, 0xfd, 0xce, 0x1b,
  0x20, 0xad, 0x1f, 0x00, 0x4c, 0x29, 0x91, 0x6b, 0x15, 0x25, 0x07, 0x1f,
  0xf1, 0xce, 0xaf, 0xf6, 0xda, 0xa7, 0x43, 0x86, 0xd0, 0xf6, 0xc9, 0x41,
  0x95, 0xdf, 0x01, 0xbe, 0xc6, 0x26, 0x24, 0xc3, 0x92, 0xd7, 0xe5, 0x41,
  0x9d, 0xb5, 0xfb, 0xb6, 0xed, 0xf4, 0x68, 0xf1, 0x90, 0x25, 0x39, 0x82,
  0x48, 0xe8, 0xcf, 0x12, 0x89, 0x9b, 0xf5, 0x72, 0xd9, 0x3e, 0x90, 0xf9,
  0xc2, 0xe8, 0x1c, 0xf7, 0x26, 0x28, 0xdd, 0xd5, 0xdb, 0xee, 0x0d, 0x97,
  0xd6, 0x5d, 0xae, 0x00, 0x5b, 0x6a, 0x19, 0xfa, 0x59, 0xfb, 0xf3, 0xf2,
  0xd2, 0xca, 0xf4, 0xe2, 0xc1, 0xb5, 0xb8, 0x0e, 0xca, 0xc7, 0x68, 0x47,
  0xc2, 0x34, 0xc1, 0x04, 0x3e, 0x38, 0xf4, 0x82, 0x01, 0x59, 0xf2, 0x8a,
  0x6e, 0xf7, 0x6b, 0x5b, 0x0a, 0xbc, 0x05, 0xa9, 0x27, 0x37, 0xb9, 0xf9,
  0x06, 0x80, 0x54, 0xe8, 0x70, 0x1a, 0xb4, 0x32, 0x93, 0x6b, 0xf5, 0x26,
  0xc7, 0x86, 0xf4, 0x58, 0x05, 0x43, 0xf9, 0x72, 0x8f, 0xec, 0x42, 0xa0,
  0x3b, 0xba, 0x35, 0x62, 0xcc, 0xec, 0xf4, 0xb3, 0x04, 0xa2, 0xeb, 0xae,
  0x3c, 0x87, 0x40, 0x8e, 0xfe, 0x8f, 0xdd, 0x14, 0xbe, 0xbd, 0x83, 0xc9,
  0xc9, 0x18, 0xca, 0x81, 0x7c, 0x06, 0xf9, 0xe3, 0x99, 0x2e, 0xec, 0x29,
  0xc5, 0x27, 0x56, 0xea, 0x1e, 0x93, 0xc6, 0xe8, 0x0c, 0x44, 0xca, 0x73,
  0x68, 0x4a, 0x7f, 0xae, 0x16, 0x25, 0x1d, 0x12, 0x25, 0x14, 0x2a, 0xec,
  0x41, 0x69, 0x25, 0xc3, 0x5d, 0xe6, 0xae, 0xe4, 0x59, 0x80, 0x1d, 0xfa,
  0xbd, 0x9f, 0x33, 0x36, 0x93, 0x9d, 0x88, 0xd6, 0x88, 0xc9, 0x5b, 0x27,
  0x7b, 0x0b, 0x61, 0x02, 0x81, 0x81, 0x00, 0xde, 0x01, 0xab, 0xfa, 0x65,
  0xd2, 0xfa, 0xd2, 0x6f, 0xfe, 0x3f, 0x57, 0x6d, 0x75, 0x7f, 0x8c, 0xe6,
  0xbd, 0xfe, 0x08, 0xbd, 0xc7, 0x13, 0x34, 0x62, 0x0e, 0x87, 0xb2, 0x7a,
  0x2c, 0xa9, 0xcd, 0xca, 0x93, 0xd8, 0x31, 0x91, 0x81, 0x2d, 0xd6, 0x68,
  0x96, 0xaa, 0x25, 0xe3, 0xb8, 0x7e, 0xa5, 0x98, 0xa8, 0xe8, 0x15, 0x3c,
  0xc0, 0xce, 0xde, 0xf5, 0xab, 0x80, 0xb1, 0xf5, 0xba, 0xaf, 0xac, 0x9c,
  0xc1, 0xb3, 0x43, 0x34, 0xae, 0x22, 0xf7, 0x18, 0x41, 0x86, 0x63, 0xa2,
  0x44, 0x8e, 0x1b, 0x41, 0x9d, 0x2d, 0x75, 0x6f, 0x0d, 0x5b, 0x10, 0x19,
  0x5d, 0x14, 0xaa, 0x80, 0x1f, 0xee, 0x02, 0x3e, 0xf8, 0xb6, 0xf6, 0xec,
  0x65, 0x8e, 0x38, 0x89, 0x0d, 0x0b, 0x50, 0xe4, 0x11, 0x49, 0x86, 0x39,
  0x82, 0xdb, 0x73, 0xe5, 0x3a, 0x0f, 0x13, 0x22, 0xab, 0xad, 0xa0, 0x78,
  0x9b, 0x94, 0x21, 0x02, 0x81, 0x81, 0x00, 0xfb, 0xcd, 0x4c, 0x52, 0x49,
  0x3f, 0x2c, 0x80, 0x94, 0x91, 0x4a, 0x38, 0xec, 0x0f, 0x4a, 0x7d, 0x3a,
  0x8e, 0xbc, 0x04, 0x90, 0x15, 0x25, 0x84, 0xfb, 0xd3, 0x68, 0xbd, 0xef,
  0xa0, 0x47, 0xfe, 0xce, 0x5b, 0xbf, 0x1d, 0x2a, 0x94, 0x27, 0xfc, 0x51,
  0x70, 0xff, 0xc9, 0xe9, 0xba, 0xbe, 0x2b, 0xa0, 0x50, 0x25, 0xd3, 0xe1,
  0xa1, 0x57, 0x33, 0xcc, 0x5c, 0xc7, 0x7d, 0x09, 0xf6, 0xdc, 0xfb, 0x72,
  0x94, 0x3d, 0xca, 0x59, 0x52, 0x73, 0xe0, 0x6c, 0x45, 0x0a, 0xd9, 0xda,
  0x30, 0xdf, 0x2b, 0x33, 0xd7, 0x52, 0x18, 0x41, 0x01, 0xf0, 0xdf, 0x1b,
  0x01, 0xc1, 0xd3, 0xb7, 0x9b, 0x26, 0xf8, 0x1c, 0x8f, 0xff, 0xc8, 0x19,
  0xfd, 0x36, 0xd0, 0x13, 0xa5, 0x72, 0x42, 0xa3, 0x30, 0x59, 0x57, 0xb4,
  0xda, 0x2a, 0x09, 0xe5, 0x45, 0x5a, 0x39, 0x6d, 0x70, 0x22, 0x0c, 0xba,
  0x53, 0x26, 0x8d, 0x02, 0x81, 0x81, 0x00, 0xb1, 0x3c, 0xc2, 0x70, 0xf0,
  0x93, 0xc4, 0x3c, 0xf6, 0xbe, 0x13, 0x11, 0x98, 0x48, 0x82, 0xe1, 0x19,
  0x61, 0xbb, 0x0a, 0x7d, 0x80, 0x0e, 0x3b, 0xf6, 0xc0, 0xc4, 0xe2, 0xdf,
  0x19, 0x03, 0x23, 0x51, 0x44, 0x41, 0x08, 0x29, 0xb2, 0xe8, 0xc6, 0x50,
  0xcf, 0x5f, 0xdd, 0x49, 0xf5, 0x03, 0xde, 0xee, 0x86, 0x82, 0x6a, 0x5a,
  0x0b, 0x4f, 0xdc, 0xbe, 0x63, 0x02, 0x26, 0x91, 0x18, 0x4e, 0xa1, 0xce,
  0xaf, 0xf1, 0x8e, 0x88, 0xe3, 0x30, 0xf4, 0xf5, 0xff, 0x71, 0xeb, 0xdf,
  0x23, 0x3e, 0x14, 0x52, 0x88, 0xca, 0x3f, 0x03, 0xbe, 0xb4, 0xe1, 0xa0,
  0x6e, 0x28, 0x4e, 0x8a, 0x65, 0x73, 0x5d, 0x85, 0xaa, 0x88, 0x5f, 0x8f,
  0x90, 0xf0, 0x3f, 0x00, 0x63, 0x52, 0x92, 0x6c, 0xd1, 0xc4, 0x52, 0x0d,
  0x5e, 0x04, 0x17, 0x7d, 0x7c, 0xa1, 0x86, 0x54, 0x5a, 0x9d, 0x0e, 0x0c,
  0xdb, 0xa0, 0x21, 0x02, 0x81, 0x81, 0x00, 0xea, 0xfe, 0x1b, 0x9e, 0x27,
  0xb1, 0x87, 0x6c, 0xb0, 0x3a, 0x2f, 0x94, 0x93, 0xe9, 0x69, 0x51, 0x19,
  0x97, 0x1f, 0xac, 0xfa, 0x72, 0x61, 0xc3, 0x8b, 0xe9, 0x2e, 0xb5, 0x23,
  0xae, 0xe7, 0xc1, 0xcb, 0x00, 0x20, 0x89, 0xad, 0xb4, 0xfa, 0xe4, 0x25,
  0x75, 0x59, 0xa2, 0x2c, 0x39, 0x15, 0x45, 0x4d, 0xa5, 0xbe, 0xc7, 0xd0,
  0xa8, 0x6b, 0xe3, 0x71, 0x73, 0x9c, 0xd0, 0xfa, 0xbd, 0xa2, 0x5a, 0x20,
  0x02, 0x6c, 0xf0, 0x2d, 0x10, 0x20, 0x08, 0x6f, 0xc2, 0xb7, 0x6f, 0xbc,
  0x8b, 0x23, 0x9b, 0x04, 0x14, 0x8d, 0x0f, 0x09, 0x8c, 0x30, 0x29, 0x66,
  0xe0, 0xea, 0xed, 0x15, 0x4a, 0xfc, 0xc1, 0x4c, 0x96, 0xae, 0xd5, 0x26,
  0x3c, 0x04, 0x2d, 0x88, 0x48, 0x3d, 0x2c, 0x27, 0x73, 0xf5, 0xcd, 0x3e,
  0x80, 0xe3, 0xfe, 0xbc, 0x33, 0x4f, 0x12, 0x8d, 0x29, 0xba, 0xfd, 0x39,
  0xde, 0x63, 0xf9, 0x02, 0x81, 0x81, 0x00, 0x8b, 0x1f, 0x47, 0xa2, 0x90,
  0x4b, 0x82, 0x3b, 0x89, 0x2d, 0xe9, 0x6b, 0xe1, 0x28, 0xe5, 0x22, 0x87,
  0x83, 0xd0, 0xde, 0x1e, 0x0d, 0x8c, 0xcc, 0x84, 0x43, 0x3d, 0x23, 0x8d,
  0x9d, 0x6c, 0xbc, 0xc4, 0xc6, 0xda, 0x44, 0x44, 0x79, 0x20, 0xb6, 0x3e,
  0xef, 0xcf, 0x8a, 0xc4, 0x38, 0xb0, 0xe5, 0xda, 0x45, 0xac, 0x5a, 0xcc,
  0x7b, 0x62, 0xba, 0xa9, 0x73, 0x1f, 0xba, 0x27, 0x5c, 0x82, 0xf8, 0xad,
  0x31, 0x1e, 0xde, 0xf3, 0x37, 0x72, 0xcb, 0x47, 0xd2, 0xcd, 0xf7, 0xf8,
  0x7f, 0x00, 0x39, 0xdb, 0x8d, 0x2a, 0xca, 0x4e, 0xc1, 0xce, 0xe2, 0x15,
  0x89, 0xd6, 0x3a, 0x61, 0xae, 0x9d, 0xa2, 0x30, 0xa5, 0x85, 0xae, 0x38,
  0xea, 0x46, 0x74, 0xdc, 0x02, 0x3a, 0xac, 0xe9, 0x5f, 0xa3, 0xc6, 0x73,
  0x4f, 0x73, 0x81, 0x90, 0x56, 0xc3, 0xce, 0x77, 0x5f, 0x5b, 0xba, 0x6c,
  0x42, 0xf1, 0x21
};
unsigned int server_key_rsa_der_len = 1191;

/**
 * This is an example which echos any data it receives on UART1 back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: UART1
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below
 */

#define ECHO_TEST_TXD  (GPIO_NUM_4)
#define ECHO_TEST_RXD  (GPIO_NUM_5)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

#define EXAMPLE_BUFFER_SZ 4096

#define SSH_GREETING_MESSAGE (byte*)((char*)"Welcome to SSH Server!\r\nYou have now logged in!\r\n\r\n")

static INLINE void c32toa(word32 u32, byte* c)
{
    c[0] = (u32 >> 24) & 0xff;
    c[1] = (u32 >> 16) & 0xff;
    c[2] = (u32 >>  8) & 0xff;
    c[3] =  u32        & 0xff;
}

/* Map user names to passwords */
/* Use arrays for username and p. The password or public key can
 * be hashed and the hash stored here. Then I won't need the type. */
typedef struct PwMap {
    byte type;
    byte username[32];
    word32 usernameSz;
    byte p[SHA256_DIGEST_SIZE];
    struct PwMap* next;
} PwMap;


typedef struct PwMapList {
    PwMap* head;
} PwMapList;


static PwMap* PwMapNew(PwMapList* list, byte type, const byte* username,
                       word32 usernameSz, const byte* p, word32 pSz)
{
    PwMap* map;

    map = (PwMap*)malloc(sizeof(PwMap));
    if (map != NULL) {
        Sha256 sha;
        byte flatSz[4];

        map->type = type;
        if (usernameSz >= sizeof(map->username))
            usernameSz = sizeof(map->username) - 1;
        memcpy(map->username, username, usernameSz + 1);
        map->username[usernameSz] = 0;
        map->usernameSz = usernameSz;

        wc_InitSha256(&sha);
        c32toa(pSz, flatSz);
        wc_Sha256Update(&sha, flatSz, sizeof(flatSz));
        wc_Sha256Update(&sha, p, pSz);
        wc_Sha256Final(&sha, map->p);

        map->next = list->head;
        list->head = map;
    }

    return map;
}


static void PwMapListDelete(PwMapList* list)
{
    if (list != NULL) {
        PwMap* head = list->head;

        while (head != NULL) {
            PwMap* cur = head;
            head = head->next;
            memset(cur, 0, sizeof(PwMap));
            free(cur);
        }
    }
}

static const char samplePasswordBuffer[] = "hopkins:hopkinsdev\n";

static int LoadPasswordBuffer(byte* buf, word32 bufSz, PwMapList* list)
{
    char* str = (char*)buf;
    char* delimiter;
    char* username;
    char* password;

    /* Each line of passwd.txt is in the format
     *     username:password\n
     * This function modifies the passed-in buffer. */

    if (list == NULL)
        return -1;

    if (buf == NULL || bufSz == 0)
        return 0;

    while (*str != 0) {
        delimiter = strchr(str, ':');
        username = str;
        *delimiter = 0;
        password = delimiter + 1;
        str = strchr(password, '\n');
        *str = 0;
        str++;
        if (PwMapNew(list, WOLFSSH_USERAUTH_PASSWORD,
                     (byte*)username, (word32)strlen(username),
                     (byte*)password, (word32)strlen(password)) == NULL ) {

            return -1;
        }
    }

    return 0;
}

static int wsUserAuth(byte authType,
                      WS_UserAuthData* authData,
                      void* ctx)
{
    PwMapList* list;
    PwMap* map;
    byte authHash[SHA256_DIGEST_SIZE];

    if (ctx == NULL) {
        fprintf(stderr, "wsUserAuth: ctx not set");
        return WOLFSSH_USERAUTH_FAILURE;
    }

    if (authType != WOLFSSH_USERAUTH_PASSWORD &&
        authType != WOLFSSH_USERAUTH_PUBLICKEY) {

        return WOLFSSH_USERAUTH_FAILURE;
    }

    /* Hash the password or public key with its length. */
    {
        Sha256 sha;
        byte flatSz[4];
        wc_InitSha256(&sha);
        if (authType == WOLFSSH_USERAUTH_PASSWORD) {
            c32toa(authData->sf.password.passwordSz, flatSz);
            wc_Sha256Update(&sha, flatSz, sizeof(flatSz));
            wc_Sha256Update(&sha,
                            authData->sf.password.password,
                            authData->sf.password.passwordSz);
        }
        else if (authType == WOLFSSH_USERAUTH_PUBLICKEY) {
            c32toa(authData->sf.publicKey.publicKeySz, flatSz);
            wc_Sha256Update(&sha, flatSz, sizeof(flatSz));
            wc_Sha256Update(&sha,
                            authData->sf.publicKey.publicKey,
                            authData->sf.publicKey.publicKeySz);
        }
        wc_Sha256Final(&sha, authHash);
    }

    list = (PwMapList*)ctx;
    map = list->head;

    while (map != NULL) {
        if (authData->usernameSz == map->usernameSz &&
            memcmp(authData->username, map->username, map->usernameSz) == 0) {

            if (authData->type == map->type) {
                if (memcmp(map->p, authHash, SHA256_DIGEST_SIZE) == 0) {
                    return WOLFSSH_USERAUTH_SUCCESS;
                }
                else {
                    return (authType == WOLFSSH_USERAUTH_PASSWORD ?
                            WOLFSSH_USERAUTH_INVALID_PASSWORD :
                            WOLFSSH_USERAUTH_INVALID_PUBLICKEY);
                }
            }
            else {
                return WOLFSSH_USERAUTH_INVALID_AUTHTYPE;
            }
        }
        map = map->next;
    }

    return WOLFSSH_USERAUTH_INVALID_USER;
}

static byte find_char(const byte* str, const byte* buf, word32 bufSz)
{
    const byte* cur;

    while (bufSz) {
        cur = str;
        while (*cur != '\0') {
            if (*cur == *buf)
                return *cur;
            cur++;
        }
        buf++;
        bufSz--;
    }

    return 0;
}

typedef struct {
    WOLFSSH* ssh;
    SOCKET_T fd;
    word32 id;
} thread_ctx_t;

static int dump_stats(thread_ctx_t* ctx)
{
    char stats[1024];
    word32 statsSz;
    word32 txCount, rxCount, seq, peerSeq;

    wolfSSH_GetStats(ctx->ssh, &txCount, &rxCount, &seq, &peerSeq);

    WSNPRINTF(stats, sizeof(stats),
            "Statistics for Thread #%u:\r\n"
            "  txCount = %u\r\n  rxCount = %u\r\n"
            "  seq = %u\r\n  peerSeq = %u\r\n",
            ctx->id, txCount, rxCount, seq, peerSeq);
    statsSz = (word32)strlen(stats);

    //ESP_LOGI(TAG, "%s", stats);
    return wolfSSH_stream_send(ctx->ssh, (byte*)stats, statsSz);
}

void server_worker()
{
    thread_ctx_t* threadCtx = (thread_ctx_t*)vArgs;
    //ESP_LOGI(TAG, "server_worker: entry");

    int acceptStatus;

    if ((acceptStatus = wolfSSH_accept(threadCtx->ssh)) == WS_SUCCESS) {
    	//ESP_LOGW(TAG, "wolfSSH_accept == WS_SUCCESS");
        byte* buf = NULL;
        byte* tmpBuf;
        int bufSz, backlogSz = 0, rxSz, txSz, stop = 0, txSum;

        wolfSSH_stream_send(threadCtx->ssh, SSH_GREETING_MESSAGE, strlen((char *)SSH_GREETING_MESSAGE));
        buf = (byte*)malloc(1024);

        do {
        	rxSz = wolfSSH_stream_read(threadCtx->ssh, buf, 1024);
        	for(int i = 0; rxSz > i; i++) {
        		pc.putc(buf[i]);
        	}
            if(rxSz == WS_WANT_READ) { // TCP Blocking socket
        		// Use this "free" time to do Tx to ssh
        		//int uartLen = uart_read_bytes(UARTInData, BUF_SIZE, portTICK_RATE_MS);
        		//wolfSSH_stream_send(threadCtx->ssh, UARTInData, uartLen);
        	} else {
        		stop = 1;
        	}
        } while(!stop);

        /*do {
            bufSz = EXAMPLE_BUFFER_SZ + backlogSz;

            tmpBuf = (byte*)realloc(buf, bufSz);
            if (tmpBuf == NULL)
                stop = 1;
            else
                buf = tmpBuf;


        	ESP_LOGI(TAG, "realloc buf completed, stop=%d", stop);


            if (!stop) {
                rxSz = wolfSSH_stream_read(threadCtx->ssh,
                                           buf + backlogSz,
                                           EXAMPLE_BUFFER_SZ);
                ESP_LOGI(TAG, "wolfSSH_stream_read: %d", rxSz);
                if (rxSz > 0) {
                    backlogSz += rxSz;
                    txSum = 0;
                    txSz = 0;

                    while (backlogSz != txSum && txSz >= 0 && !stop) {
                        txSz = wolfSSH_stream_send(threadCtx->ssh,
                                                   buf + txSum,
                                                   backlogSz - txSum);

                        if (txSz > 0) {
                            byte c;
                            const byte matches[] = { 0x03, 0x05, 0x06, 0x00 };

                            c = find_char(matches, buf + txSum, txSz);
                            switch (c) {
                                case 0x03:
                                    stop = 1;
                                    break;
                                case 0x06:
                                    if (wolfSSH_TriggerKeyExchange(threadCtx->ssh)
                                            != WS_SUCCESS)
                                        stop = 1;
                                    break;
                                case 0x05:
                                    if (dump_stats(threadCtx) <= 0)
                                        stop = 1;
                                    break;
                            }
                            txSum += txSz;
                        }
                        else if (txSz != WS_REKEYING)
                            stop = 1;
                    }

                    if (txSum < backlogSz)
                        memmove(buf, buf + txSum, backlogSz - txSum);
                    backlogSz -= txSum;
                }
                else
                    stop = 1;
            }
        } while (!stop);*/

        free(buf);
    }
    WCLOSESOCKET(threadCtx->fd);
    wolfSSH_free(threadCtx->ssh);
    free(threadCtx);


    //ESP_LOGW(TAG, "server_worker: exit; accept status=%d", acceptStatus);

    return 0;
}

void echoserver_test()
{
    WOLFSSH_CTX* ctx = NULL;
    PwMapList pwMapList;
    SOCKET_T listenFd = 0;
    word32 defaultHighwater = 0x3FFF8000;
    word32 threadCount = 0;
    bool multipleConnections = true;
    int useEcc = 0;
    char ch;
    word16 port = wolfSshPort;

    //ESP_LOGI(TAG, "echoserver_test");
    if (wolfSSH_Init() != WS_SUCCESS) {
        //ESP_LOGE(TAG, "Couldn't initialize wolfSSH.");
        exit(EXIT_FAILURE);
    }
    //ESP_LOGI(TAG, "wolfSSH_Init OK");

    ctx = wolfSSH_CTX_new(WOLFSSH_ENDPOINT_SERVER, NULL);
    if (ctx == NULL) {
    	//ESP_LOGE(TAG, "Couldn't allocate SSH CTX data.");
        exit(EXIT_FAILURE);
    }
    //ESP_LOGI(TAG, "wolfSSH_CTX_new OK");

    memset(&pwMapList, 0, sizeof(pwMapList));
	wolfSSH_SetUserAuth(ctx, wsUserAuth);
    //ESP_LOGI(TAG, "wolfSSH_SetUserAuth OK");
    wolfSSH_CTX_SetBanner(ctx, "WolfSSH Server ");
    //ESP_LOGI(TAG, "wolfSSH_CTX_SetBanner OK");


    {
		const char* bufName;
		byte buf[1200];
		word32 bufSz;

		memcpy(buf, server_key_rsa_der, server_key_rsa_der_len);
		bufSz = server_key_rsa_der_len;

		if (wolfSSH_CTX_UsePrivateKey_buffer(ctx, buf, bufSz, WOLFSSH_FORMAT_ASN1) < 0) {
	    	//ESP_LOGE(TAG, "Couldn't use key buffer.");
			exit(EXIT_FAILURE);
		}
	    //ESP_LOGI(TAG, "wolfSSH_CTX_UsePrivateKey_buffer OK");

		bufSz = (word32)strlen(samplePasswordBuffer);
		memcpy(buf, samplePasswordBuffer, bufSz);
		buf[bufSz] = 0;
		LoadPasswordBuffer(buf, bufSz, &pwMapList);
	    //ESP_LOGI(TAG, "LoadPasswordBuffer OK");
	}

    // Sig: static INLINE void tcp_listen(SOCKET_T* sockfd, word16* port, int useAnyAddr)
	//tcp_listen(&listenFd, &port, 1);

	do {
		SOCKET_T      clientFd = 0;
		//SOCKADDR_IN_T clientAddr;
		//socklen_t     clientAddrSz = sizeof(clientAddr);
		WOLFSSH*      ssh;
		thread_ctx_t* threadCtx;

		//ESP_LOGI(TAG, "Main loop.");

		threadCtx = (thread_ctx_t*)malloc(sizeof(thread_ctx_t));
		if (threadCtx == NULL) {
			//ESP_LOGE(TAG, "Couldn't allocate thread context data.");
			exit(EXIT_FAILURE);
		}
		//ESP_LOGI(TAG, "threadCtx OK.");

		ssh = wolfSSH_new(ctx);
		//ESP_LOGI(TAG, "Available HEAP Size: %d", esp_get_free_heap_size());
		if (ssh == NULL) {
			//ESP_LOGE(TAG, "Couldn't allocate SSH data.");
			exit(EXIT_FAILURE);
		}
		//ESP_LOGI(TAG, "wolfSSH_new OK.");
		wolfSSH_SetUserAuthCtx(ssh, &pwMapList);
		//ESP_LOGI(TAG, "wolfSSH_SetUserAuthCtx OK.");
		/* Use the session object for its own highwater callback ctx */
		if (defaultHighwater > 0) {
			wolfSSH_SetHighwaterCtx(ssh, (void*)ssh);
			//ESP_LOGI(TAG, "wolfSSH_SetHighwaterCtx OK.");
			wolfSSH_SetHighwater(ssh, defaultHighwater);
			//ESP_LOGI(TAG, "wolfSSH_SetHighwater OK.");
		}

		//SignalTcpReady(NULL, port);
    PrintfTCPHandler tcphandler;
    TCPSocket server;
    server.open(&interface);
    server.bind(22);
    server.listen(1);
    TCPSocket *client;
    client = server.accept();

		//clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientAddrSz);
		if (clientFd == -1)
			err_sys("tcp accept failed");
		//ESP_LOGI(TAG, "accept OK.");
		//ESP_LOGW(TAG, "Setting to non-block");
	    fcntl(clientFd, F_SETFL, O_NONBLOCK);

		wolfSSH_set_fd(ssh, (int)clientFd);

		threadCtx->ssh = ssh;
		threadCtx->fd = clientFd;
		threadCtx->id = threadCount++;

		server_worker(threadCtx);

	} while (multipleConnections);

	PwMapListDelete(&pwMapList);
	wolfSSH_CTX_free(ctx);
	if (wolfSSH_Cleanup() != WS_SUCCESS) {
		fprintf(stderr, "Couldn't clean up wolfSSH.\n");
		exit(EXIT_FAILURE);
	}

    return 0;
}

//const int CONNECTED_BIT = BIT0;
//static ip4_addr_t s_ip_addr;

void ssh()
{
	wolfSSH_Debugging_ON();
	wolfSSH_Init();
	ChangeToWolfSshRoot();
	echoserver_test();
	wolfSSH_Cleanup();
}
