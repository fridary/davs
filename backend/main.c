/*
 * Copyright (c) 2015
 * Author: Nikita Sushko
 * Url: fridary.com
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <pcap.h>

#include "lib/sha1.c"
#include "lib/base64_enc.c"
#include "lib/websocket.c"


double last_send;
int last_count;

#define BUF_LEN 0xFFFF
#define PACKET_DUMP

uint8_t gBuffer[BUF_LEN];
size_t frameSize = BUF_LEN;
#define prepareBuffer frameSize = BUF_LEN; memset(gBuffer, 0, BUF_LEN);

size_t readedLength = 0;
enum wsFrameType frameType = WS_INCOMPLETE_FRAME;
#define initNewFrame frameType = WS_INCOMPLETE_FRAME; readedLength = 0; memset(gBuffer, 0, BUF_LEN);


typedef struct {
    char *clientIP;
    int clientSocket, clientPort;
} s_param;

//-------------------------------------------+


/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN  6

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_NS   0x100
        #define TH_RS   0xE00
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_PUSH|TH_ACK|TH_URG|TH_ECE|TH_CWR|TH_NS|TH_RS)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
};

void
got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

void
print_payload(const u_char *payload, int len);

void
print_hex_ascii_line(const u_char *payload, int len, int offset);


/*
 * print data in rows of 16 bytes: offset   hex   ascii
 *
 * 00000   47 45 54 20 2f 20 48 54  54 50 2f 31 2e 31 0d 0a   GET / HTTP/1.1..
 */
void
print_hex_ascii_line(const u_char *payload, int len, int offset)
{

  int i;
  int gap;
  const u_char *ch;

  /* offset */
  printf("%05d   ", offset);
  
  /* hex */
  ch = payload;
  for(i = 0; i < len; i++) {
    printf("%02x ", *ch);
    ch++;
    /* print extra space after 8th byte for visual aid */
    if (i == 7)
      printf(" ");
  }
  /* print space to handle line less than 8 bytes */
  if (len < 8)
    printf(" ");
  
  /* fill hex gap with spaces if not full line */
  if (len < 16) {
    gap = 16 - len;
    for (i = 0; i < gap; i++) {
      printf("   ");
    }
  }
  printf("   ");
  
  /* ascii (if printable) */
  ch = payload;
  for(i = 0; i < len; i++) {
    if (isprint(*ch))
      printf("%c", *ch);
    else
      printf(".");
    ch++;
  }

  printf("\n");

return;
}

/*
 * print packet payload data (avoid printing binary data)
 */
void
print_payload(const u_char *payload, int len)
{

  int len_rem = len;
  int line_width = 32;      /* number of bytes per line */
  int line_len;
  int offset = 0;         /* zero-based offset counter */
  const u_char *ch = payload;

  if (len <= 0)
    return;

  /* data fits on one line */
  if (len <= line_width) {
    print_hex_ascii_line(ch, len, offset);
    return;
  }

  /* data spans multiple lines */
  for ( ;; ) {
    /* compute current line length */
    line_len = line_width % len_rem;
    /* print line */
    print_hex_ascii_line(ch, line_len, offset);
    /* compute total remaining */
    len_rem = len_rem - line_len;
    /* shift pointer to remaining bytes to print */
    ch = ch + line_len;
    /* add offset */
    offset = offset + line_width;
    /* check if we have line width chars or less */
    if (len_rem <= line_width) {
      /* print last line and get out */
      print_hex_ascii_line(ch, len_rem, offset);
      break;
    }
  }

return;
}


/*
 * dissect/print packet
 */
