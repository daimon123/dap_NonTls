//
// Created by KimByoungGook on 2020-10-23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include "util.h"
#include "com/dap_def.h"
#include "com/dap_com.h"
#include "secure/dap_secure.h"
#include "sock/dap_sock.h"

static void fst_DisplayVersion(void);
static int fst_Init(void);
static int fst_ParseCmdOption(int argc, char **argv);
static int fst_MainStressTask(void);
static int fst_ForkTask(void);
void* fst_ThreadTask(void* argv);
static int fst_SslConnectionTest(char *addr, int port, int ThraedNum);
int fst_getMacAddress(void);
void fst_convrtMac(const char *data, char *cvrt_str, int sz);
static int fst_TcpConnectionTest(char *addr, int port,int* RefSocket);
static int fst_getNIC(); //eth0�� ���� NIC�� �����´�.
static int fst_getIPAddress();
void* fst_ThreadStep(void* ThreadArg);


//int* g_ThreadStep;
//int* g_ThreadFlag;

int main(int argc, char** argv)
{
    int nRet = 0;

    fst_Init();

    /* DAP Process Log Init */
    fcom_LogInit("dap_stress");

    nRet = fst_ParseCmdOption(argc, argv);

    printf("SSL Version : [%s] \n",  SSLeay_version(SSLEAY_VERSION));
    fflush(stdout);

    if(nRet != 0)
    {
        printf("DAP Stress Argument Func Failed (%d) \n",nRet);
        return 0;
    }
    else
    {
        fst_MainStressTask();
    }
    return 0;


}


/**SIGCHLD HANDLER**/
void futil_SigchldHandler(int sigNo)
{
    int 	pid = 0, cpId = 0;

    WRITE_DEBUG(CATEGORY_DEBUG,"Catch signo : [%d] ",sigNo);

    while((pid = waitpid(-1,NULL,WNOHANG)) > 0)
    {
       if((cpId = fork()) < 0)
        {
            continue;
        }
        else if(cpId > 0)
        {
            continue;
        }
        else if(cpId == 0) // child process
        {
            fst_ForkTask();
            exit(3);
            break;
        }
    }
}

static int fst_ParseCmdOption(int argc, char **argv)
{
    /* dap_stress fork thread ip */
    if (argc != 4)
    {
        printf("argc [%d] \n",argc);
        if(argc > 1)
        {
            if(strcmp(argv[1],"-h") == 0)
            {
                fst_DisplayVersion();
                return (-1);
            }
            else
            {
                printf("Invalid Argument \n");
                fst_DisplayVersion();
            }

        }
        else
        {
            fst_DisplayVersion();
            printf("Invalid Argument Help : Ex Help -> dap_stress -h \n");
            return (-1);
        }
    }
    else
    {
        g_nForkCnt = atoi(argv[1]);
        g_nThreadCnt = atoi(argv[2]);
        /* Server IP */
        sprintf(g_ConnSvrIp,"%s",argv[3]);

    }

    return 0;
}

/*********************************************************************
 *
 *	display program version
 *
 ********************************************************************/
static void fst_DisplayVersion()
{
    puts("------------------------------------------");
    puts("\n\nProgram Usage ForeGround \n\t Ex) dap_stress ProcessCnt ThreadCnt SvrIP");
    puts("------------------------------------------");
    puts("\n");

    return;
}

static int fst_Init(void)
{
    _DAP_COMN_INFO* pstComnInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome          ,
             sizeof(pstComnInfo->szDapHome  ),
             "%s"                     ,
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                          ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return FALSE;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);


    /* Cert Config */
