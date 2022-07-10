/*
    ----------------------------------------------------
    Made by JB RYU, KOR - 2015
    
    origin - C /  mdns-sd on CONTIKI-ipv6(uIP) I'd made
        
    now - mdns-sd on MBED-LWIP-ipv4
    ----------------------------------------------------
    More progressed things than others :
    {
        Parsing multi-records (OK),
        Check duplicated host and avoid (OK)
    }
    
    TODO : (Actully these are not necessary for my prj)
    {
        register multiple service, 
        managing records-caches
    }
    ----------------------------------------------------
    Notes :
        srv-instance-name = host-name
        SEP-<rand 2hex><IP 2hex>
    ----------------------------------------------------
*/


#include "lwip/opt.h"
#include "mbed.h"
#include "EthernetInterface.h"
#include <cstring>
#if LWIP_DNS /* don't build if not configured for use in lwipopts.h */

#include "mDNSResponder.h"
#include "dns-sd.h"

#define MDNS_DBG   1
#if (MDNS_DBG > 0)
#define MDBG(x...) printf(x)
#else
#define MDBG(x...)
#endif

mDNSResponder::mDNSResponder(EthernetInterface interface) : IP_str(NULL)
{
    mdns_sock.open(&interface);
}

/* initialize static instance */
char mDNSResponder::query_buf[1500] =
{
    0,
};

MY_SD_DOMAINS mDNSResponder::SD_domains =
{
    .numbers = N_CONTIKI_SERVICES,
    .elements =
    {
        {
            .inst_len = 5,
            .instance = {'s','p','y','o','s'},
            .serv_len = 12,
            .service = {'_','s','m','a','r','t','e','n','e','r','g','y',},
            .trl_len = 4,
            .trl = {'_','t','c','p'},
            .domain_len = 5,
            .domain = {'l','o','c','a','l'},
            .txt =
            {
                .len_txtvers = 9,
                .txtvers = {'t','x','t','v','e','r','s','=','1'},
                .len_dcap = 10,
                .dcap = {'d','c','a','p','=','/','d','c','a','p'},
                .len_path = 9,
                .path = {'p','a','t','h','=','/','u','p','t'},
                .len_https = 9,
                .https = {'h','t','t','p','s','=','4','4','3'},
                .len_level = 9,
                .level = {'l','e','v','e','l','=','-','S','0'},
            }
        },
    },
};

struct dns_hdr mDNSResponder::SD_res_hdr =
{
    .id = 0x0000,
    .flags1 = DNS_FLAG1_RESPONSE | DNS_FLAG1_AUTHORATIVE,
    .flags2 = 0,
    .numquestions = 0,
    .numanswers = htons(1),
    .numauthrr = 0,
    .numextrarr = 0,
};

QR_MAP mDNSResponder::g_queries = {0,};

/* end of initialize static instance */

mDNSResponder::~mDNSResponder() 
{
    close();
}

void mDNSResponder::
register_service(char* number)
{
    memcpy(&SD_domains.elements[0].instance[4], number, 4);
}

void mDNSResponder::
query_domain(void)
{
    volatile uint8_t i;
    struct dns_hdr* hdr;
    struct dns_question* question;
    uint8_t *query;

    MDBG("start probe duplicate domain\n\r");

    hdr = (struct dns_hdr *)query_buf;
    
    memset(hdr, 0, sizeof(struct dns_hdr));
    
    hdr->id = 0;
    hdr->flags1 = DNS_FLAG1_RD;
    hdr->numquestions = htons(1);
    
    query = (unsigned char *)query_buf + sizeof(*hdr);
    
    memcpy(query, &SD_domains.elements[0].inst_len, 
        SD_domains.elements[0].inst_len+1);
    query += SD_domains.elements[0].inst_len+1;

    memcpy(query, &SD_domains.elements[0].domain_len,
        SD_domains.elements[0].domain_len+1);
    query += SD_domains.elements[0].domain_len+1;

    *query = 0x00;
    query++;
    
    question = (struct dns_question*)query;
    question->type = htons(DNS_TYPE_ANY);
    question->obj = htons(DNS_CLASS_IN);
    
    query += 4;

    MDBG("send probe query(%d)\n\r", 
        (uint16_t)((uint32_t)query - (uint32_t)query_buf));

    mdns_sock.sendto(send_endpt, query_buf,
        (query - (uint8_t *) query_buf));
}

