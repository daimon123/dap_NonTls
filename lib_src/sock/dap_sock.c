//
// Created by KimByoungGook on 2020-06-30.
//

#include    <stdio.h>
#include    <fcntl.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <string.h>
#include    <time.h>
#include    <sys/time.h>
#include    <sys/types.h>
#include    <sys/sysinfo.h>
#include    <sys/wait.h>
#include    <sys/ioctl.h>
#include    <errno.h>
#include    <ifaddrs.h>
#include    <signal.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <limits.h>
#include    <netdb.h>
#include    <arpa/inet.h>
#include    <net/if_arp.h>
#include    <linux/wireless.h>
#include    <math.h>
#include    <netinet/ether.h>

#include "com/dap_com.h"
#include "sock/dap_sock.h"



static char* fstEthernetMacToa(struct sockaddr *addr);

static char* fstEthernetMacToa(struct sockaddr *addr)
{
    static char buff[256];

    unsigned char *ptr = (unsigned char *) addr->sa_data;

    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
            (ptr[0] & 0xff), (ptr[1] & 0xff), (ptr[2] & 0xff),
            (ptr[3] & 0xff), (ptr[4] & 0xff), (ptr[5] & 0xff));

    return (buff);

}


void fsock_convrtMac(const char* param_Data, char* param_CovrtStr, int param_len)
{
    int local_nTemp=0;
    char local_szBuf[128] = {0x00,};
    char local_szTBuf[8] = {0x00,};
    char* local_TokPtr = strtok( (char *)param_Data, ":" );

    do
    {
        memset( local_szTBuf, 0x00, sizeof(local_szTBuf) );

        sscanf( local_TokPtr, "%x", &local_nTemp );
        snprintf( local_szTBuf, sizeof(local_szTBuf)-1, "%02X", local_nTemp );
        strncat( local_szBuf, local_szTBuf, sizeof(local_szBuf)-1 );
        strncat( local_szBuf, ":", sizeof(local_szBuf)-1 );

    } while( (local_TokPtr = strtok( NULL , ":" )) != NULL );

    local_szBuf[strlen(local_szBuf) -1] = '\0';
    strncpy( param_CovrtStr, local_szBuf, param_len );

    return;
}


int fsock_GetNic(char* param_NicName)
{
    int local_nSock = 0;
    int local_nIfs = 0;
    int local_nLoop = 0;
    struct ifconf stIfconf;
    struct ifreq  stIfr[50];

    local_nSock = socket(AF_INET, SOCK_STREAM, 0);
    if (local_nSock < 0)
    {
        return 0;
    }

    stIfconf.ifc_buf = (char *) stIfr;
    stIfconf.ifc_len = sizeof( stIfr );

    if (ioctl(local_nSock, SIOCGIFCONF, &stIfconf) == -1)
    {
        return 0;
    }

    local_nIfs = stIfconf.ifc_len / sizeof(stIfr[0]);

    for (local_nLoop = 0; local_nLoop < local_nIfs; local_nLoop++)
    {
        if(strcmp(stIfr[local_nLoop].ifr_name, "lo") != 0)
        {
            strcpy(param_NicName, stIfr[local_nLoop].ifr_name);
            break;
        }
    }

    if( local_nSock > 0)
    {
        close(local_nSock);
        local_nSock = 0;
    }

    return 1;

}

int fsock_GetIpAddress(char* param_Nic, char* param_Ipaddr)
{
    int local_nTmpSock = 0;
    struct ifreq stIfr;

    local_nTmpSock = socket(AF_INET, SOCK_STREAM, 0);
    strcpy(stIfr.ifr_name, param_Nic);

//    if (ioctl(local_nTmpSock, SIOCGIFHWADDR, &stIfr)< 0)  // 실제 IP Address
    if (ioctl(local_nTmpSock, SIOCGIFADDR, &stIfr)< 0)    // Private IP Address
    {
        close(local_nTmpSock);
        return (-1);
    }

    inet_ntop(AF_INET, stIfr.ifr_addr.sa_data+2, param_Ipaddr, sizeof(struct sockaddr));
    close(local_nTmpSock);

    return 0;

}