void
got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    static int count = 1;                   /* packet counter */

    char *answer = malloc(5000);
    if (answer == NULL) {
        printf("\nCan not allocate memory!\n");
        free(answer);
        return;
    }


    sprintf(answer, "{\"packet\": %d, ", count);

    count++;
    
    /* declare pointers to packet headers */
    const struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
    const struct sniff_ip *ip;              /* The IP header */
    const struct sniff_tcp *tcp;            /* The TCP header */
    const char *payload;                    /* Packet payload */

    int size_ip;
    int size_tcp;
    int size_payload;
    
    /* define ethernet header */
    ethernet = (struct sniff_ethernet*)(packet);
    
    /* define/compute ip header offset */
    ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip)*4;
    if (size_ip < 20) {
        printf("* Invalid IP header length: %u bytes\n", size_ip);
        free(answer);
        return;
    }

    /* define/compute tcp header offset */
    tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp)*4;
    if (size_tcp < 20) {
        printf("* Invalid TCP header length: %u bytes\n", size_tcp);
        free(answer);
        return;
    }

    /* define/compute tcp payload (segment) offset */
    payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
    size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);


    sprintf(answer + strlen(answer), "\"ip_src\": \"%s\", \"th_sport\": %d, ", inet_ntoa(ip->ip_src), ntohs(tcp->th_sport));
    sprintf(answer + strlen(answer), "\"ip_dst\": \"%s\", \"th_dport\": %d, \"protocol\": ", inet_ntoa(ip->ip_dst), ntohs(tcp->th_dport));

    /* determine protocol */    
    switch(ip->ip_p) {
        case IPPROTO_TCP:
          sprintf(answer + strlen(answer), "\"TCP\"");
          break;
        case IPPROTO_UDP:
          sprintf(answer + strlen(answer), "\"UDP\"");
          return;
        case IPPROTO_ICMP:
          sprintf(answer + strlen(answer), "\"ICMP\"");
          return;
        case IPPROTO_IP:
          sprintf(answer + strlen(answer), "\"IP\"");
          return;
        default:
          sprintf(answer + strlen(answer), "\"unknown\"");
          return;
      }
    
    /*
     *  OK, this packet is TCP.
     */
    
    
    sprintf(answer + strlen(answer), ", \"flags\": [");
    if (tcp->th_flags & TH_FIN)  sprintf(answer + strlen(answer), "\"FIN\",");
    if (tcp->th_flags & TH_SYN)  sprintf(answer + strlen(answer), "\"SYN\",");
    if (tcp->th_flags & TH_RST)  sprintf(answer + strlen(answer), "\"RST\",");
    if (tcp->th_flags & TH_PUSH) sprintf(answer + strlen(answer), "\"PSH\",");
    if (tcp->th_flags & TH_ACK)  sprintf(answer + strlen(answer), "\"ACK\",");
    if (tcp->th_flags & TH_URG)  sprintf(answer + strlen(answer), "\"URG\",");
    if (tcp->th_flags & TH_ECE)  sprintf(answer + strlen(answer), "\"ECE\",");
    if (tcp->th_flags & TH_CWR)  sprintf(answer + strlen(answer), "\"CWR\",");
    if (tcp->th_flags & TH_NS)   sprintf(answer + strlen(answer), "\"NS\",");
    if (tcp->th_flags & TH_RS)   sprintf(answer + strlen(answer), "\"RS\",");
    answer[strlen(answer)-1] = 0;


    sprintf(answer + strlen(answer), "], \"th_seq\": %u, \"th_ack\": %u, \"th_win\": %u, \"th_sum\": %u, \"ip_sum\": %u, \"th_urp\": %u",
        ntohl(tcp->th_seq), ntohl(tcp->th_ack), tcp->th_win, tcp->th_sum, ip->ip_sum, tcp->th_urp);
    
    /*
     * Print payload data; it might be binary, so don't just
     * treat it as a string.
     */
    //if (size_payload > 0) {
        // payload in bytes
        sprintf(answer + strlen(answer), ", \"payload\": %d", size_payload);

        /*int i;
        for (i=0; i<size_payload; i++) { 
          if ( isprint(payload[i]) )
            printf("%c ", payload[i]); 
          else 
            printf(". "); 

          if(i==size_payload-1 ) 
            printf("\n"); 
        }*/


        //print_payload(payload, size_payload);
    //}

    sprintf(answer + strlen(answer), "},");


    FILE *f;

    f = fopen("db.txt", "a");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        free(answer);
        return;
    }

    fprintf(f, "%s", answer);
    fclose(f);

    free(answer);



    struct timeval tv;
    gettimeofday(&tv, NULL);
    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

    if (count == 1) {
        last_send = time_in_mill;
        last_count = 1;
    }

    if (time_in_mill - last_send > 1000 || count - last_count > 200) {
        last_send = time_in_mill;
        last_count = count;

        struct stat st;
        stat("db.txt", &st);
        printf("\n\ndb.txt size: %llu\n", st.st_size);


        f = fopen("db.txt", "r");

        #define chunk 80000
        char *buf_answer = calloc(chunk, sizeof(char*));
        size_t nread;

        if (buf_answer == NULL) {
            printf("\nCan not allocate memory chunk!\n");
            free(buf_answer);
            return;
        }

        while ((nread = fread(buf_answer, 1, chunk, f)) > 0) {
            //fwrite(buf_answer, 1, nread, stdout);
        }

        fclose(f);


        /* clean file */
        f = fopen("db.txt", "w");
        fclose(f);

        uint8_t *recievedString = NULL;
        recievedString = malloc(strlen(buf_answer)+1);
        assert(recievedString);
        memcpy(recievedString, buf_answer, strlen(buf_answer));
        recievedString[ strlen(buf_answer) ] = 0;
        
        prepareBuffer;
        wsMakeFrame(recievedString, strlen(buf_answer), gBuffer, &frameSize, WS_TEXT_FRAME);
        free(recievedString);
        free(buf_answer);

        if (safeSend(args[0], gBuffer, frameSize) == EXIT_FAILURE) {
            printf("Error sending packet to client\n");
        }
    }

    initNewFrame;
    return;
}