//    fcom_GetProfile("SSL","CERT_PATH"   ,szCertPath  ,"/home/intent/dap/config/cert");
//    fcom_GetProfile("SSL","CA_FILE"     ,szCaFile    ,"root.crt");
//    fcom_GetProfile("SSL","CERT_FILE"   ,szCertFile  ,"server.crt");
//    fcom_GetProfile("SSL","KEY_FILE"    ,szKeyFile   ,"server.key");
    snprintf(szCertPath,sizeof(szCertPath),"%s/bin/util/certs",getenv("DAP_HOME"));
    snprintf(szCaFile,sizeof(szCaFile),"%s","dap_util.pem");
    snprintf(szCertFile,sizeof(szCertFile),"%s","dap_util.pem");
    snprintf(szKeyFile,sizeof(szKeyFile),"%s","dap_util_key.pem");

//    printf("Succeed in init, pid(%d) |%s\n", getpid(),__func__);

    return TRUE;

}




static int fst_MainStressTask(void)
{
    int nLoop = 0;
    char szFilePath[256 +1] = {0x00,};
    struct sigaction saDeadChild;

    memset(szFilePath, 0x00, sizeof(szFilePath));

    fst_getNIC();

    fst_getMacAddress();

    fst_getIPAddress();

    saDeadChild.sa_handler = futil_SigchldHandler; // reap all dead processes
    sigemptyset(&saDeadChild.sa_mask);
    saDeadChild.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saDeadChild, NULL)==-1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit(1);
    }

    fcom_malloc((void**)&g_ptrForkPidArr, sizeof(int) * g_nForkCnt);

    for(nLoop = 0; nLoop < g_nForkCnt; nLoop++)
    {
        if( (g_ptrForkPidArr[nLoop] = fork() ) == 0) // Child ���μ���
        {
            fst_ForkTask();
        }
        else
        {
            /* Nothing.. */
        }
    }

    while(1)
    {
        sleep(1);
    }


    return 0;
}

int fst_ForkTask(void)
{
    pthread_t   local_pthread_t;
    int nLoop = 0;
    int nFlag = 0;

    pthread_mutex_init(&ThreadCntMutex, NULL);

    /* SSL 통신을 위해 암호와 해시 알고리즘 로드 */
    SSL_library_init();

    /* 에러 기록을 남기기 위한 에러 관련 문자열 로드 */
    SSL_load_error_strings();

    /* 모든 알고리즘 로드 */
    OpenSSL_add_all_algorithms();

    while(1)
    {
        printf("Thread [%d] \n",g_nThreadCnt);

        for(nLoop = 0; nLoop < g_nThreadCnt; nLoop++)
        {
            fcom_ThreadCreate(&local_pthread_t, fst_ThreadTask, (void*)&nLoop,4 * 1024 * 1024);
            printf("Created [%d] \n",nLoop);
        }

        fcom_SleepWait(5);
    }

    return 0;
}



void* fst_ThreadTask(void* ThreadArg)
{
    int local_nCnt = 0;

    int ThreadNum = 0;

    ThreadNum = *((int *) ThreadArg);
    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Number : [%d]",ThreadNum);

    fst_SslConnectionTest(g_ConnSvrIp, 50203, ThreadNum);


    pthread_exit(NULL);

}