int fsock_getMacAddress(char* param_Nic, char* param_Mac)
{
    int local_nSock = 0;
    struct ifreq stIfr;
    char local_szMacAddr[18] = {0,};

    local_nSock = socket(AF_INET, SOCK_STREAM, 0);
    if (local_nSock < 0)
    {
        printf("Socket Create Failed ");
        return 0;
    }

    strcpy(stIfr.ifr_name, param_Nic);

    if (ioctl(local_nSock, SIOCGIFHWADDR, &stIfr)< 0)
    {
        if(local_nSock > 0)
        {
            close(local_nSock);
            local_nSock = 0;
        }
        return 0;
    }

    fsock_convrtMac( ether_ntoa((struct ether_addr *)(stIfr.ifr_hwaddr.sa_data)),
                   local_szMacAddr,sizeof(local_szMacAddr) -1 );

    strcpy(param_Mac, local_szMacAddr);

    if( local_nSock > 0)
    {
        close(local_nSock);
        local_nSock = 0;
    }


    return 1;

}

int fsock_GetPeerAddr(int fd, char *raddr)
{
    int rcvleng;

    unsigned char *addr;
    struct sockaddr_in paddr;

    rcvleng =   sizeof(paddr);
    getpeername(fd,(struct sockaddr *) &paddr, &rcvleng);
/*    memcpy(clientIP, &paddr.sin_addr, 4);
*/

    addr = (unsigned char *)&paddr.sin_addr;

    sprintf(raddr, "%d.%d.%d.%d",
            addr[0], addr[1], addr[2],addr[3]);

    return 1;
}

int fsock_GetMacAddress(char *cpip, char *device, char *res)
{
    struct ifaddrs *ifaddr, *ifa;

    if(device == NULL)
    {
        if (getifaddrs(&ifaddr) == -1)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in getifaddrs ");
            return -1;
        }

        /* Walk through linked list, maintaining head pointer so we
            can free list later */
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            char protocol[IFNAMSIZ]  = {0};

            if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET)
                continue;

            if (fsock_CheckWireless(ifa->ifa_name, protocol))
            {
                WRITE_INFO(CATEGORY_INFO,"Interface %s is wireless: %s",
                       ifa->ifa_name,protocol);
            }
            else
            {
                if(fsock_GetClientMacAddress(cpip, ifa->ifa_name, res) == 0)
                    break;
            }
        }

        freeifaddrs(ifaddr);
    }
    else
    {
        fsock_GetClientMacAddress(cpip, device, res);
    }

    return 0;
}

int fsock_GetClientMacAddress(char *ip, char *device, char *res)
{
    int                 s;
    struct arpreq       areq;
    struct sockaddr_in *sin;
    struct in_addr      ipaddr;

    /* Get an internet domain socket.
    */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        //perror("socket");
        //exit(1);
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket ");
        return -1;
    }

    /* Make the ARP request.
    */
    memset(&areq, 0, sizeof(areq));
    sin = (struct sockaddr_in *) &areq.arp_pa;
    sin->sin_family = AF_INET;

    if (inet_aton(ip, &ipaddr) == 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in bad dotted-decimal IP '%s'", ip);
        return -1;
    }

    sin->sin_addr = ipaddr;
    sin = (struct sockaddr_in *) &areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;

    strncpy(areq.arp_dev, device, 15);

    if (ioctl(s, SIOCGARP, (caddr_t) &areq) == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Unable to make ARP request ");
        return -1;
    }

    strcpy(res, fstEthernetMacToa(&areq.arp_ha));

    return 0;
}


int fsock_CheckWireless(const char* ifname, char* protocol)
{
    int sock = -1;
    struct iwreq pwrq;
    memset(&pwrq, 0, sizeof(pwrq));
    strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        //perror("socket");
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in socket ");
        return 0;
    }

    if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1)
    {
        if (protocol)
            strncpy(protocol, pwrq.u.name, IFNAMSIZ);
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}



/*---------------------------------------------------------------------
 *
 *  FUNCTION NAME   : makeSocket
 *  PURPOSE         : socket library socket-bind-listen function
 *  INPUT ARGUMENTS : nSocketType, usPort, nListener, RxtErr
 *  RETURN VALUE    : none
 *  GLOBAL VARIABLES
 *    1) EXTERNAL   : none
 *    2) LOCAL      : none
 *
----------------------------------------------------------------------*/