void mDNSResponder::announce(const char* ip) 
{
    send_endpt.set_ip_address(MCAST);   
    send_endpt.set_port(MDNS_PORT);

    mdns_sock.bind(MDNS_PORT);
    if (mdns_sock.join_multicast_group(send_endpt) != 0) 
    {
        printf("Error joining the multicast group\n");
        while(true);
    }
    
    IP_str = new char[16];
    strcpy(IP_str, ip);
    
    IPstringToByte(IP_str);

    char instance_number[4];

    register_service(instance_number);
    
    query_domain();
}

void mDNSResponder::close() 
{
    mdns_sock.close();
}

void mDNSResponder::IPstringToByte(char* IPstr)
{
    char ip1[4] = {0,}; char ip2[4] = {0,};
    char ip3[4] = {0,}; char ip4[4] = {0,};
    char* p;
    char* p_dot;

    p_dot = strstr(IPstr, ".");
    memcpy(ip1, IPstr, p_dot - IPstr);
    IP_byte[0] = (uint8_t)atoi((const char*)ip1);
    
    p = p_dot + 1;
    p_dot = strstr(p, ".");
    memcpy(ip2, p, p_dot - p);
    IP_byte[1] = (uint8_t)atoi((const char*)ip2);
    
    p = p_dot + 1;
    p_dot = strstr(p, ".");
    memcpy(ip3, p, p_dot - p);
    IP_byte[2] = (uint8_t)atoi((const char*)ip3);
    
    p = p_dot + 1;
    memcpy(ip4, p, 3);
    IP_byte[3] = (uint8_t)atoi((const char*)ip4);
}

char* mDNSResponder::
skip_name(char* query)
{
  unsigned char n;

  do 
    {
    n = *query;
    if(n & 0xc0) 
        {
      ++query;
      break;
    }

    ++query;

    while(n > 0) 
        {
      ++query;
      --n;
    };
  } while(*query != 0);
    
  return query + 1;
}

char* mDNSResponder::
decode_name(char* query, char* packet)
{
    char c = 0xff;
    static char dest[MAX_DOMAIN_LEN];
    int len = 0;

    memset(dest, 0, MAX_DOMAIN_LEN);

    MDBG("\n\r");
    while((len < MAX_DOMAIN_LEN) && (c != 0x00)) 
    {
        c = *query;
        
        while (c == 0xc0) 
        {
            MDBG("%s : read offset\n\r", __FUNCTION__);
            query++;
            query  = (char*)((uint32_t)packet + *query);
            c = *query;
        }

        memset(&dest[len], c, 1);

        query++;
        len++;
    }

    MDBG("\n%s : %s\n\r", __FUNCTION__, dest);
    return dest;
}