int fst_SslConnectionTest(char *addr, int port, int ThreadNum)
{
    int sockfd = 0;
    int nRet = 0;
    int sslfd = 0;
    int local_nCnt = 0;
    char localCertPath[256 + 1] = {0x00,};
    char testData[512+1] = {0x00,};
    char RecvBuffer[256 +1] = {0x00,};
    int nDataSize = 0;
    char szLocalIp[32 +1] = {0x00,};
    char local_err_msg[128 +1] = {0x00,};
    int p_msgLen = 0;

    char*   local_ptrData = NULL;
    CRhead	local_SHead;

    SSL_CTX *local_ctx = NULL;
    SSL *local_socket_ssl = NULL;
    const SSL_METHOD    *meth = NULL;

    sockfd = 0;
    memset(&local_SHead, 0x00, sizeof(CRhead));

    snprintf(szLocalIp,sizeof(szLocalIp),"%s",addr);

    printf("[%s] [%d] \n",szLocalIp, port);
    nRet = fst_TcpConnectionTest(szLocalIp,port,&sockfd);
    if(nRet != 0)
    {
        printf("Tcp Conn Failed \n");
        return -1;
    }
    else
    {
        meth = TLSv1_2_client_method();
        local_ctx = SSL_CTX_new(meth);
        SSL_CTX_set_cipher_list(local_ctx,"AES256-SHA");
        SSL_CTX_set_mode(local_ctx, SSL_MODE_AUTO_RETRY);
        SSL_CTX_set_ecdh_auto(local_ctx, 1);
        memset(localCertPath, 0x00, sizeof(localCertPath));
        while(1)
        {
            if(pthread_mutex_trylock(&ThreadCntMutex) == 0)
            {
                snprintf(localCertPath,sizeof(localCertPath), "%s/%s",szCertPath,szCertFile);
                if(SSL_CTX_use_certificate_file(local_ctx, localCertPath, SSL_FILETYPE_PEM) <= 0) // �������� ���Ϸ� ���� �ε��Ҷ� �����.
                {
                    ERR_print_errors_fp(stderr);
                    if(sockfd > 0)
                        close(sockfd);
                    pthread_mutex_unlock(&ThreadCntMutex);
                    printf("Cert Failed \n");
                    exit(0);
                }


                memset(localCertPath, 0x00, sizeof(localCertPath));
                snprintf(localCertPath,sizeof(localCertPath), "%s/%s",szCertPath,szKeyFile);

                if(SSL_CTX_use_PrivateKey_file(local_ctx, localCertPath, SSL_FILETYPE_PEM) <= 0)
                {
                    ERR_print_errors_fp(stderr);
                    if(sockfd > 0)
                        close(sockfd);
                    pthread_mutex_unlock(&ThreadCntMutex);
                    printf("Cert Failed \n");
                    exit(0);

                }

                if(!SSL_CTX_check_private_key(local_ctx))
                {
                    ERR_print_errors_fp(stderr);
                    if(sockfd > 0)
                        close(sockfd);
                    pthread_mutex_unlock(&ThreadCntMutex);
                    printf("Cert Failed \n");
                    exit(0);
                }
                pthread_mutex_unlock(&ThreadCntMutex);
                break;
            }
            else
            {
                if(local_nCnt > 5000)
                {
                    printf("Mutex Failed loop\n");
                    pthread_exit(NULL);
                }
                local_nCnt++;
                fcom_SleepWait(4);
            }
        }

        local_socket_ssl = SSL_new(local_ctx);
        SSL_set_fd(local_socket_ssl, sockfd);
        SSL_set_connect_state(local_socket_ssl);
        SSL_set_mode(local_socket_ssl, SSL_MODE_AUTO_RETRY );
        nRet = SSL_connect(local_socket_ssl);
        if(nRet <= 0)
        {
            printf("Connect Failed \n");

            ERR_error_string_n(ERR_get_error(), local_err_msg, sizeof(local_err_msg));

            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            if(sslfd > 0)
            {
                close(sslfd);
            }

            ERR_remove_state(0);
            SSL_CTX_free( local_ctx );
            ERR_remove_state(0);

            return (-1);
        }

        memset(testData, 0x00, sizeof(testData));
        memset(&local_SHead, 0x00, sizeof(CRhead));


        snprintf(testData,sizeof(testData),"{\n"
                                                "   \"agent_ver\" : \"1.1.9.2.0\",\n"
                                                "   \"user_key\" : \"20060217160940190820\",\n"
                                                "   \"user_mac\" : \"%s^%s\",\n"
                                                "   \"user_seq\" : 0,\n"
                                                "   \"user_sno\" : \"\"\n"
                                                "}",
                                                g_MyIp,g_MAC);
        nDataSize = strlen(testData);
        p_msgLen = nDataSize + 2;
        strcpy(local_SHead.msgtype, DAP_AGENT);
        local_SHead.msgcode  = htons(1);
        local_SHead.msgleng = htonl(p_msgLen);
        local_ptrData = (char*)malloc(sizeof(CRhead) + nDataSize);
        memset(local_ptrData, 0x00, sizeof(CRhead) +  nDataSize);


        memcpy(local_ptrData, &local_SHead.msgtype,10);
        memcpy(local_ptrData +10, &local_SHead.msgleng, 4);
        memcpy(local_ptrData +10 + 4, &local_SHead.msgcode, 2);
        memcpy(&local_ptrData[10+4+2], testData, nDataSize);


        nRet = fsock_SslSocketSend(local_socket_ssl, local_ptrData, 10+4+2 + nDataSize);
        if (nRet <= 0)
        {
            ERR_error_string_n(ERR_get_error(), local_err_msg, sizeof(local_err_msg));
            WRITE_CRITICAL(CATEGORY_DEBUG,"SSL Send Failed (%d) (%s) ", errno, local_err_msg)

            printf("SSL Send Failed (%d) (%s) \n", errno, local_err_msg);
            fflush(stdout);

            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            close(sslfd);
            ERR_remove_state(0);
            SSL_CTX_free( local_ctx );
            ERR_remove_state(0);
            fcom_MallocFree((void**)&local_ptrData);

            return -1;
        }


        memset(RecvBuffer, 0x00, sizeof(RecvBuffer));
        nRet = fsock_SslSocketRecv(local_socket_ssl, RecvBuffer,sizeof(RecvBuffer)-1);

        if(nRet <= 0)
        {
            ERR_error_string_n(ERR_get_error(), local_err_msg, sizeof(local_err_msg));
            WRITE_CRITICAL(CATEGORY_DEBUG,"SSL Recv Failed (%d) (%s) ", errno, local_err_msg)

            printf("SSL Recv Failed (%d) (%s) \n", errno, local_err_msg);
            fflush(stdout);

            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            close(sslfd);
            ERR_remove_state(0);
            SSL_CTX_free( local_ctx );
            ERR_remove_state(0);
            fcom_MallocFree((void**)&local_ptrData);


            return (-1);
        }

        if(memcmp(RecvBuffer,DAP_AGENT,strlen(DAP_AGENT)) == 0)
        {
            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            close(sslfd);
            ERR_remove_state(0);
            SSL_CTX_free( local_ctx );
            ERR_remove_state(0);
            fcom_MallocFree((void**)&local_ptrData);

            return 0;
        }
        else
        {
            printf("Recv Error shutdown \n");
            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            close(sslfd);
            ERR_remove_state(0);
            SSL_CTX_free( local_ctx );
            ERR_remove_state(0);
            fcom_MallocFree((void**)&local_ptrData);

            return (-1);
        }

        sslfd = SSL_get_fd(local_socket_ssl);
        SSL_shutdown(local_socket_ssl);
        SSL_free(local_socket_ssl);
        close(sslfd);
        ERR_remove_state(0);
        SSL_CTX_free( local_ctx );
        ERR_remove_state(0);
        fcom_MallocFree((void**)&local_ptrData);


    }


}

