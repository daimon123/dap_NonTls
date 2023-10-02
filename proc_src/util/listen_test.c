//
// Created by KimByoungGook on 2021-11-29.
//

// 프로토콜별 Listen 후 수신한 버퍼를 출력해주는 프로그램


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/timeb.h>
#include <iconv.h>

#include "com/dap_com.h"
#include "com/dap_def.h"
#include "sock/dap_sock.h"


int main(int argc, char** argv)
{
    int  local_nListenSock          = 0;
    int  local_nClientSock          = 0;
    int  local_nListenPort          = 0;
    char local_szProtocol[10]       = {0x00,};
    char local_szRecvBuffer[1024]   = {0x00,};
    struct sockaddr_in local_stSockAddr;

    memset( &local_stSockAddr, 0x00, sizeof(local_stSockAddr) );

    if ( argc < 3 )
    {
        printf("%s TCP/UDP Port \n", argv[0]);
        exit(0);
    }

    snprintf(local_szProtocol, sizeof(local_szProtocol), "%s", argv[1]);
    local_nListenPort = atoi(argv[2]);

    if ( strcmp( local_szProtocol, "TCP") != 0 && strcmp(local_szProtocol,"UDP") != 0 )
    {
        printf("Usage %s TCP/UDP Port \n", argv[0]);
        exit(0);
    }

    if ( strcmp(local_szProtocol, "TCP") == 0 )
    {
        local_nListenSock = fsock_Socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(local_nListenSock < 0)
        {
            exit((-1));
        }

        local_stSockAddr.sin_family      =   PF_INET;
        local_stSockAddr.sin_addr.s_addr =   htonl(INADDR_ANY);
        local_stSockAddr.sin_port        =   htons(local_nListenPort);

        // Bind
        if(fsock_Bind(local_nListenSock,(struct sockaddr*)&local_stSockAddr,sizeof(local_stSockAddr)) != 0)
        {
            exit((-1));
        }

        // Listen
        if(fsock_Listen(local_nListenSock, 100) < 0)
        {
            exit((-1));
        }

        local_nClientSock = fsock_Accept(local_nListenSock);
        if(local_nClientSock < 0)
        {
            printf("Client Socket Accept Failed \n");
            exit(0);
        }

        recv(local_nClientSock, local_szRecvBuffer, sizeof(local_szRecvBuffer), 0);

        printf("Recv Buffer : [%s] \n", local_szRecvBuffer);

    }

    else if ( strcmp(local_szProtocol, "UDP") == 0)
    {
        local_nListenSock = fsock_Socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(local_nListenSock < 0)
        {
            exit((-1));
        }

        local_stSockAddr.sin_family      =   PF_INET;
        local_stSockAddr.sin_addr.s_addr =   htonl(INADDR_ANY);
        local_stSockAddr.sin_port        =   htons(local_nListenPort);

        // Bind
        if(fsock_Bind(local_nListenSock,(struct sockaddr*)&local_stSockAddr,sizeof(local_stSockAddr)) != 0)
        {
            exit((-1));
        }


        while(1)
        {
            recv(local_nListenSock,local_szRecvBuffer, sizeof(local_szRecvBuffer), 0);

            printf("Recv Buffer : [%s] \n",local_szRecvBuffer);
        }

    }
    else
    {
        printf("Usage %s TCP/UDP Port \n", argv[0]);
        exit(0);
    }


    return 0;

}