void mDNSResponder::
send_dns_ans(struct dns_hdr* hdr)
{
    int i = 0;
    uint8_t* ptr;
    uint32_t off_inst = 0;
    uint32_t off_serv = 0;
    uint32_t off_domain = 0; 
    PTR_ANS* ptr_ans;
    SRV_ANS* srv_ans;
    struct dns_answer* A_ans;
    TXT_ANS* txt_ans;
    int numbers = g_queries.numbers;

    uint8_t ans_mask = DNS_TYPE_ANY; //later, for addtional ans

    memcpy(hdr, &SD_res_hdr, sizeof(struct dns_hdr));
    hdr->numanswers = htons(numbers);
    ptr = (uint8_t*)hdr + sizeof(struct dns_hdr);

    while (numbers--)
    {
        switch (g_queries.reqs[i])
        {
            case DNS_TYPE_PTR :
            {
                ans_mask = ans_mask ^ DNS_TYPE_PTR;
                if (off_serv == 0)
                {
                    off_serv = (uint32_t)ptr - (uint32_t)hdr;

                    memcpy(ptr, &SD_domains.elements[0].serv_len,
                        SD_domains.elements[0].serv_len + 1);
                    ptr += SD_domains.elements[0].serv_len + 1;

                    memcpy(ptr, &SD_domains.elements[0].trl_len,
                        SD_domains.elements[0].trl_len + 1);
                    ptr += SD_domains.elements[0].trl_len + 1;
                
                    memcpy(ptr, &SD_domains.elements[0].domain_len,
                        SD_domains.elements[0].domain_len + 1);
                    ptr += SD_domains.elements[0].domain_len + 1;
                    *ptr = 0x00;
                    ptr++;
                }
                else
                {
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_serv;
                    ptr++;
                }

                ptr_ans = (PTR_ANS*)ptr;
                
                ptr_ans->type = htons(DNS_TYPE_PTR);
                ptr_ans->obj = htons(DNS_CLASS_IN);
                ptr_ans->ttl[0] = 0;
                ptr_ans->ttl[1] = htons(120);
            
                int instance_len = SD_domains.elements[0].inst_len;
                
                ptr = (uint8_t*)ptr_ans->name;

                if (off_inst == 0)
                {
                    off_inst = (uint32_t)ptr - (uint32_t)hdr;

                    ptr_ans->len = htons(instance_len + 3);
                    
                    memcpy(ptr_ans->name, 
                        &SD_domains.elements[0].inst_len, 
                        1 + SD_domains.elements[0].inst_len);
                
                    ptr += instance_len + 1;
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_serv;
                    ptr++;
                }
                else
                {
                    ptr_ans->len = htons(2);
                    
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_inst;
                    ptr++;
                }
                break;
            }
            case DNS_TYPE_SRV :
                ans_mask = ans_mask ^ DNS_TYPE_SRV;
                if (off_inst == 0)
                {
                    off_inst = (uint32_t)ptr - (uint32_t)hdr;
                    
                    memcpy(ptr, 
                        &SD_domains.elements[0].inst_len, 
                        1 + SD_domains.elements[0].inst_len);

                    ptr += 1 + SD_domains.elements[0].inst_len;

                    if (off_serv == 0)
                    {
                        off_serv = (uint32_t)ptr - (uint32_t)hdr;
                        
                        memcpy(ptr, &SD_domains.elements[0].serv_len,
                            SD_domains.elements[0].serv_len + 1);
                        ptr += SD_domains.elements[0].serv_len + 1;

                        memcpy(ptr, &SD_domains.elements[0].trl_len,
                            SD_domains.elements[0].trl_len + 1);
                        ptr += SD_domains.elements[0].trl_len + 1;
                    
                        memcpy(ptr, &SD_domains.elements[0].domain_len,
                            SD_domains.elements[0].domain_len + 1);
                        ptr += SD_domains.elements[0].domain_len + 1;

                        *ptr = 0x00;
                        ptr++;
                    }
                    else
                    {
                        *ptr = 0xc0;
                        ptr++;
                        *ptr = (uint8_t)off_serv;
                        ptr++;
                    }
                }
                else
                {
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_inst;
                    ptr++;
                }
                
                srv_ans = (SRV_ANS*)(ptr);
                srv_ans->type = htons(DNS_TYPE_SRV);
                srv_ans->obj = htons(DNS_CLASS_IN | DNS_CASH_FLUSH);
                srv_ans->ttl[0] = 0;
                srv_ans->ttl[1] = htons(120);
                srv_ans->pri = 0;
                srv_ans->wei = 0;
                srv_ans->port = htons(8080);
                ptr = (uint8_t*)srv_ans->name;

                if (off_domain == 0)
                {
                    off_domain = (uint32_t)ptr - (uint32_t)hdr;

                    memcpy(ptr, 
                        &SD_domains.elements[0].inst_len,
                        1 + SD_domains.elements[0].inst_len);

                    ptr += 1 + SD_domains.elements[0].inst_len;

                    memcpy(ptr,
                        &SD_domains.elements[0].domain_len,
                        1 + SD_domains.elements[0].domain_len);

                    ptr+= 1 + SD_domains.elements[0].domain_len;
                    *ptr = 0x00;
                    ptr++;
                }
                else
                {
                  *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_domain;
                    ptr++;
                }
                srv_ans->len =
                    htons((uint16_t)((uint32_t)ptr - (uint32_t)&srv_ans->pri));

                break;
            case DNS_TYPE_TXT :
                ans_mask = ans_mask ^ DNS_TYPE_TXT;
                if (off_inst == 0)
                {
                    off_inst = (uint32_t)ptr - (uint32_t)hdr;
                    
                    memcpy(ptr, 
                        &SD_domains.elements[0].inst_len, 
                        1 + SD_domains.elements[0].inst_len);

                    ptr += 1 + SD_domains.elements[0].inst_len;

                    if (off_serv == 0)
                    {
                        off_serv = (uint32_t)ptr - (uint32_t)hdr;
                        
                        memcpy(ptr, &SD_domains.elements[0].serv_len,
                            SD_domains.elements[0].serv_len + 1);
                        ptr += SD_domains.elements[0].serv_len + 1;

                        memcpy(ptr, &SD_domains.elements[0].trl_len,
                            SD_domains.elements[0].trl_len + 1);
                        ptr += SD_domains.elements[0].trl_len + 1;
                    
                        memcpy(ptr, &SD_domains.elements[0].domain_len,
                            SD_domains.elements[0].domain_len + 1);
                        ptr += SD_domains.elements[0].domain_len + 1;

                        *ptr = 0x00;
                        ptr++;
                    }
                    else
                    {
                        *ptr = 0xc0;
                        ptr++;
                        *ptr = (uint8_t)off_serv;
                        ptr++;
                    }
                }
                else
                {
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_inst;
                    ptr++;
                }
                
                txt_ans = (TXT_ANS*)(ptr);
                txt_ans->type = htons(DNS_TYPE_TXT);
                txt_ans->obj = htons(DNS_CLASS_IN | DNS_CASH_FLUSH);
                txt_ans->ttl[0] = 0;
                txt_ans->ttl[1] = htons(120);
                ptr = (uint8_t*)txt_ans->txt;

                memcpy(ptr,
                    &SD_domains.elements[0].txt.len_txtvers,
                    1 + SD_domains.elements[0].txt.len_txtvers);
                ptr += 1 + SD_domains.elements[0].txt.len_txtvers;

                memcpy(ptr,
                    &SD_domains.elements[0].txt.len_dcap,
                    1 + SD_domains.elements[0].txt.len_dcap);
                ptr += 1 + SD_domains.elements[0].txt.len_dcap;

                memcpy(ptr,
                    &SD_domains.elements[0].txt.len_path,
                    1 + SD_domains.elements[0].txt.len_path);
                ptr += 1 + SD_domains.elements[0].txt.len_path;

                memcpy(ptr,
                    &SD_domains.elements[0].txt.len_https,
                    1 + SD_domains.elements[0].txt.len_https);
                ptr += 1 + SD_domains.elements[0].txt.len_https;                

                memcpy(ptr,
                    &SD_domains.elements[0].txt.len_level,
                    1 + SD_domains.elements[0].txt.len_level);
                ptr += 1 + SD_domains.elements[0].txt.len_level;                

                txt_ans->len =
                    htons((uint16_t)((uint32_t)ptr - (uint32_t)txt_ans->txt));
                
                break;
            case DNS_TYPE_A :
                ans_mask = ans_mask ^ DNS_TYPE_A;
                if (off_domain == 0)
                {
                    off_domain = (uint32_t)ptr - (uint32_t)hdr;
                    
                    memcpy(ptr, 
                        &SD_domains.elements[0].inst_len, 
                        1 + SD_domains.elements[0].inst_len);
                    
                    ptr += 1 + SD_domains.elements[0].inst_len;

                    memcpy(ptr, 
                        &SD_domains.elements[0].domain_len, 
                        1 + SD_domains.elements[0].domain_len);
                    
                    ptr += 1 + SD_domains.elements[0].domain_len;
                    *ptr = 0x00;
                    ptr++;
                }
                else
                {
                    *ptr = 0xc0;
                    ptr++;
                    *ptr = (uint8_t)off_domain;
                    ptr++;
                }
                
                A_ans = (struct dns_answer*)(ptr);
                
                A_ans->type = htons(DNS_TYPE_A);
                A_ans->obj = htons(DNS_CLASS_IN | DNS_CASH_FLUSH);
                A_ans->ttl[0] = 0;
                A_ans->ttl[1] = htons(120);
                A_ans->len = htons(4);
                memcpy((uint8_t*)A_ans->ipaddr, 
                    IP_byte, 4);
                
                ptr = (uint8_t*)&A_ans->ipaddr + 4;
                break;
    
            default :
                break;
        }
        i++;
    }

    int send_len = ptr - (uint8_t*)hdr;

    mdns_sock.sendto(send_endpt, (char*)hdr, send_len);
}