int fsock_MakeSocket(int nSocketType, u_short usPort, int *nListener, char* cpResult)
{
    struct sockaddr_in	stAddress;
    int					nListeningSocket;
    int					nReuseAddr = 1;
    int					RxtVal;
    int					nSockBuf = 417180;

    /* Setup internet address information.
        This is used with the bind() call */
    memset((char *) &stAddress, 0, sizeof(stAddress));
    stAddress.sin_family = AF_INET;
    stAddress.sin_port = usPort;


    stAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    nListeningSocket = socket(AF_INET, nSocketType, 0);
    if (nListeningSocket < 0)
    {
        strcpy(cpResult, "makeSocket-socket fail");
        WRITE_CRITICAL(CATEGORY_DEBUG,"makeSocket fail ")
        return -1;
    }

    if (nListener != NULL)
        *nListener = nListeningSocket;

    setsockopt(nListeningSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nReuseAddr, sizeof(nReuseAddr));

    setsockopt(nListeningSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nSockBuf, sizeof(nSockBuf));
    setsockopt(nListeningSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nSockBuf, sizeof(nSockBuf));

    RxtVal = bind(nListeningSocket, (struct sockaddr *) &stAddress, sizeof(stAddress));
    if (RxtVal < 0)
    {
        close(nListeningSocket);
        strcpy(cpResult, "makeSocket-bind fail");
        WRITE_CRITICAL(CATEGORY_DEBUG,"makeSocket-bind fail (%d) (%s) ",errno,strerror(errno))
        return -1;
    }

    listen(nListeningSocket, 10000); /* Queue up to five connections before
		                          having them automatically rejected. */
    return nListeningSocket;
}

/**SOCKET CREATION: it returns file descriptor for the socket**/
int fsock_Socket(int family,int type,int protocol)
{
    int tmp;

    if((tmp=socket(family,type,protocol))<0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error Socket creation: %s ",strerror(errno));
    }

    return(tmp);
}

/**BIND: it assigns a local address to a socket**/
int fsock_Bind(int socket,const SA *address,socklen_t addrlen)
{
    int tmp;

    if((tmp=bind(socket,address,addrlen))!=0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Bind: %s. ", strerror(errno));
    }

    return(tmp);
}

/**LISTEN: set a socket in passive mode, in order to listen to a connection**/
int fsock_Listen(int socket,int backlog)
{
    char *ptr;
    int tmp;

    if((ptr=getenv("LISTENQ"))!= NULL)
    {
        backlog=atoi(ptr);
    }

    if((tmp=listen(socket,backlog))<0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Listen: %s.", strerror(errno));
    }

    return(tmp);
}

/**ACCEPT: it manages a new connection after the end of three-way handshake**/
int fsock_Accept(int socket)
{
    int connectedSocket;
    int repeat=0;
    do
    {

        connectedSocket=accept(socket,NULL,NULL);
        if(connectedSocket<0)
        {
            //check if there are signals
            if(errno == EINTR || errno == ECHILD)
            {
                repeat=1;
            }
            else
            {
                repeat=1;
                //printf the error condition
                WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Accept: %s.", strerror(errno));
            }
        }
        else
        {
            repeat=0;
        }
    }while(repeat==1);

    return(connectedSocket);
}

int fsock_Select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset, int thr_flag)
{
    int repeat = 0;
    int tmp = 0;
    int Checkcnt = 0;
    struct timeval timeout;

    timeout.tv_usec = 0;
    timeout.tv_sec = 1;

    do
    {
        /* 0x02일 경우 프로세스 Stop 시그널의 경우라 종료한다. */
        if(thr_flag == 1)
        {
            return RET_SUCC;
        }
        tmp = select(maxfdp1, readset, writeset, exceptset, &timeout);
        if(tmp < 0)
        {
            //check if there are signals
            if(errno == EINTR || errno == ECHILD)
            {
                repeat=1;
            }
            else
            {
                if(Checkcnt > 10)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"Select Is Break (%d)", tmp);
                    break;
                }
                WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Select: %s. ", strerror(errno));
                Checkcnt++;
            }
        }
        else
        {
            Checkcnt++;
            if(Checkcnt > 10)
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"Select Is Break (%d)", tmp);
                break;
            }
            repeat=0;
        }
        sleep(0);
    }while(repeat==1);

    return(tmp);
}