int fst_TcpConnectionTest(char *addr, int port,int* RefSocket)
{
    int sockfd = 0;
    int nRet = 0;
    int nFlags = 0;
    socklen_t lon;
    fd_set          rset;
    int valopt;

    struct sockaddr_in serveraddr;
    struct timeval  tval;
    char szLocalAddr[32+1] = {0x00,};

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    snprintf(szLocalAddr,sizeof(szLocalAddr),"%s",addr);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons ( port );
    serveraddr.sin_addr.s_addr = inet_addr((const char *)addr );

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return (-1);
    }

    /* Set Non Blocking */
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

    // Trying to connect with timeout
    nRet = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (nRet < 0)
    {
        if (errno == EINPROGRESS)
        {
            do
            {
                /* timeout 1�� */
                tval.tv_sec = 0;
                tval.tv_usec = 500000; // 1,000,000

                /* Socket Fd Set ���� */
                FD_ZERO(&rset);
                FD_SET(sockfd, &rset);

                nRet = select(sockfd+1, NULL, &rset, NULL, &tval);
                if (nRet < 0 && errno != EINTR)
                {
                    if(sockfd > 0)
                    {
                        close(sockfd);
                        sockfd = 0;
                    }

                    return (-1);
                }

                else if (nRet > 0)
                {
                    // Socket selected for write
                    lon = sizeof(int);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
                    {
                        if(sockfd > 0)
                        {
                            close(sockfd);
                            sockfd = 0;
                        }

                        return (-1);
                    }
                    // Check the value returned...
                    if (valopt)
                    {
                        if(sockfd > 0)
                        {
                            close(sockfd);
                            sockfd = 0;
                        }
                        return (-1);
                    }
                    break;
                }
                else
                {
                    if(sockfd > 0)
                    {
                        close(sockfd);
                        sockfd = 0;
                    }
                    return (-1);
                }
            } while (1);
        }
        else
        {
            if(sockfd > 0)
            {
                close(sockfd);
                sockfd = 0;
            }

            return (-1);
        }
    }

    nFlags = fcntl(sockfd, F_GETFL, 0);
    nFlags = nFlags^O_NONBLOCK;
    fcntl(sockfd, F_SETFL, nFlags);

    if(RefSocket != NULL)
        *RefSocket = sockfd;


    return 0;
}