//-----------------------------------------+

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int safeSend(int clientSocket, const uint8_t *buffer, size_t bufferSize)
{
    #ifdef PACKET_DUMP
    printf("\n\nout packet:\n");
    fwrite(buffer, 1, bufferSize, stdout);
    printf("\n");
    #endif
    ssize_t written = send(clientSocket, buffer, bufferSize, 0);
    if (written == -1) {
        close(clientSocket);
        perror("send failed");
        return EXIT_FAILURE;
    }
    if (written != bufferSize) {
        close(clientSocket);
        perror("written not all bytes");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void *clientWorker(void *socket_desc)
{
    s_param *param = socket_desc;
    int clientSocket = param->clientSocket;

    printf("Connected %s:%d\n", param->clientIP, param->clientPort);

    prepareBuffer;
    initNewFrame;

    memset(gBuffer, 0, BUF_LEN);
    enum wsState state = WS_STATE_OPENING;
    uint8_t *data = NULL;
    size_t dataSize = 0;
    struct handshake hs;
    nullHandshake(&hs);
    
    while (frameType == WS_INCOMPLETE_FRAME) {
        ssize_t readed = recv(clientSocket, gBuffer+readedLength, BUF_LEN-readedLength, 0);
        if (!readed) {
            close(clientSocket);
            perror("recv failed");
            return 0;
        }
        #ifdef PACKET_DUMP
        printf("in packet:\n");
        fwrite(gBuffer, 1, readed, stdout);
        printf("\n");
        #endif
        readedLength+= readed;
        assert(readedLength <= BUF_LEN);
        
        if (state == WS_STATE_OPENING) {
            frameType = wsParseHandshake(gBuffer, readedLength, &hs);
        } else {
            frameType = wsParseInputFrame(gBuffer, readedLength, &data, &dataSize);
        }
        
        if ((frameType == WS_INCOMPLETE_FRAME && readedLength == BUF_LEN) || frameType == WS_ERROR_FRAME) {
            if (frameType == WS_INCOMPLETE_FRAME)
                printf("buffer too small");
            else
                printf("error in incoming frame\n");
            
            if (state == WS_STATE_OPENING) {
                prepareBuffer;
                frameSize = sprintf((char *)gBuffer,
                                    "HTTP/1.1 400 Bad Request\r\n"
                                    "%s%s\r\n\r\n",
                                    versionField,
                                    version);
                safeSend(clientSocket, gBuffer, frameSize);
                break;
            } else {
                prepareBuffer;
                wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
                if (safeSend(clientSocket, gBuffer, frameSize) == EXIT_FAILURE)
                    break;
                state = WS_STATE_CLOSING;
                initNewFrame;
            }
        }
        
        if (state == WS_STATE_OPENING) {
            assert(frameType == WS_OPENING_FRAME);
            if (frameType == WS_OPENING_FRAME) {
                // if resource is right, generate answer handshake and send it
                if (strcmp(hs.resource, "/echo") != 0) {
                    frameSize = sprintf((char *)gBuffer, "HTTP/1.1 404 Not Found\r\n\r\n");
                    safeSend(clientSocket, gBuffer, frameSize);
                    break;
                }
                
                prepareBuffer;
                wsGetHandshakeAnswer(&hs, gBuffer, &frameSize);
                freeHandshake(&hs);
                if (safeSend(clientSocket, gBuffer, frameSize) == EXIT_FAILURE)
                    break;

                state = WS_STATE_NORMAL;
                initNewFrame;
                
                //-------------------------------------+


                char *dev = NULL;     /* capture device name */
                char errbuf[PCAP_ERRBUF_SIZE];    /* error buffer */
                pcap_t *handle;       /* packet capture handle */

                char filter_exp[50];   /* filter expression [3] */
                sprintf(filter_exp, "port not 22 and not (host %s and port %d)", param->clientIP, param->clientPort);

                struct bpf_program fp;      /* compiled filter program (expression) */
                bpf_u_int32 mask;     /* subnet mask */
                bpf_u_int32 net;      /* ip */

                /* find a capture device if not specified on command-line */
                dev = pcap_lookupdev(errbuf);
                if (dev == NULL) {
                    fprintf(stderr, "Couldn't find default device: %s\n",
                      errbuf);
                    exit(EXIT_FAILURE);
                }

                /* get network number and mask associated with capture device */
                if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
                    fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
                        dev, errbuf);
                    net = 0;
                    mask = 0;
                }

                /* print capture info */
                printf("Device: %s\n", dev);
                printf("Filter expression: %s\n", filter_exp);

                /* open capture device */
                handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
                if (handle == NULL) {
                    fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
                    exit(EXIT_FAILURE);
                }

                /* make sure we're capturing on an Ethernet device [2] */
                if (pcap_datalink(handle) != DLT_EN10MB) {
                    fprintf(stderr, "%s is not an Ethernet\n", dev);
                    exit(EXIT_FAILURE);
                }

                /* compile the filter expression */
                if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
                    fprintf(stderr, "Couldn't parse filter %s: %s\n",
                        filter_exp, pcap_geterr(handle));
                    exit(EXIT_FAILURE);
                }

                /* apply the compiled filter */
                if (pcap_setfilter(handle, &fp) == -1) {
                    fprintf(stderr, "Couldn't install filter %s: %s\n",
                        filter_exp, pcap_geterr(handle));
                    exit(EXIT_FAILURE);
                }


                u_char ch[sizeof(int)];
                memcpy(&ch, &clientSocket, sizeof clientSocket);

                /* now we can set our callback function */
                if (pcap_loop(handle, -1, got_packet, ch) == -1) {
                    fprintf(stderr, "ERROR: %s\n", pcap_geterr(handle) );
                    exit(EXIT_FAILURE);
                }

                printf("\n\nI am here\n\n");

                /* cleanup */
                pcap_freecode(&fp);
                pcap_close(handle);

                //-------------------------------------+

            }
        } else {
            if (frameType == WS_CLOSING_FRAME) {
                if (state == WS_STATE_CLOSING) {
                    break;
                } else {
                    prepareBuffer;
                    wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
                    safeSend(clientSocket, gBuffer, frameSize);
                    printf("disconnected\n");
                    break;
                }
            } else if (frameType == WS_TEXT_FRAME) {
                uint8_t *recievedString = NULL;
                recievedString = malloc(dataSize+1);
                assert(recievedString);
                memcpy(recievedString, data, dataSize);
                recievedString[ dataSize ] = 0;
                
                prepareBuffer;
                wsMakeFrame(recievedString, dataSize, gBuffer, &frameSize, WS_TEXT_FRAME);
                free(recievedString);
                if (safeSend(clientSocket, gBuffer, frameSize) == EXIT_FAILURE)
                    break;
                initNewFrame;
            }
        }
    } // read/write cycle
    
    printf("\n\nI am here 2\n\n");
    close(clientSocket);
}

int main(int argc, char** argv)
{
    /* clean file */
    FILE *f = fopen("db.txt", "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        return 0;
    }
    fclose(f);

    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        error("create socket failed");
    }

    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    // local.sin_addr.s_addr = inet_addr("127.0.0.1");
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(atoi(argv[1]));
    if (bind(listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1) {
        error("bind failed");
    }
    
    if (listen(listenSocket, 5) == -1) {
        error("listen failed");
    }
    printf("opened %s:%d\n", inet_ntoa(local.sin_addr), ntohs(local.sin_port));
    
    while (TRUE) {
        struct sockaddr_in remote;
        socklen_t sockaddrLen = sizeof(remote);
        int clientSocket = accept(listenSocket, (struct sockaddr*)&remote, &sockaddrLen);
        if (clientSocket == -1) {
            error("accept failed");
        }

        pthread_t sniffer_thread;

        s_param param;
        param.clientSocket = clientSocket;
        param.clientIP = inet_ntoa(remote.sin_addr);
        param.clientPort = ntohs(remote.sin_port);

        if( pthread_create( &sniffer_thread , NULL ,  clientWorker , &param) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    }
    
    close(listenSocket);
    printf("\n\nCLOSED\n\n");
    return EXIT_SUCCESS;
}