/**RECV: it tries to read from the socket**/
int fsock_Recv(int socket,void *buffer,int len,int flag)
{
    int tmp = 0;
    int		local_max_sec = 0;
    struct		timeval		 local_tval;

    /*--------- Server Socket SELECT 을 위한 FD ----------*/
    fd_set		local_rset, local_rset_back;
    FD_ZERO( &local_rset );


    FD_SET( socket , &local_rset );
    local_rset_back = local_rset;

    while(1)
    {
        local_tval.tv_sec  = 5;
        local_tval.tv_usec = 0;

        local_rset = local_rset_back;

        if( select(  socket+1  , &local_rset , NULL , NULL , &local_tval ) > 0 )
        {
            local_max_sec = 0; /* 시간 초기화 */

            if ( FD_ISSET ( socket , &local_rset ) )
            {
                tmp = recv(socket,buffer,len,flag);
                if(tmp<0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Recv: %s. ", strerror(errno));
                }
                else
                {
                    /* 가끔 DAP_Manager가 0Byte를 보내서 0 Byte보낼경우 Success Return 처리 */
                    return tmp;
                }

            }
        }
        else
        {
            local_max_sec++;
            if(local_max_sec > 10)
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"Server Socket Select Timeout max sec %d ",local_max_sec);
                break;
            }
        }

        usleep(1000);
    }

    return tmp;

}

int fsock_SendSSL(int socket,void *buffer,int len, int *epipeError)
{
    int tmp;

    tmp=send(socket,buffer,len,0);
    *epipeError=0;
    if(tmp<0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Send: %s. ", strerror(errno));
        if(errno==EPIPE)
        {
            *epipeError=1;
            WRITE_INFO(CATEGORY_INFO,"EPIPE.");
        }
    }

    return(tmp);
}

/**SEND: it tries to send data to the socket**/
int fsock_Send(int socket,void *buffer,int len)
{
    int tmp;

    tmp=send(socket,buffer,len,0);
    if(tmp<0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Send: %s. ", strerror(errno));
        if(errno==EPIPE)
        {
            WRITE_INFO(CATEGORY_INFO,"EPIPE.");
        }
    }

    return(tmp);
}

/**CONNECT: it establishes a new connection with a server**/
int fsock_Connect(int socket,const SA *serverAddress,socklen_t addrlen)
{
    int tmp;

    if((tmp=connect(socket,serverAddress,addrlen))!=0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error in Connect: %s. ", strerror(errno));
    }

    return(tmp);
}


/**CLOSE: set a socket as closed*/
int fsock_Close(int socket)
{
    int tmp;

    if(socket > 0)
    {
        if ((tmp=close(socket))!= 0)
            return(tmp);
    }


    return RET_SUCC;
}


int fsock_SetNonblock(int sfd)
{
    if (fcntl(sfd, F_SETFL,
              fcntl(sfd, F_GETFD, 0) | O_NONBLOCK) == -1)
        return -1;

    return 0;
}

/** AS-IS **/
//unsigned int fsock_Ip2Ui(char *ip)
//{
//    long ipAsUInt = 0;
//    char tmpIp[15+1] = {0x00,};
//    char *cPtr = NULL;
//    char *TempPtr = NULL;
//
//    memset(tmpIp, 0x00, sizeof(tmpIp));
//    strcpy(tmpIp, ip);
//
//    cPtr = strtok_r(tmpIp, ".", &TempPtr);
//    if(cPtr) ipAsUInt += atoi(cPtr) * pow(256, 3);
//
//    int exponent = 2;
//    while(cPtr && exponent >= 0)
//    {
//        cPtr = strtok_r(NULL, ".\0",&TempPtr);
//        if(cPtr) ipAsUInt += atoi(cPtr) * pow(256, exponent--);
//    }
//
//    return ipAsUInt;
//}

unsigned int fsock_Ip2Ui(char* param_Ip)
{
    unsigned long local_nRet = 0;
    unsigned long local_nAddr = 0;

    local_nAddr = inet_addr(param_Ip);
    local_nRet = ntohl(local_nAddr);

    return local_nRet;
}


