/*
    Edited by JB RYU, KOR - 2015
    
    DNS-headers and values.
    
    Actually, Defines for DNS-type-values are not necessary.
    LWIP NETIF has defines for DNS formats already.
*/

#ifndef DNS_SD_H_
#define DNS_SD_H_

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define UPPERBYTE(word)\
    (unsigned char)(word >> 8)
#define LOWERBYTE(word)\
    (unsigned char)(word)

/** \internal The DNS message header. */
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

/** \internal The DNS question message structure. */
struct dns_question 
{
  uint16_t type;
  uint16_t obj;
};

struct dns_hdr 
{
  uint16_t id;
  uint8_t flags1, flags2;
  uint16_t numquestions;
  uint16_t numanswers;
  uint16_t numauthrr;
  uint16_t numextrarr;
};

/** \internal The DNS answer message structure. */
struct dns_answer 
{
  /* DNS answer record starts with either a domain name or a pointer
   * to a name already present somewhere in the packet. */
  uint16_t type;
  uint16_t obj;
  uint16_t ttl[2];
  uint16_t len;
#if NET_IPV6
  uint8_t ipaddr[16];
#else
  uint8_t ipaddr[4];
#endif
};

#define DNS_TYPE_A      1
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_PTR   12
#define DNS_TYPE_MX    15
#define DNS_TYPE_TXT   16
#define DNS_TYPE_AAAA  28
#define DNS_TYPE_SRV   33
#define DNS_TYPE_ANY   255
#define DNS_TYPE_NSEC  47

#if NET_IPV6
#define NATIVE_DNS_TYPE DNS_TYPE_AAAA /* IPv6 */
#else
#define NATIVE_DNS_TYPE DNS_TYPE_A    /* IPv4 */
#endif

#define DNS_CLASS_IN        0x0001
#define DNS_CASH_FLUSH      0x8000
#define DNS_CLASS_ANY       0z00ff

/* all entities include its length */
#define MAX_DOMAIN_LEN                  64
#define MAX_INSTANCE_NAME_LEN       64
#define MAX_SUBTYPE_NAME_LEN        69
#define MAX_SERVICE_NAME_LEN        22
#define MAX_SERVICE_DOMAIN_LEN  64
#define MAX_PARENT_DOMAIN_LEN       100 //.local so...
#define MDNS_P_DOMAIN_LEN               5

#define PTR_QUERY_NAME_LEN      18
#pragma pack (push, 1)
typedef struct  
{
  /* DNS answer record starts with either a domain name or a pointer
   * to a name already present somewhere in the packet. */
  uint16_t  type;
  uint16_t  obj;
  uint16_t  ttl[2];
  uint16_t  len;
  char      name[MAX_INSTANCE_NAME_LEN
        + MAX_SERVICE_NAME_LEN + MDNS_P_DOMAIN_LEN];
}PTR_ANS;

typedef struct
{
  uint16_t  type;
  uint16_t  obj;
  uint16_t  ttl[2];
  uint16_t  len;    
  uint16_t  pri;
  uint16_t  wei;
  uint16_t  port;
  char      name[MAX_DOMAIN_LEN
        + MDNS_P_DOMAIN_LEN];
}SRV_ANS;

typedef struct
{
  uint16_t  type;
  uint16_t  obj;
  uint16_t  ttl[2];
  uint16_t  len;    
  char      txt[1024];
}TXT_ANS;

typedef struct
{
    uint8_t len_txtvers;
    char txtvers[10];
    uint8_t len_dcap;
    char dcap[10];
    uint8_t len_path;
    char path[10];
    uint8_t len_https;
    char https[10];
    uint8_t len_level;
    char level[10];
}__SEP_TXT;

typedef struct
{
    uint8_t inst_len;
    char instance[63]; //RYU-PC
    uint8_t serv_len;
    char service[16]; //_http
    uint8_t trl_len;
    char trl[4];            //_tcp
    uint8_t domain_len;
    char domain[5]; //local
    __SEP_TXT txt;
}__SD_DOMAIN;
#pragma pack (pop)

#define N_CONTIKI_SERVICES  1
typedef struct
{
    int numbers;
    __SD_DOMAIN elements[N_CONTIKI_SERVICES];
}MY_SD_DOMAINS;

typedef struct
{
    int numbers;
    uint8_t reqs[10];
}QR_MAP;

#ifdef __cplusplus
}
#endif

#endif

