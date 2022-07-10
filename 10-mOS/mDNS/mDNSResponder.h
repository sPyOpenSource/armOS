/*
    Made by JB RYU, KOR - 2015
    
    class mDNSResponder
*/

#ifndef MDNS_RESPONDER_H
#define MDNS_RESPONDER_H

#include "lwip/opt.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include "dns-sd.h"

#if LWIP_DNS 

#include "mbed.h"
#include "mem.h"
#include "memp.h"
#include "dns.h"

#define MDNS_PORT (5353)
#define MCAST "224.0.0.251"

class mDNSResponder
{
public:
    mDNSResponder(EthernetInterface);
    virtual ~mDNSResponder();
    void close();
    void announce(const char* ip);
    void MDNS_process();
  
private:
    void IPstringToByte(char* IPstr);
    char* skip_name(char* query);
    void query_domain(void);
    void register_service(char* number);
    char* decode_name(char* query, char* packet);
    void send_dns_ans(struct dns_hdr* hdr);

    /*static objs */
    static MY_SD_DOMAINS SD_domains;
    static struct dns_hdr SD_res_hdr;

    static char query_buf[1500];
    static QR_MAP g_queries;
    
    char* IP_str;
    uint8_t IP_byte[4];
    
    UDPSocket mdns_sock;
    SocketAddress rcv_endpt; //initial value := ip addr any
    SocketAddress send_endpt;
};

#endif

#ifndef htons
#define htons( x ) ( (( x << 8 ) & 0xFF00) | (( x >> 8 ) & 0x00FF) )
#define ntohs( x ) (htons(x))
#endif

#ifndef htonl
#define htonl( x ) ( (( x << 24 ) & 0xff000000)  \
                   | (( x <<  8 ) & 0x00ff0000)  \
                   | (( x >>  8 ) & 0x0000ff00)  \
                   | (( x >> 24 ) & 0x000000ff)  )
#define ntohl( x ) (htonl(x))
#endif

#endif
