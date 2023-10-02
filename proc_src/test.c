//
// Created by KimByoungGook on 2021-06-16.
//

#include <stdio.h>
#include <sys/errno.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

typedef struct
{
    char UseFlag; /*  DF_NI_CONNTRACK_FLAG_INIT      0x00  DF_NI_CONNTRACK_FLAG_INSERT    0x01  DF_NI_CONNTRACK_FLAG_USE       0x02 */

    char szSrcIp[16 +1];
    unsigned int nSrcIp; // 검색용

    char szDestIp[16 +1];
    unsigned int nDestIp; // 검색용

    int nSrcPort; //검색용
    int nDestPort;
    int nClientDestPort;//검색용

    char ProtocolType; /* 0x01 : tcp , 0x02 : udp */
    time_t EnqueueTime;

}_PROXY_CONNTRACK_INFO;

int futil_StrToInt(char* param_str)
{
    int   local_nRetValue = 0;
    char* local_ptrStrEnd = NULL;

    local_nRetValue = (int)strtol(param_str, &local_ptrStrEnd, 10);

    return local_nRetValue;
}


void fproxy_ConntrackParsing(char* param_Buffer, _PROXY_CONNTRACK_INFO* param_PtrProxyTrackInfo)
{
    char  local_buffer[1024 +1] = {0x00,};
    char* local_ptrTok          = NULL;
    char* local_ptrRemain       = NULL;


    snprintf(local_buffer, sizeof(local_buffer), "%s", param_Buffer);

    local_ptrTok = strtok_r(local_buffer, " ", &local_ptrRemain);

    while( (local_ptrTok = strtok_r(NULL," ", &local_ptrRemain) ) != NULL)
    {
        if(memcmp(local_ptrTok, "tcp", 3) == 0)
        {
            param_PtrProxyTrackInfo->ProtocolType = 0x01;
        }

        if(memcmp(local_ptrTok, "udp", 3) == 0)
        {
            param_PtrProxyTrackInfo->ProtocolType = 0x02;
        }

        if(param_PtrProxyTrackInfo->szSrcIp[0] == 0x00 &&
           memcmp(local_ptrTok, "src=", 4) == 0)
        {
            snprintf(param_PtrProxyTrackInfo->szSrcIp,
                     sizeof(param_PtrProxyTrackInfo->szSrcIp),
                     "%s", local_ptrTok + 4);
        }

        if(param_PtrProxyTrackInfo->szDestIp[0] == 0x00 &&
           memcmp(local_ptrTok, "dst=", 4) == 0)
        {
            snprintf(param_PtrProxyTrackInfo->szDestIp,
                     sizeof(param_PtrProxyTrackInfo->szDestIp),
                     "%s", local_ptrTok + 4);
        }

        if(param_PtrProxyTrackInfo->nSrcPort == 0 &&
            memcmp(local_ptrTok, "sport=", 6) == 0)
        {
            if ( (param_PtrProxyTrackInfo->nSrcPort = futil_StrToInt(local_ptrTok + 6)) < 0)
            {
                printf("Convert Fail (%s)", local_ptrTok +6);
                return;
            }
        }

        if ( param_PtrProxyTrackInfo->nDestPort != 0 )
        {
            if(memcmp(local_ptrTok, "dport=", 6) == 0)
            {
                if( (param_PtrProxyTrackInfo->nClientDestPort = futil_StrToInt(local_ptrTok + 6)) < 0)
                {
                    printf("Convert Fail (%s)", local_ptrTok +6);
                    return;
                }
            }
        }

        if(param_PtrProxyTrackInfo->nDestPort == 0 &&
            memcmp(local_ptrTok, "dport=", 6) == 0)
        {
            if( (param_PtrProxyTrackInfo->nDestPort = futil_StrToInt(local_ptrTok + 6)) < 0)
            {
                printf("Convert Fail (%s)", local_ptrTok +6);
                return;
            }
        }


        if(param_PtrProxyTrackInfo->nDestPort != 0 &&
           param_PtrProxyTrackInfo->nSrcPort != 0 &&
           param_PtrProxyTrackInfo->nClientDestPort != 0 &&
           param_PtrProxyTrackInfo->szSrcIp[0] != 0x00 &&
           param_PtrProxyTrackInfo->szDestIp[0] != 0x00)
        {
            break;
        }
    }

    printf ("(%s) "
           "Parse Src : [%s], Dest : [%s], sport : [%d], dport : [%d] clientdport : [%d]\n",
           (param_PtrProxyTrackInfo->ProtocolType == 0x01) ? "tcp" : "udp",
           param_PtrProxyTrackInfo->szSrcIp,
           param_PtrProxyTrackInfo->szDestIp,
           param_PtrProxyTrackInfo->nSrcPort,
           param_PtrProxyTrackInfo->nDestPort,
           param_PtrProxyTrackInfo->nClientDestPort);

    return;
}

int main(void)
{
    char buf[1023 +1] = {0x00,};
    _PROXY_CONNTRACK_INFO  local_stTrackInfo;

    memset(&local_stTrackInfo, 0x00, sizeof(local_stTrackInfo));

    snprintf(buf, sizeof(buf),
             "%s",
             " [UPDATE] tcp      6 432000 ESTABLISHED src=10.20.20.92 dst=18.177.83.12 sport=1034 dport=443 src=10.20.20.96 dst=10.20.20.92 sport=50302 dport=7459");
//     "[UPDATE] tcp      6 432000 ESTABLISHED src=10.20.20.96 dst=208.100.17.187 sport=40396 dport=443 src=208.100.17.187 dst=10.20.20.96 sport=443 dport=40396 [ASSURED]");


    fproxy_ConntrackParsing(buf,&local_stTrackInfo);


    return;
}
