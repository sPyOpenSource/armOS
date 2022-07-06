
#include <EthernetInterface.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include "UnitTest.h"
#include "mDNSResponder.h"

#include <string>
#include "UnitTest.h"
#include <user_settings.h>

#define MAXDATASIZE (1024 * 4)

#define GRAPHICSMODE

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

    //InterruptManager interrupts(0x20, &gdt, &taskManager);

    pc.printf("Initializing Hardware, Stage 1\n\r");
    
    #ifdef GRAPHICSMODE
        Desktop desktop(320, 200, 0x00, 0x00, 0xA8);
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
            VideoGraphicsArray vga;
        #endif
        
    pc.printf("Initializing Hardware, Stage 2\n\r");

    #ifdef GRAPHICSMODE
        vga.SetMode(320, 200, 8);
        Window win1(&desktop, 10, 10, 20, 20, 0xA8, 0x00, 0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);
        desktop.AddChild(&win2);
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