char* fsock_Ui2Ip(unsigned int ipAsUInt)
{
    int exponent = 0;
    char* ip = NULL;
    if(fcom_malloc((void**)&ip, 16*sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return NULL;
    }

    for(exponent = 3; exponent >= 0; --exponent)
    {
        int r = ipAsUInt / pow(256, exponent);
        char buf[4];
        sprintf(buf, "%d", r);
        strcat(ip, buf);
        strcat(ip, ".");
        ipAsUInt -= r*pow(256, exponent);
    }
    ip[strlen(ip)-1] = 0;
    return ip;
}

int fsock_CidrToIpAndMask(const char *cidr, uint32_t *ip, uint32_t *mask)
{
    uint8_t a, b, c, d, bits;

    if(sscanf(cidr, "%hhu.%hhu.%hhu.%hhu/%hhu", &a, &b, &c, &d, &bits) < 5)
    {
        return -1;
    }
    if (bits > 32)
    {
        return -1;
    }
    *ip =
            (a << 24UL) |
            (b << 16UL) |
            (c << 8UL) |
            (d);
    *mask = (0xFFFFFFFFUL << (32 - bits)) & 0xFFFFFFFFUL;

    return 0;
}

int fsock_CompareIpRange(char *p_userIp, char *p_arrTokenIp)
{
    unsigned long   ul_userIp;
    unsigned long   ul_tokenIp1;
    unsigned long   ul_tokenIp2;
    char            tokenIp1[15+1];
    char            tokenIp2[15+1];

    memset(tokenIp1, 0x00, sizeof(tokenIp1));
    memset(tokenIp2, 0x00, sizeof(tokenIp2));

    ul_userIp = fsock_Ip2Ui(p_userIp);

    if(strstr(p_arrTokenIp, "/") != NULL) // CIDR
    {
        uint32_t ip;
        uint32_t mask;

        if (fsock_CidrToIpAndMask(p_arrTokenIp, &ip, &mask) < 0)
        {
            return -1;
        }
        ul_tokenIp1 = ip & mask;
        ul_tokenIp2 = ul_tokenIp1 | ~mask;
    }
    else
    {
        fcom_Str2ValToken(p_arrTokenIp, ",", tokenIp1, tokenIp2);

        ul_tokenIp1 = fsock_Ip2Ui(tokenIp1);
        ul_tokenIp2 = fsock_Ip2Ui(tokenIp2);
    }


    if(ul_userIp >= ul_tokenIp1 && ul_userIp <= ul_tokenIp2)
    {
        //LogDRet(5, "IP OK!!!\n");
        return 1;
    }
    else if(ul_userIp <= ul_tokenIp1 && ul_userIp >= ul_tokenIp2)
    {
        //LogDRet(5, "IP OK!!!\n");
        return 2;
    }
    else
    {
        //LogDRet(5, "IP Fail!!!\n");
        return 0;
    }
}

/*---------------------------------------------------------------------
 *
 *  FUNCTION NAME   : atoPort
 *  PURPOSE         : socket library port convert function
 *  INPUT ARGUMENTS : cpService, cpProto
 *  RETURN VALUE    : port
 *  GLOBAL VARIABLES
 *    1) EXTERNAL   : none
 *    2) LOCAL      : none
 *
----------------------------------------------------------------------*/

int fsock_atoPort(char *cpService, char *cpProto)
{
    int				nPort;
    long int		lnPort;
    struct servent	*stServ;
    char			*cpErrpos;

    /* First try to read it from /etc/services */
    stServ = getservbyname(cpService, cpProto);
    if (stServ != NULL)
    {
        nPort = stServ->s_port;
    }
    else  /* Not in services, maybe a number? */
    {
        lnPort = strtol(cpService, &cpErrpos,0);

        if ( (cpErrpos[0] != 0) || (lnPort < 1) || (lnPort > 65535) )
            return -1; /* Invalid port address */

        nPort = htons(lnPort);
    }

    return nPort;
}

void fsock_InetNtop(int af,const void *addrptr,char *strptr,size_t length)
{
    if(inet_ntop(af, addrptr, strptr, length)==NULL)
    {
        printf("Error in inet_ntop: %s.\n",strerror(errno));
        exit((-1));
    }
}

void fsock_InetPton(int af,const char *strptr,void *addrptr)
{
    int status;

    status=inet_pton(af, strptr, addrptr);
    if(status<=0)
    {
        printf("Error in inet_pton: %s.\n",strerror(errno));
        exit(-1);
    }
    return;
}