int fst_getNIC()
{

    int sock;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        return 0;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;

    if (ioctl(sock, SIOCGIFCONF, &ifconf) == -1)
    {
        return 0;

    }

    ifs = ifconf.ifc_len / sizeof(ifr[0]);

    for (i = 0; i < ifs; i++)
    {

        if(strcmp(ifr[i].ifr_name, "lo") != 0)
        {
            strcpy(g_NIC, ifr[i].ifr_name);
            break;
        }
    }

    close(sock);

    return 1;

}
void fst_convrtMac(const char *data, char *cvrt_str, int sz)
{
    char buf[128] = {0,};
    char t_buf[8];
    char *stp = strtok( (char *)data , ":" );
    int temp=0;

    do
    {
        memset( t_buf, 0, sizeof(t_buf) );

        sscanf( stp, "%x", &temp );
        snprintf( t_buf, sizeof(t_buf)-1, "%02X", temp );
        strncat( buf, t_buf, sizeof(buf)-1 );
        strncat( buf, ":", sizeof(buf)-1 );

    } while( (stp = strtok( NULL , ":" )) != NULL );

    buf[strlen(buf) -1] = '\0';
    strncpy( cvrt_str, buf, sz );

}


int fst_getMacAddress()
{
    int sock;
    struct ifreq ifr;
    char mac_adr[18] = {0,};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return 0;
    }


    strcpy(ifr.ifr_name, g_NIC);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr)< 0)
    {
        close(sock);
        return 0;

    }

    //convert format ex) 00:00:00:00:00:00

    fst_convrtMac( ether_ntoa((struct ether_addr *)(ifr.ifr_hwaddr.sa_data)), mac_adr, sizeof(mac_adr) -1 );

    strcpy(g_MAC, mac_adr);

    close(sock);

    return 1;

}


static int fst_getIPAddress()
{
    int sock;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        return 0;
    }

    strcpy(ifr.ifr_name, g_NIC);
    if (ioctl(sock, SIOCGIFADDR, &ifr)< 0)
    {
        close(sock);
        return 0;

    }

    sin = (struct sockaddr_in*)&ifr.ifr_addr;
    strcpy(g_MyIp, inet_ntoa(sin->sin_addr));

    close(sock);
    return 1;

}
