//
// Created by KimByoungGook on 2021-06-02.
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

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include "util.h"
#include "com/dap_def.h"
#include "com/dap_com.h"
#include "secure/dap_secure.h"
#include "sock/dap_sock.h"

static int fst_Init(void);
static int fst_ParseCmdOption(int argc, char **argv);
static void fst_DisplayVersion();
static int fst_MainStressTask(void);
static void fst_SigchldHandler(int sigNo);
static int fst_ForkTask(void);
void* fst_ThreadTask(void* ThreadArg);
int fst_TcpConnectionTest(char *addr, int port,int* RefSocket);
int fst_SslConnectionTest(char *addr, int port, int ThreadNum);

int main(int argc, char** argv)
{
    int nRet = 0;
    fst_Init();

    /* DAP Process Log Init */
    fcom_LogInit("dap_update_stress");

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
void 	func_DUMP_BIN_DATA_VIEW( char *pre_data_buff , int pre_data_buff_size )
{

    int			cut_line = 0;
    int			put_for = 0;
    char		mybuff[60];
    int			mybuff_i = 0;


    if ( pre_data_buff_size <= 0 )
    {
        printf("(%s)@ line num : (%d) Packet Size is Zero or Minus value : %d " , __func__ , __LINE__ , pre_data_buff_size );
        return; }


    memset( mybuff , 0x00 , sizeof( mybuff ) );
    printf("Dump : %d -------------------------------------------------------------------------------\n", pre_data_buff_size );


    while ( put_for < pre_data_buff_size )
    {

        if( cut_line > 20 )
        {
            printf("\t");

            while ( mybuff_i < cut_line  )
            {

                if ( mybuff[mybuff_i] >= 0x20 &&  mybuff[mybuff_i] <= 0x7e )
                {
                    printf("%c", mybuff[mybuff_i] );
                }
                else {  printf(".");  }

                mybuff_i++;
            }

            cut_line = 0;
            memset( mybuff , 0x00 , sizeof( mybuff ) );
            mybuff_i=0;
            printf("\n");
        }

        printf("[%2x]",(unsigned char)pre_data_buff[put_for] );

        mybuff[cut_line] = pre_data_buff[put_for];

        put_for ++;
        cut_line ++;
    }   // while

    put_for = cut_line;

    while ( put_for <= 20 ) { printf("[  ]"); put_for++;  }

    printf("\t");

    while ( mybuff_i < cut_line  )
    {

        if ( mybuff[mybuff_i] >= 0x20 &&  mybuff[mybuff_i] <= 0x7e )
        {
            printf("%c", mybuff[mybuff_i] );
        }
        else {  printf(".");  }

        mybuff_i++;
    }



    printf("\n--------------------------------------------------------------------------------------------------------------\n");
    fflush(stdout);

    return;
}

static int fst_MainStressTask(void)
{
    int nLoop = 0;
    char szFilePath[256 +1] = {0x00,};
    struct sigaction saDeadChild;

    memset(szFilePath, 0x00, sizeof(szFilePath));

    saDeadChild.sa_handler = fst_SigchldHandler; // reap all dead processes
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
        if( (g_ptrForkPidArr[nLoop] = fork() ) == 0)
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

static int fst_ForkTask(void)
{
    pthread_t   local_pthread_t;
    int nLoop = 0;

    pthread_mutex_init(&ThreadCntMutex, NULL);


    /* SSL 통신을 위해 암호와 해시 알고리즘 로드 */
    SSL_library_init();

    /* 에러 기록을 남기기 위한 에러 관련 문자열 로드 */
    SSL_load_error_strings();

    /* 모든 알고리즘 로드 */
    OpenSSL_add_all_algorithms();

    while(1)
    {
        for(nLoop = 0; nLoop < g_nThreadCnt; nLoop++)
        {
            fcom_ThreadCreate(&local_pthread_t, fst_ThreadTask, (void*)&nLoop,4 * 1024 * 1024);

            fcom_SleepWait(5);
        }

        sleep(3);
    }

    return 0;
}

void* fst_ThreadTask(void* ThreadArg)
{
    int local_nCnt = 0;
    int ThreadNum = 0;

    ThreadNum = *((int *) ThreadArg);
    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Number : [%d]",ThreadNum);

    /** SSL Connect **/
    fst_SslConnectionTest(g_ConnSvrIp, 50100, ThreadNum);


    pthread_exit(NULL);

}

int fst_SslConnectionTest(char *addr, int port, int ThreadNum)
{
    int local_nLoop = 0;
    int sockfd = 0;
    int nRet = 0;
    int sslfd = 0;
    int local_nCnt = 0;
    int local_nReadSize = 0;
    int local_nFileSize = 0;
    int local_nRecvSize = 0;
    unsigned int local_nDataSize = 0;
    short local_nCmd = 0;
    char local_szSendBuffer[95 +1] = {0x00,};
    char localCertPath[256 + 1] = {0x00,};
    char szLocalIp[32 +1] = {0x00,};
    char local_err_msg[128 +1] = {0x00,};
    char local_szHeaderStr[14 +1] = {0x00,};
    char local_szProdName[3 +1] = {0x00,};
    char local_szUniq[30 +1] = {0x00,};
    char local_szVersion[16 +1] = {0x00,};

    CRhead	local_SHead;

    char*       local_nFileBuffer   = NULL;
    SSL_CTX*    local_ctx           = NULL;
    SSL*        local_socket_ssl    = NULL;
    const SSL_METHOD    *meth       = NULL;

    sockfd = 0;
    memset(&local_SHead, 0x00, sizeof(CRhead));

    snprintf(szLocalIp,sizeof(szLocalIp),"%s",addr);

    nRet = fst_TcpConnectionTest(szLocalIp,port,&sockfd);
    if(nRet != 0)
    {
        return -1;
    }
    else
    {
//        meth = SSLv23_client_method();
        meth = TLSv1_2_client_method();
        local_ctx = SSL_CTX_new(meth);

//        SSL_CTX_set_cipher_list(local_ctx,"AES256-SHA");

        SSL_CTX_set_mode(local_ctx, SSL_MODE_AUTO_RETRY);
        SSL_CTX_set_ecdh_auto(local_ctx, 1);

        memset(localCertPath, 0x00, sizeof(localCertPath));

        while(1)
        {
            if(pthread_mutex_trylock(&ThreadCntMutex) == 0)
            {
                snprintf(localCertPath,sizeof(localCertPath), "%s/%s",szCertPath,szCertFile);
                if(SSL_CTX_use_certificate_file(local_ctx, localCertPath, SSL_FILETYPE_PEM) <= 0) // ???????? ????? ???? ?ε???? ?????.
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
            WRITE_CRITICAL(CATEGORY_DEBUG, "Conntect SSL Protocol (%s) ",  SSL_get_version(local_socket_ssl));
            ERR_error_string_n(ERR_get_error(), local_err_msg, sizeof(local_err_msg));
            WRITE_CRITICAL(CATEGORY_DEBUG,"Openssl Conntect Failed msg  (%s) Protocol : [%s] ",
                           local_err_msg, SSL_get_version(local_socket_ssl ) );
            WRITE_DEBUG(CATEGORY_DEBUG,"SSL PreAuth Ret (%d) (%s)",
                        nRet, SSL_state_string_long(local_socket_ssl) );
            if ( local_socket_ssl != NULL )
            {
                sslfd = SSL_get_fd(local_socket_ssl);
                SSL_shutdown(local_socket_ssl);
                SSL_free(local_socket_ssl);
                local_socket_ssl = NULL;
            }

            if(sslfd > 0)
            {
                close(sslfd);
                sslfd = 0;
            }
            if ( local_ctx != NULL )
            {
                SSL_CTX_free( local_ctx );
                local_ctx = NULL;
            }

            ERR_remove_state(0);
            if ( sockfd > 0)
            {
                close(sockfd);
                sockfd = 0;
            }

            goto _out_;
        }
        else
        {
            local_nDataSize = 78;   local_nDataSize = htonl(local_nDataSize);
            local_nCmd = 1;         local_nCmd = htons(local_nCmd);

            // 고정(헤더 스트링)
            snprintf(local_szHeaderStr, sizeof(local_szHeaderStr), "%s", "IN10AUTOUPDATE");
            // 제품 키워드(Product Type)
            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAP");
//            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAM");
            // 가비지 값
            snprintf(local_szUniq, sizeof(local_szUniq), "%s", "111111111111111111111111111111");
            // 제품 버전
            // TODO 테스트시 변경필요.
            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.2.1.3.0");
//            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.0.9.9");

            memcpy(local_szSendBuffer, local_szHeaderStr, 14);
            memcpy(local_szSendBuffer + 14, &local_nDataSize, 4);
            memcpy(local_szSendBuffer + 18, &local_nCmd, 2);
            memcpy(local_szSendBuffer + 20, local_szProdName, 3);
            memcpy(local_szSendBuffer + 23, local_szUniq, 30);
            memcpy(local_szSendBuffer + 53, local_szVersion, 16);

            nRet = fsock_SslSocketSend(local_socket_ssl, local_szSendBuffer, sizeof(local_szSendBuffer));
            if (nRet <= 0)
            {
                printf("\tHost (%s) port (%d)  SSL_write Failed code : (%d) msg(%s) |%s@%d",
                       addr,
                       port,
                       errno,strerror(errno),__func__ ,__LINE__);

                goto _out_;
            }

            // 응답 수신 후 버린다.
            memset(local_szSendBuffer, 0x00, sizeof(local_szSendBuffer));
            nRet = fsock_SslSocketRecv(local_socket_ssl, local_szSendBuffer, sizeof(local_szSendBuffer));

            local_nDataSize = 78;   local_nDataSize = htonl(local_nDataSize);
            local_nCmd = 4;         local_nCmd = htons(local_nCmd);

            memset(local_szSendBuffer, 0x00, sizeof(local_szSendBuffer));

            // 고정(헤더 스트링)
            snprintf(local_szHeaderStr, sizeof(local_szHeaderStr), "%s", "IN10AUTOUPDATE");
            // 제품 키워드(Product Type)
            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAP");
//            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAM");
            // 가비지 값
            snprintf(local_szUniq, sizeof(local_szUniq), "%s", "111111111111111111111111111111");
            // 제품 버전
            // TODO 테스트시 변경필요.
            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.2.1.3.0");
//            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.0.9.9");

            memcpy(local_szSendBuffer, local_szHeaderStr, 14);
            memcpy(local_szSendBuffer + 14, &local_nDataSize, 4);
            memcpy(local_szSendBuffer + 18, &local_nCmd, 2);
            memcpy(local_szSendBuffer + 20, local_szProdName, 3);
            memcpy(local_szSendBuffer + 23, local_szUniq, 30);
            memcpy(local_szSendBuffer + 53, local_szVersion, 16);

            nRet = fsock_SslSocketSend(local_socket_ssl, local_szSendBuffer, sizeof(local_szSendBuffer));
            if (nRet <= 0)
            {
                printf("\tHost (%s) port (%d)  SSL_write Failed code : (%d) msg(%s) |%s@%d",
                       addr,
                       port,
                       errno,strerror(errno),__func__ ,__LINE__);

                goto _out_;
            }

            // TODO 테스트시 변경필요.
            // 파일 사이즈, agent나 update cfg의 파일 사이즈.
            local_nFileSize = 24132800;
//            local_nFileSize = 40096208;

            local_nFileBuffer = (char*)malloc(local_nFileSize + sizeof(short) + 14 + sizeof(int)) ;
            memset(local_nFileBuffer, 0x00, local_nFileSize);

            local_nReadSize = 10262;
            local_nFileSize += 22;

            local_nRecvSize = 0;
            /** File 데이터 수신 **/
            while(local_nRecvSize < local_nFileSize)
            {
                nRet = SSL_read(local_socket_ssl, local_nFileBuffer, local_nReadSize);
                if ( nRet != 85 && nRet <= 0)
                    break;
                local_nRecvSize += nRet;
            }

            if ( local_nFileBuffer != NULL)
            {
                free(local_nFileBuffer);
                local_nFileBuffer = NULL;
            }

            local_nDataSize = 78;    local_nDataSize = htonl(local_nDataSize);
            local_nCmd = 5;         local_nCmd = htons(local_nCmd);

            memset(local_szSendBuffer, 0x00, sizeof(local_szSendBuffer));
            // 고정(헤더 스트링)
            snprintf(local_szHeaderStr, sizeof(local_szHeaderStr), "%s", "IN10AUTOUPDATE");
            // 제품 키워드(Product Type)
            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAP");
//            snprintf(local_szProdName, sizeof(local_szProdName), "%s", "DAM");
            // 가비지 값
            snprintf(local_szUniq, sizeof(local_szUniq), "%s", "111111111111111111111111111111");
            // 제품 버전
            // TODO 테스트시 변경필요.
            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.2.1.3.0");
//            snprintf(local_szVersion, sizeof(local_szVersion), "%s", "1.0.9.9");

            memcpy(local_szSendBuffer, local_szHeaderStr, 14);
            memcpy(local_szSendBuffer + 14, &local_nDataSize, 4);
            memcpy(local_szSendBuffer + 18, &local_nCmd, 2);
            memcpy(local_szSendBuffer + 20, local_szProdName, 3);
            memcpy(local_szSendBuffer + 23, local_szUniq, 30);
            memcpy(local_szSendBuffer + 53, local_szVersion, 16);

            nRet = fsock_SslSocketSend(local_socket_ssl, local_szSendBuffer, sizeof(local_szSendBuffer));
            if (nRet <= 0)
            {
                printf("\tHost (%s) port (%d)  SSL_write Failed code : (%d) msg(%s) |%s@%d",
                       addr,
                       port,
                       errno,strerror(errno),__func__ ,__LINE__);

                goto _out_;
            }

            printf("Patch Success \n");
        }

_out_:
        if ( local_socket_ssl != NULL )
        {
            sslfd = SSL_get_fd(local_socket_ssl);
            SSL_shutdown(local_socket_ssl);
            SSL_free(local_socket_ssl);
            local_socket_ssl = NULL;
        }

        if ( sslfd > 0)
        {
            close(sslfd);
            sslfd = 0;
        }

        if ( local_ctx != NULL )
        {
            SSL_CTX_free( local_ctx );
            local_ctx = NULL;
        }

        ERR_remove_state(0);
        if ( sockfd > 0)
        {
            close(sockfd);
            sockfd = 0;
        }

        if ( local_nFileBuffer != NULL)
        {
            free(local_nFileBuffer);
            local_nFileBuffer = NULL;
        }
        ERR_free_strings();
        EVP_cleanup();
        ERR_remove_state(0);
        sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
        CRYPTO_cleanup_all_ex_data();

    }

    return 0;

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
                /* timeout 1?? */
                tval.tv_sec = 0;
                tval.tv_usec = 500000; // 1,000,000

                /* Socket Fd Set ???? */
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

static void fst_DisplayVersion()
{
    puts("------------------------------------------");
    puts("\n\nProgram Usage ForeGround \n\t Ex) dap_update_stress ProcessCnt ThreadCnt SvrIP");
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
    snprintf(szCertPath,sizeof(szCertPath),"%s/bin/util/certs",getenv("DAP_HOME"));
    snprintf(szCaFile,sizeof(szCaFile),"%s","dap_util.pem");
    snprintf(szCertFile,sizeof(szCertFile),"%s","dap_util.pem");
    snprintf(szKeyFile,sizeof(szKeyFile),"%s","dap_util_key.pem");

//    printf("Succeed in init, pid(%d) |%s\n", getpid(),__func__);

    return TRUE;

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

/**SIGCHLD HANDLER**/
static void fst_SigchldHandler(int sigNo)
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