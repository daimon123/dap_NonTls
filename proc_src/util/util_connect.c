//
// Created by KimByoungGook on 2020-10-22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include "util.h"
#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "sock/dap_sock.h"



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

int fUtil_TcpConnectionTest(char *addr, int port, int SslFlag, int* RefSocket)
{
    int sockfd = 0;
    int nRet = 0;
    int nFlags = 0;
    socklen_t lon;
    fd_set          rset;
    int valopt;

    struct sockaddr_in serveraddr;
    struct hostent *host = NULL;
    struct timeval  tval;
    char szLocalAddr[32+1] = {0x00,};

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    snprintf(szLocalAddr,sizeof(szLocalAddr),"%s",addr);

    printf("connect trying host(%s) port(%d) \n", szLocalAddr, port);
    host = gethostbyname(szLocalAddr);
    if (host == (struct hostent *)NULL)
    {
        printf("tcp: connect to address failed %s:%d - %s(%d)\n", szLocalAddr, port, strerror(errno), errno);
        return (-1);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    memcpy(&serveraddr.sin_addr, host->h_addr, host->h_length);
    serveraddr.sin_port = htons((unsigned short)port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket create failed %s:%d - %s(%d)\n", szLocalAddr, port, strerror(errno), errno);
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
                printf("sec : [%d] , usec : [%d] \n",g_nTimeOutsec,g_nTimeOutUsec);

                /* timeout 1초 */
                tval.tv_sec = g_nTimeOutsec;
                tval.tv_usec = g_nTimeOutUsec; // 1,000,000

                FD_ZERO(&rset);
                FD_SET(sockfd, &rset);

                nRet = select(sockfd+1, NULL, &rset, NULL, &tval);
                if (nRet < 0 && errno != EINTR)
                {
                    printf("\tHost (%s) port (%d) Error connecting %d - %s\n",
                           szLocalAddr,
                           port,
                           errno,
                           strerror(errno));
                    if(sockfd > 0)
                    {
                        close(sockfd);
                        sockfd = 0;
                    }

                    return (-1);
                }

                else if (nRet > 0)
                {
                    // Socket Selected for write
                    lon = sizeof(int);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
                    {
                        printf("\tHost (%s) port (%d) Error in getsockopt() %d - %s\n",
                               szLocalAddr,
                               port,
                               errno, strerror(errno));
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
                        printf("\tHost (%s) port (%d)  Error in delayed connection() %d - %s\n",
                               szLocalAddr,
                               port,
                               valopt,
                               strerror(valopt));
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
                    printf("\tHost (%s) port (%d) Error Connection Timeout Cancelling! \n",
                           szLocalAddr, port);
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
            printf("\tHost (%s) port (%d) Error connecting %d - %s\n",
                   szLocalAddr,
                   port,
                   errno, strerror(errno));
            if(sockfd > 0)
            {
                close(sockfd);
                sockfd = 0;
            }

            return (-1);
        }
    }

    if(SslFlag != 1)
    {
        if(sockfd > 0)
            close(sockfd);

        printf("tcp: connect to address Success (%s) (%d)\n", addr, port);
    }
    else
    {
        /* Set Blocking */
        nFlags = fcntl(sockfd, F_GETFL, 0);
        nFlags = nFlags^O_NONBLOCK;
        fcntl(sockfd, F_SETFL, nFlags);

        if(RefSocket != NULL)
            *RefSocket = sockfd;
    }

    return 0;
}

int fpcif_SendJsonToServer(SSL *ssl, int code, char *jsonData, int jsonSize)
{
    int     rxt = 0;
    char    *data = NULL;
    CRhead  SHead;

    memset(&SHead,   0x00, sizeof(SHead));
    strcpy(SHead.msgtype, DAP_AGENT);

    SHead.msgcode  = htons(code);
    SHead.msgleng = htonl(jsonSize + 2); //include msgCode size(2)

    if(fcom_malloc((void**)&data, sizeof(char)*(sizeof(SHead)+jsonSize)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }
    memcpy(data, &SHead, sizeof(CRhead));
    memcpy(&data[sizeof(CRhead)], (char *)jsonData, jsonSize);

    rxt = fsock_SslSocketSend(ssl, data, sizeof(CRhead)+jsonSize);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,  "[REQ] Retry, because fail in send, errno(%d)code(%d)",
                       errno,code);
    }

    fcom_MallocFree((void**)&data);

    return 0;
}

int fUtil_SslConnectionTest(char *addr, int port)
{
    int sockfd = 0;
    int nRet = 0;
    char localCertPath[256 + 1] = {0x00,};
    char testData[20+1] = {0x00,};
    char RecvBuffer[256 +1] = {0x00,};
    int nDataSize = 0;
    char szLocalIp[32 +1] = {0x00,};

    int p_msgLen = 0;

    SSL_CTX *local_ctx = NULL;
    SSL *local_socket_ssl = NULL;
    const SSL_METHOD    *meth = NULL;

    sockfd = 0;
    snprintf(szLocalIp,sizeof(szLocalIp),"%s",addr);
    nRet = fUtil_TcpConnectionTest(szLocalIp,port,1,&sockfd);
    if(nRet != 0)
    {
        printf("Tcp Connect Fail host (%s) port (%d) Not SSL Check \n",szLocalIp,port);
        return -1;
    }
    else
    {
        SSL_load_error_strings();
        SSLeay_add_ssl_algorithms();
        meth = SSLv23_client_method();
        local_ctx = SSL_CTX_new(meth);

        snprintf(localCertPath,sizeof(localCertPath), "%s/%s",szCertPath,szCertFile);
        if(SSL_CTX_use_certificate_file(local_ctx, localCertPath, SSL_FILETYPE_PEM) <= 0) // �������� ���Ϸ� ���� �ε��Ҷ� �����.
        {
            ERR_print_errors_fp(stderr);
            return (-1);
        }
        memset(localCertPath, 0x00, sizeof(localCertPath));
        snprintf(localCertPath,sizeof(localCertPath), "%s/%s",szCertPath,szKeyFile);
        if(SSL_CTX_use_PrivateKey_file(local_ctx, localCertPath, SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stderr);
            return (-1);
        }

        if(!SSL_CTX_check_private_key(local_ctx))
        {
            fprintf(stderr, "Private key does not match the certificate public keyn");
            return (-1);
        }

        local_socket_ssl = SSL_new(local_ctx);

        SSL_set_fd(local_socket_ssl, sockfd);
        nRet = SSL_connect(local_socket_ssl);
        if(nRet <= 0)
        {
            printf("\thost (%s) port (%d) Could not connect to SSL Server|%s",
                   addr,
                   port,
                   __func__ );
            return (-1);
        }

        memset(testData, 0x00, sizeof(testData));
        memset(&SHead, 0x00, sizeof(CRhead));

        snprintf(testData,sizeof(testData),"%s","{\"Hello\":9}");
        nDataSize = strlen(testData);

        p_msgLen = nDataSize + 2;
        strcpy(SHead.msgtype, DAP_AGENT);
        SHead.msgcode  = htons(0);
        SHead.msgleng = htonl(p_msgLen);

//        func_DUMP_BIN_DATA_VIEW(SHead.msgtype,sizeof(SHead.msgtype));
//        func_DUMP_BIN_DATA_VIEW(&SHead.msgcode,sizeof(SHead.msgcode));
//        func_DUMP_BIN_DATA_VIEW(&SHead.msgleng,sizeof(SHead.msgleng));
//        func_DUMP_BIN_DATA_VIEW(testData,strlen(testData));

        data = (char*)malloc(sizeof(CRhead) + nDataSize);
        memset(data, 0x00, sizeof(CRhead) +  nDataSize);

        memcpy(data, &SHead.msgtype,10);
        memcpy(data +10, &SHead.msgleng, 4);
        memcpy(data +10 + 4, &SHead.msgcode, 2);
//        func_DUMP_BIN_DATA_VIEW(data,10+4+2 );


        memcpy(&data[10+4+2], testData, nDataSize);
//        func_DUMP_BIN_DATA_VIEW(&data[10+4+2],nDataSize);


        nRet = fsock_SslSocketSend(local_socket_ssl, data, 10+4+2 + nDataSize);


        if (nRet <= 0)
        {
            printf("\tHost (%s) port (%d)  SSL_write Failed code : (%d) msg(%s) |%s@%d",
                   addr,
                   port,
                   errno,strerror(errno),__func__ ,__LINE__);
            return -1;
        }

        memset(RecvBuffer, 0x00, sizeof(RecvBuffer));
        nRet = fsock_SslSocketRecv(local_socket_ssl, RecvBuffer, sizeof(RecvBuffer)-1);

        if(nRet <= 0)
        {
            printf("\tHost (%s) port (%d) SSL_read Failed code : (%d) msg(%s) |%s@%d\n",
                   addr,
                   port,
                   errno,strerror(errno),__func__ ,__LINE__);
            return (-1);
        }

        SSL_shutdown(local_socket_ssl);

        if(sockfd > 0)
            close(sockfd);
        SSL_free(local_socket_ssl);
        SSL_CTX_free(local_ctx);


        if(memcmp(RecvBuffer,DAP_AGENT,strlen(DAP_AGENT)) == 0)
        {
            printf("\tHost(%s) port (%d) Recv Success !! \n",
                   addr,
                   port);

            return 0;
        }
        else
        {
            printf("\tHost(%s) port (%d) Recv Fail !! \n",
                   addr,
                   port);

            return (-1);
        }
    }

}

int fUtil_HwBaseTcpCnnectionTest(void)
{
    int		rowCnt = 0;
    int     nRet = 0;
    char    szRet[32 +1] = {0x00,};
    char	sqlColumn[15 +1] = {0x00,};
    char	sqlBuf[128 +1] = {0x00,};
    char    szAccessIp[32 +1] = {0x00,};
    char    szErrMsg[64 +1] = {0x00,};

    FILE*   fp = NULL;

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));


    fp = fopen("./HWBASE_AGENT_CONN_TEST","w");

    sprintf(sqlBuf, "SELECT HB_ACCESS_IP FROM HW_BASE_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            nRet = 0;
            memset(szAccessIp, 0x00, sizeof(szAccessIp));
            memset(szErrMsg, 0x00, sizeof(szErrMsg));

            snprintf(szAccessIp,sizeof(szAccessIp),"%s",g_stMyCon->row[0]);

            nRet = fUtil_TcpConnectionTest(szAccessIp,50204,0,NULL); /* Agent Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");   }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");   }

            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s] \n","Agent Check",szAccessIp,"Result",szRet);

            nRet = fUtil_TcpConnectionTest(szAccessIp,50119,0,NULL); /* EMG Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");  }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");  }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","EMG Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s] \n","EMG Agent Check",szAccessIp,"Result",szRet);

            fflush(fp);
        }
    }
    if(fp != NULL)
    {
        fclose(fp);
    }

    fdb_SqlFreeResult(g_stMyCon);

    printf("Proc Row Cnt (%d) |%s\n",rowCnt,__func__ );

    return 0;

}

int fUtil_HwBaseInactiveTcpCnnectionTest(void)
{
    int		rowCnt = 0;
    int     nRet = 0;
    char    szRet[32 +1] = {0x00,};
    char	sqlColumn[15 +1] = {0x00,};
    char	sqlBuf[128 +1] = {0x00,};
    char    szAccessIp[32 +1] = {0x00,};
    char    szErrMsg[64 +1] = {0x00,};
    FILE*   fp = NULL;

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));


    fp = fopen("./HWBASE_INACTIVE_AGENT_CONN_TEST","w");

    sprintf(sqlBuf, "SELECT HB_ACCESS_IP FROM HW_BASE_TB WHERE TIMESTAMPDIFF(MINUTE, HB_ACCESS_TIME, NOW()) > 10");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            nRet = 0;
            memset(szAccessIp, 0x00, sizeof(szAccessIp));
            memset(szErrMsg, 0x00, sizeof(szErrMsg));

            snprintf(szAccessIp,sizeof(szAccessIp),"%s",g_stMyCon->row[0]);
            printf("IP : [%s] \n",szAccessIp);
            nRet = fUtil_TcpConnectionTest(szAccessIp,50204,0,NULL); /* Agent Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");   }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");   }

            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s] \n","Agent Check",szAccessIp,"Result",szRet);
            printf("IP : [%s] \n",szAccessIp);
            nRet = fUtil_TcpConnectionTest(szAccessIp,50119,0,NULL); /* EMG Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");  }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");  }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","EMG Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s] \n","EMG Agent Check",szAccessIp,"Result",szRet);

            fflush(fp);
        }
    }
    if(fp != NULL)
    {
        fclose(fp);
    }


    fdb_SqlFreeResult(g_stMyCon);

    printf("Proc Row Cnt (%d) |%s\n",rowCnt,__func__ );

    return 0;

}

int fUtil_HwBaseSslCnnectionTest(void)
{
    int		rowCnt = 0;
    int     nRet = 0;
    char    szRet[32 +1] = {0x00,};
    char	sqlColumn[15 +1] = {0x00,};
    char	sqlBuf[128 +1] = {0x00,};
    char    szAccessIp[32 +1] = {0x00,};

    FILE*   fp = NULL;

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));


    fp = fopen("./HWBASE_AGENT_SSL_CONN_TEST","w");

    sprintf(sqlBuf, "SELECT HB_ACCESS_IP FROM HW_BASE_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            nRet = 0;
            memset(szAccessIp, 0x00, sizeof(szAccessIp));

            snprintf(szAccessIp,sizeof(szAccessIp),"%s",g_stMyCon->row[0]);

            nRet = fUtil_SslConnectionTest(szAccessIp,50204); /* Agent Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");            }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");               }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s]\n","Agent Check",szAccessIp,"Result",szRet);


            nRet = fUtil_SslConnectionTest(szAccessIp,50119); /* EMG Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");            }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");               }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","EMG Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s]\n","EMG Agent Check",szAccessIp,"Result",szRet);

            fflush(fp);
        }
    }
    if(fp != NULL)
    {
        fclose(fp);
    }


    fdb_SqlFreeResult(g_stMyCon);
    fdb_SqlClose(g_stMyCon);

    printf("Proc Row Cnt (%d) |%s\n",rowCnt,__func__ );

    return 0;

}

int fUtil_HwBaseSslInActiveConectionTest(void)
{
    int		rowCnt = 0;
    int     nRet = 0;
    char    szRet[32 +1] = {0x00,};
    char	sqlColumn[15 +1] = {0x00,};
    char	sqlBuf[128 +1] = {0x00,};
    char    szAccessIp[32 +1] = {0x00,};

    FILE*   fp = NULL;

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    fp = fopen("./HWBASE_INACTIVE_AGENT_SSL_CONN_TEST","w");

    sprintf(sqlBuf, "SELECT HB_ACCESS_IP FROM HW_BASE_TB WHERE TIMESTAMPDIFF(MINUTE, HB_ACCESS_TIME, NOW()) > 10");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            nRet = 0;
            memset(szAccessIp, 0x00, sizeof(szAccessIp));

            snprintf(szAccessIp,sizeof(szAccessIp),"%s",g_stMyCon->row[0]);
            WRITE_INFO(CATEGORY_INFO,"IP : [%s] \n",szAccessIp);
            nRet = fUtil_SslConnectionTest(szAccessIp,50204); /* Agent Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");               }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");               }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s]\n","Agent Check",szAccessIp,"Result",szRet);

            WRITE_INFO(CATEGORY_INFO,"IP : [%s] \n",szAccessIp);
            nRet = fUtil_SslConnectionTest(szAccessIp,50119); /* EMG Port üũ */
            if(nRet == 0) { snprintf(szRet,sizeof(szRet),"%s","SUCCESS");            }
            else          { snprintf(szRet,sizeof(szRet),"%s","FAIL   ");            }
            WRITE_INFO(CATEGORY_INFO,"%-12s [%s] Result [%s]","EMG Agent Check",szAccessIp,szRet);
            fprintf(fp,"%-16s [%s] %-7s [%s]\n","EMG Agent Check",szAccessIp,"Result",szRet);

            fflush(fp);
        }
    }
    if(fp != NULL)
    {
        fclose(fp);
    }


    fdb_SqlFreeResult(g_stMyCon);
    fdb_SqlClose(g_stMyCon);

    printf("Proc Row Cnt (%d) |%s\n",rowCnt,__func__ );

    return 0;

}