void mDNSResponder::
MDNS_process()
{
    char rcv_buf[ 1500 ];
    uint8_t nquestions, nanswers;
    struct dns_hdr* hdr;
    char *queryptr;
    uint8_t is_request;
        
    MDBG("MDNS Process\n\r");

    while(mdns_sock.recvfrom(&rcv_endpt, 
                    rcv_buf, sizeof(rcv_buf)) != 0)
    {
        hdr = (struct dns_hdr *)rcv_buf;
        queryptr = (char *)hdr + sizeof(*hdr);
        is_request = ((hdr->flags1 & ~1) == 0) && (hdr->flags2 == 0);
        nquestions = (uint8_t) ntohs(hdr->numquestions);
        nanswers = (uint8_t) ntohs(hdr->numanswers);
            
        MDBG("resolver: flags1=0x%02X flags2=0x%02X nquestions=%d,\
             nanswers=%d, nauthrr=%d, nextrarr=%d\n\r",\
             hdr->flags1, hdr->flags2, (uint8_t) nquestions, (uint8_t) nanswers,\
             (uint8_t) ntohs(hdr->numauthrr),\
             (uint8_t) ntohs(hdr->numextrarr));
        
        if(hdr->id != 0)
        {
            return;
        }
    
        /** ANSWER HANDLING SECTION ************************************************/
        struct dns_question* question = (struct dns_question *)skip_name(queryptr);
    
        uint16_t type;
    
        if (is_request)
        {
            char* name;
            
            memset(&g_queries, 0, sizeof(QR_MAP));
            
            while (nquestions--)
            {
                type = ntohs(question->type);
                name = decode_name(queryptr, (char*)hdr);
                
                if ((type & DNS_TYPE_PTR) == DNS_TYPE_PTR)
                {
                    if (strncmp((const char*)name, 
                                (const char*)&SD_domains.elements[0].serv_len, 
                                SD_domains.elements[0].serv_len + 1) == 0)
                    {
                        MDBG("recv PTR request\n\r");
                        g_queries.reqs[g_queries.numbers] = DNS_TYPE_PTR;
                        g_queries.numbers++;
                    }
                }
    
                if ((type & DNS_TYPE_SRV) == DNS_TYPE_SRV)
                {
                    if ((strncmp((const char*)name, 
                                (const char*)&SD_domains.elements[0].inst_len, 
                                SD_domains.elements[0].inst_len + 1) == 0) &&
                            (strncmp((const char*)(name + SD_domains.elements[0].inst_len+1),
                                    (const char*)&SD_domains.elements[0].serv_len,
                                    SD_domains.elements[0].serv_len+1) == 0))
                    {
                        MDBG("recv SRV request\n");
                        g_queries.reqs[g_queries.numbers] = DNS_TYPE_SRV;
                        g_queries.numbers++;
                    }
                }
    
                if ((type & DNS_TYPE_TXT) == DNS_TYPE_TXT)
                {
                    if ((strncmp((const char*)name, 
                                (const char*)&SD_domains.elements[0].inst_len, 
                                SD_domains.elements[0].inst_len + 1) == 0) &&
                            (strncmp((const char*)(name + SD_domains.elements[0].inst_len+1),
                                    (const char*)&SD_domains.elements[0].serv_len,
                                    SD_domains.elements[0].serv_len+1) == 0))
                    {
                        MDBG("recv TXT request\n");
                        g_queries.reqs[g_queries.numbers] = DNS_TYPE_TXT;
                        g_queries.numbers++;
                    }
                }
    
                if ((type & DNS_TYPE_A) == DNS_TYPE_A)
                {
                    if ((strncmp((const char*)name, 
                                (const char*)&SD_domains.elements[0].inst_len, 
                                SD_domains.elements[0].inst_len + 1) == 0) &&
                            (strncmp((const char*)(name + SD_domains.elements[0].inst_len + 1),
                                    (const char*)&SD_domains.elements[0].domain_len, 
                                    SD_domains.elements[0].domain_len + 1) == 0))
                    {
                        MDBG("recv AAAA request\n");
                        g_queries.reqs[g_queries.numbers] = DNS_TYPE_A;
                        g_queries.numbers++;
                    }
                }
    
                queryptr = (char*)question + sizeof(struct dns_question);
                question = (struct dns_question *)skip_name(queryptr);
            }
    
            if (g_queries.numbers)
                send_dns_ans(hdr);
        }
        else
        {
            if (strncmp((const char*)queryptr, 
                        (const char*)&SD_domains.elements[0].inst_len, 
                        SD_domains.elements[0].inst_len + 1) == 0)
            {
                type = ntohs(question->type);
                if (type == DNS_TYPE_A || type == DNS_TYPE_A)
                {
                    char instance_number[4];
                    register_service(instance_number);
                    query_domain();
                }
            }                           
        }
        
    }
}
#endif
