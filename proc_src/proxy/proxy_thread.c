//
// Created by KimByoungGook on 2021-04-09.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <openssl/ssl.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include "com/dap_com.h"
#include "com/dap_req.h"
#include "sock/dap_sock.h"
#include "secure/dap_secure.h"

#include "proxy.h"
#include "dap_version.h"


static unsigned char ENC_AESKEY[] =
{
        0x31, 0xf7, 0xe5, 0xab, 0x43, 0xd2,
        0x43, 0x9f, 0x87, 0xf1, 0x13, 0x6f,
        0x14, 0xcd, 0x96, 0x5f,	0xff, 0x95,
        0x10, 0x7c, 0x6b, 0xc8, 0x4d, 0x8c,
        0x93, 0xbb, 0x0e, 0x88, 0xd8, 0xe8,
        0xe9, 0xf5
};

static unsigned char ENC_IV[] =
{
        0x32, 0xf8, 0xe6, 0xac, 0x44,
        0xd3, 0x44, 0xa0, 0x88, 0xf2,
        0x14, 0x70, 0x15, 0xce, 0x07, 0x60,
        0x11, 0x45, 0x35, 0xce, 0x17, 0x50,
        0x12, 0x46, 0x36, 0xc6, 0x18, 0x51,
        0x13, 0x42, 0x31, 0xc6
};

void* fproxy_ThreadWorkd(void* Param_pstProcSockInfo)
{
    int		selectRes = 0;
    int		sockServer = 0;
    int 	reuseS = 1;
    int		byteAlreadySent = 0;
    int		bufferAllocResult = 0;
    int		byteRead = 0,byteSent = 0;
    int	 	epipeError = 0;
    const int     bufsize = 10240;
    int		rxt = 0;
    char local_err_msg[256] = {0x00,};


    void   *EncBuffer = NULL, *DecryptBuffer = NULL, *bufferBackup = NULL;
    char* 	local_data = NULL,  *dataBackup = NULL;
    CRhead  SHead;
    fd_set multiplexing, multiplexingBackup;
    _DAP_SOCK_PROXY_INFO local_pstProcSockInfo;

    memset(&local_pstProcSockInfo, 0x00, sizeof(_DAP_SOCK_PROXY_INFO));
    memset(&SHead, 0x00, sizeof(CRhead));

    memcpy(&local_pstProcSockInfo,(_DAP_SOCK_PROXY_INFO*)Param_pstProcSockInfo,sizeof(_DAP_SOCK_PROXY_INFO));

    WRITE_INFO(CATEGORY_DEBUG, "thread(%d:%d) work ", local_pstProcSockInfo.ClientIp, getpid());
    /**SETTING HANDLER TO NOT IGNORE SIGINT**/
    saSignInt.sa_handler=SIG_DFL;
    sigemptyset(&saSignInt.sa_mask);
    saSignInt.sa_flags=0;
    if(sigaction(SIGINT, &saSignInt, NULL)==-1)
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while sigaction");
        exit(FAILURE);
    }

    /**SOCKET CREATION**/
    sockServer = fsock_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( sockServer < 0 )
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while socket creation");
        exit(FAILURE);
    }

    /**SETSOCKOPT SO_REUSEADDR**/
    if(setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, &reuseS, sizeof(reuseS)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket option");
        exit(FAILURE);
    }

    /** 2021.04.09 ���ۻ���� 10240�̹Ƿ� �ʿ䰡 ������� �ּ�ó��. **/
    if(setsockopt(sockServer,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(sockServer,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    /**PROXY has to connect to server**/
    if( fsock_Connect(sockServer, (SA*)&server, sizeof(server) ) !=0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in connect");
        fsock_Close(sockServer);
        exit(FAILURE);
    }

    WRITE_INFO(CATEGORY_INFO,"PID[%d]: Succeed in connected target, csock(%d)ssock(%d)cip(%s)",
               getpid(),
               local_pstProcSockInfo.clientSock,
               sockServer,
               local_pstProcSockInfo.ClientIp);


    FD_ZERO(&multiplexing);
    FD_SET(local_pstProcSockInfo.clientSock,&multiplexing);
    FD_SET(sockServer,&multiplexing);

    multiplexingBackup = multiplexing; /* Backup*/

    while( 1 )
    {
        multiplexing = multiplexingBackup;	/*-- Restore --*/

        if ( g_ForkExit == FAILURE)
        {
            break;
        }

        if( ( selectRes = fsock_Select((local_pstProcSockInfo.clientSock > sockServer ?
                                        local_pstProcSockInfo.clientSock : sockServer) + 1,
                                       &multiplexing, NULL, NULL,
                                       g_ForkExit) ) == (-1) )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Select Return Failed Break.. ");
            break;
        }

        if(selectRes > 0)
        {
            if(FD_ISSET(local_pstProcSockInfo.clientSock,&multiplexing))
            {
                char szEncHeaderBuffer[32] = {0x00,};
                char szHeader[32] = {0x00,};
                char msgType[10] = {0x00,};
                int  	msgLeng = 0;
                short	msgCode = 0;
                int decrypto_size = 0;
                int nByteAlreadyRead = 0;

                CRhead RHead;
                memset((void *)&RHead, 0x00, sizeof(RHead));

                rxt = fsock_Recv(local_pstProcSockInfo.clientSock, (char *)szEncHeaderBuffer, sizeof(szEncHeaderBuffer),0);
                if (!fsec_Decrypt(szEncHeaderBuffer, rxt, ENC_AESKEY, ENC_IV, (unsigned char*)szHeader, &decrypto_size))
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Decrypt Failed ");
                    break;
                }
                memcpy(&RHead, szHeader, sizeof(RHead));
                memset(msgType, 0x00, sizeof(msgType));
                strcpy(msgType, RHead.msgtype);
                msgLeng = ntohl(RHead.msgleng);
                msgCode = ntohs(RHead.msgcode);

//                WRITE_DEBUG(CATEGORY_DEBUG,"Msg Type : [%s] Msg Leng : [%d] Msg Code : [%d] \n",
//                            msgType, msgLeng, msgCode);

                /** CLIENT WAKES UP **/
                if(fcom_malloc((void**)&EncBuffer, msgLeng) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    break;
                }

                nByteAlreadyRead = 0;
                while(msgLeng != nByteAlreadyRead) {
                    if ( nByteAlreadyRead > msgLeng )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", nByteAlreadyRead);
                        break;
                    }
                    rxt = fsock_Recv(local_pstProcSockInfo.clientSock, (char *)&EncBuffer[nByteAlreadyRead], msgLeng - nByteAlreadyRead,0);
                    if ( rxt <= 0){
                        break;
                    }
                    nByteAlreadyRead += rxt;
                }

                if ( rxt < 0) {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: byteRead(%d), csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                byteRead,
                                local_pstProcSockInfo.clientSock);
                    fcom_MallocFree((void**)&EncBuffer);
                    break;
                }
                else if (rxt == 0){
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by client, csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                local_pstProcSockInfo.clientSock);
                    fcom_MallocFree((void**)&EncBuffer);
                    break;
                }
                if(fcom_malloc((void**)&DecryptBuffer, msgLeng*2) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    fcom_MallocFree((void**)&EncBuffer);
                    break;
                }

                bufferBackup = DecryptBuffer;

                decrypto_size = 0;
                if (!fsec_Decrypt(EncBuffer, msgLeng, ENC_AESKEY, ENC_IV, DecryptBuffer, &decrypto_size))
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"[Client -> DB] Decrypt Failed ");
                    break;
                }

                byteRead = decrypto_size;

                /* PROXY has to send data to server */
                byteAlreadySent = 0;
                while( byteAlreadySent != byteRead )
                {
                    if ( byteAlreadySent > byteRead )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }
                    byteSent = fsock_Send(sockServer, DecryptBuffer,(byteRead-byteAlreadySent));
//                    WRITE_DEBUG(CATEGORY_DEBUG,"@@@@@ Send To Maria : [%s] [%d]\n", DecryptBuffer,byteSent);
                    if( byteSent < 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (server-side), csock(%d) errno (%d) (%s)",
                                    local_pstProcSockInfo.thrNum,
                                    local_pstProcSockInfo.clientSock,
                                    errno,
                                    strerror(errno));
                        break;
                    }
                    DecryptBuffer    += byteSent;
                    byteAlreadySent += byteSent;
                }
                fcom_MallocFree((void**)&bufferBackup);
                fcom_MallocFree((void**)&EncBuffer);
                DecryptBuffer = NULL;
            }

            if( FD_ISSET( sockServer, &multiplexing ) )
            {
                int Headerencrypto_size = 0;
                int DataEncrypto_Size = 0;
                /** SERVER WAKES UP **/
                if(fcom_malloc((void**)&DecryptBuffer, BUFFER_LEN) != 0)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: Fail in bufferAllocReturning, ssock(%d)",
                               local_pstProcSockInfo.thrNum, sockServer);
                    break;
                }

                byteRead = recv(sockServer, DecryptBuffer, BUFFER_LEN, 0 );
                if(byteRead < 0)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: byteRead(%d), ssock(%d)",
                               local_pstProcSockInfo.thrNum, byteRead, sockServer);
                    fcom_MallocFree((void**)&DecryptBuffer);
                    break;
                }
                else if(byteRead == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by server, ssock(%d)",
                                local_pstProcSockInfo.thrNum, sockServer);
                    fcom_MallocFree((void**)&DecryptBuffer);
                    WRITE_INFO(CATEGORY_DEBUG,"-> THR[%d]END: closed by server", local_pstProcSockInfo.thrNum);
                    break;
                }

//                WRITE_DEBUG(CATEGORY_DEBUG,"Maria Recv11 [%s] [%d]", DecryptBuffer, byteRead );

                if(fcom_malloc((void**)&EncBuffer, byteRead*2) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    break;
                }

                DataEncrypto_Size = fsec_Encrypt(DecryptBuffer, byteRead, ENC_AESKEY, ENC_IV, EncBuffer);

                dataBackup      = local_data;


                char szEncHeaderBuffer[32] = {0x00,};
                strcpy(SHead.msgtype, MSG_DB_TYPE);
                SHead.msgcode  = htons(111);
                SHead.msgleng = htonl(DataEncrypto_Size);
                Headerencrypto_size = fsec_Encrypt((char*)&SHead, sizeof(SHead), ENC_AESKEY, ENC_IV, szEncHeaderBuffer);

                rxt = fsock_Send(local_pstProcSockInfo.clientSock, (char *)szEncHeaderBuffer, Headerencrypto_size);
                if(rxt < 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (client-side), ssock(%d) errno(%d) (%s) ",
                                local_pstProcSockInfo.thrNum,sockServer, errno, strerror(errno));
                    break;
                }

//                WRITE_DEBUG(CATEGORY_DEBUG,"Send To Manager Header [%d] Body : [%d]", rxt,DataEncrypto_Size);

                /*PROXY has to send data to server*/
                byteAlreadySent = 0;
                while( byteAlreadySent != DataEncrypto_Size )
                {
                    if ( byteAlreadySent > DataEncrypto_Size )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }

                    if ( DataEncrypto_Size - byteAlreadySent > BUFFER_LEN){
                        byteSent = fsock_Send(local_pstProcSockInfo.clientSock, &EncBuffer[byteAlreadySent],  BUFFER_LEN - byteAlreadySent );
                    } else {
                        byteSent = fsock_Send(local_pstProcSockInfo.clientSock, &EncBuffer[byteAlreadySent],  DataEncrypto_Size - byteAlreadySent );
                    }
//                    WRITE_DEBUG(CATEGORY_DEBUG,"Send To Manager Body [%s] [%d] ", EncBuffer, byteSent);
//                    WRITE_DEBUG(CATEGORY_DEBUG,"@@@ [%d] [%d]", byteSent,DataEncrypto_Size - byteAlreadySent );
                    if( byteSent < 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (client-side), ssock(%d) errno(%d) (%s) ",
                                    local_pstProcSockInfo.thrNum,sockServer, errno, strerror(errno));
                        break;
                    }

                    byteAlreadySent += byteSent;
                    local_data      += byteSent;

                } //while
                fcom_MallocFree((void**)&DecryptBuffer);
                fcom_MallocFree((void**)&EncBuffer);
                fcom_MallocFree((void**)&dataBackup);
                local_data   = NULL;
            }
        }
    }//while

    pthread_mutex_lock( &g_pthread_Mutex );
    thrFlag[local_pstProcSockInfo.thrNum] = 0;
    pthread_mutex_unlock( &g_pthread_Mutex );
    fsock_Close(local_pstProcSockInfo.clientSock);
    fsock_Close(sockServer);

    pthread_exit(NULL);
}

void* fproxy_ThreadWorkd_SSL(void* Param_pstProcSockInfo)
{
    int		selectRes = 0;
    int		sockServer = 0;
    int 	reuseS = 1;
    int		byteAlreadySent = 0;
    int		bufferAllocResult = 0;
    int		byteRead = 0,byteSent = 0;
    int	 	epipeError = 0;
    const int     bufsize = 10240;
    int		rxt = 0;
    char local_err_msg[256] = {0x00,};


    void*   local_buffer = NULL, *bufferBackup = NULL;
    char* 	local_data = NULL, *dataBackup = NULL;
    CRhead  SHead;
    fd_set multiplexing, multiplexingBackup;
    _DAP_SOCK_PROXY_INFO local_pstProcSockInfo;

    memset(&local_pstProcSockInfo, 0x00, sizeof(_DAP_SOCK_PROXY_INFO));
    memset(&SHead, 0x00, sizeof(CRhead));

    memcpy(&local_pstProcSockInfo,(_DAP_SOCK_PROXY_INFO*)Param_pstProcSockInfo,sizeof(_DAP_SOCK_PROXY_INFO));

    WRITE_INFO(CATEGORY_DEBUG,
               "thread(%d:%d) work ", local_pstProcSockInfo.ClientIp, getpid());

    /**SETTING HANDLER TO NOT IGNORE SIGINT**/
    saSignInt.sa_handler=SIG_DFL;
    sigemptyset(&saSignInt.sa_mask);
    saSignInt.sa_flags=0;
    if(sigaction(SIGINT, &saSignInt, NULL)==-1)
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while sigaction");
        exit(FAILURE);
    }

    /**SOCKET CREATION**/
    sockServer = fsock_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( sockServer < 0 )
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while socket creation");
        exit(FAILURE);
    }

    /**SETSOCKOPT SO_REUSEADDR**/
    if(setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, &reuseS, sizeof(reuseS)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket option");
        exit(FAILURE);
    }

    /** 2021.04.09 ���ۻ���� 10240�̹Ƿ� �ʿ䰡 ������� �ּ�ó��. **/
    if(setsockopt(sockServer,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(sockServer,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    /**PROXY has to connect to server**/
    if( fsock_Connect(sockServer, (SA*)&server, sizeof(server) ) !=0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in connect");
        fsock_Close(sockServer);
        exit(FAILURE);
    }

    WRITE_INFO(CATEGORY_INFO,"PID[%d]: Succeed in connected target, csock(%d)ssock(%d)cip(%s)",
               getpid(),
               local_pstProcSockInfo.clientSock,
               sockServer,
               local_pstProcSockInfo.ClientIp);


    FD_ZERO(&multiplexing);
    FD_SET(local_pstProcSockInfo.clientSock,&multiplexing);
    FD_SET(sockServer,&multiplexing);

    multiplexingBackup = multiplexing; /* Backup*/

    while( 1 )
    {
        multiplexing = multiplexingBackup;	/*-- Restore --*/

        if ( g_ForkExit == FAILURE)
        {
            break;
        }

        if( ( selectRes = fsock_Select((local_pstProcSockInfo.clientSock > sockServer ?
                                        local_pstProcSockInfo.clientSock : sockServer) + 1,
                                       &multiplexing, NULL, NULL,
                                       g_ForkExit) ) == (-1) )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Select Return Failed Break.. ");
            break;
        }

        if(selectRes > 0)
        {
            if(FD_ISSET(local_pstProcSockInfo.clientSock,&multiplexing))
            {
                /** CLIENT WAKES UP **/
                rxt = fstPreAuthSSL(local_pstProcSockInfo.sslClient);
                if(rxt <= 0)
                {
                    WRITE_INFO(CATEGORY_DEBUG,"-> THR[%d]fail: pre_auth", local_pstProcSockInfo.thrNum);
                    /* Error Msg */
                    ERR_error_string_n(ERR_get_error(), local_err_msg, sizeof(local_err_msg));
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Openssl Accept Failed msg  (%s) Protocol : [%s] ", local_err_msg, SSL_get_version(local_pstProcSockInfo.sslClient ));
                    WRITE_CRITICAL(CATEGORY_DEBUG,"SSL Accept Ret err(%d) msg(%s) State (%s)",
                                   errno, strerror(errno),
                                   SSL_state_string_long(local_pstProcSockInfo.sslClient));
                    break;
                }

                local_buffer = fcom_BufferAllocReturning(BUFFER_LEN, &bufferAllocResult);
                if( bufferAllocResult == 1 )
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: Fail in bufferAllocReturning, csock(%d)",
                               local_pstProcSockInfo.thrNum,
                               local_pstProcSockInfo.clientSock);
                    break;
                }
                memset( local_buffer, 0x00, BUFFER_LEN);
                bufferBackup    = local_buffer;

                byteRead = SSL_read(local_pstProcSockInfo.sslClient, local_buffer, BUFFER_LEN);
                if(byteRead < 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: byteRead(%d), csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                byteRead,
                                local_pstProcSockInfo.clientSock);

                    fcom_MallocFree((void**)&local_buffer);
                    break;
                }
                else if(byteRead == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by client, csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                local_pstProcSockInfo.clientSock);
                    fcom_MallocFree((void**)&local_buffer);
                    break;
                }

                /* PROXY has to send data to server */
                byteAlreadySent = 0;
                while( byteAlreadySent != byteRead )
                {
                    if ( byteAlreadySent > byteRead )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }

                    byteSent = fsock_SendSSL(sockServer, local_buffer,(byteRead-byteAlreadySent), &epipeError);
                    if( byteSent <= 0 && epipeError == 1 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (server-side), csock(%d) errno (%d) (%s)",
                                    local_pstProcSockInfo.thrNum,
                                    local_pstProcSockInfo.clientSock,
                                    errno,
                                    strerror(errno));
                        break;
                    }

                    if(byteSent < 0 && epipeError == 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: byteSent(%d)epipeError(%d), csock(%d) errno (%d) (%s)",
                                    local_pstProcSockInfo.thrNum,
                                    byteSent,
                                    epipeError,
                                    local_pstProcSockInfo.clientSock, errno, strerror(errno));
                        break;
                    }
                    local_buffer    += byteSent;
                    byteAlreadySent += byteSent;
                }
                fcom_MallocFree((void**)&bufferBackup);
                local_buffer = NULL;
            }

            if( FD_ISSET( sockServer, &multiplexing ) )
            {
                /** SERVER WAKES UP **/
                local_buffer = fcom_BufferAllocReturning(BUFFER_LEN,&bufferAllocResult);
                if(bufferAllocResult == 1)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: Fail in bufferAllocReturning, ssock(%d)",
                               local_pstProcSockInfo.thrNum, sockServer);
                    break;
                }
                memset( local_buffer, 0x00, BUFFER_LEN );

                byteRead = recv(sockServer, local_buffer, BUFFER_LEN, 0 );
                if(byteRead < 0)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: byteRead(%d), ssock(%d)",
                               local_pstProcSockInfo.thrNum, byteRead, sockServer);
                    fcom_MallocFree((void**)&local_buffer);
                    break;
                }
                else if(byteRead == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by server, ssock(%d)",
                                local_pstProcSockInfo.thrNum, sockServer);
                    fcom_MallocFree((void**)&local_buffer);
                    WRITE_INFO(CATEGORY_DEBUG,"-> THR[%d]END: closed by server", local_pstProcSockInfo.thrNum);
                    break;
                }

                strcpy(SHead.msgtype, MSG_DB_TYPE);
                SHead.msgcode  = htons(111);
                SHead.msgleng = htonl(byteRead);
                if(fcom_malloc((void**)&local_data, sizeof(SHead)+byteRead+1 ) != 0 )
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    fcom_MallocFree((void**) &local_buffer);
                    break;
                }
                dataBackup      = local_data;
                memcpy(local_data, &SHead, sizeof(CRhead));
                memcpy(&local_data[sizeof(CRhead)], local_buffer, byteRead);

                /*PROXY has to send data to server*/
                byteAlreadySent = 0;

                while( byteAlreadySent != ( (int)sizeof(CRhead) + byteRead) )
                {
                    if ( byteAlreadySent > (int)sizeof(CRhead) + byteRead )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }

                    byteSent = SSL_write(local_pstProcSockInfo.sslClient, local_data,  (sizeof(SHead) + byteRead) - byteAlreadySent );
                    if( byteSent <= 0 && epipeError == 1 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (client-side), ssock(%d) errno(%d) (%s) ",
                                    local_pstProcSockInfo.thrNum,sockServer, errno, strerror(errno));
                        break;
                    }

                    if(byteSent < 0 && epipeError == 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: byteSent(%d)epipeError(%d), ssock(%d)  errno(%d) (%s) ",
                                    local_pstProcSockInfo.thrNum,byteSent,epipeError,sockServer, errno, strerror(errno));
                        break;
                    }

                    byteAlreadySent += byteSent;
                    local_data      += byteSent;

                } //while

                fcom_MallocFree((void**)&dataBackup);
                fcom_MallocFree((void**)&local_buffer);
                local_buffer = NULL;
                local_data   = NULL;
            }
        }
    }//while

    pthread_mutex_lock( &g_pthread_Mutex );
    thrFlag[local_pstProcSockInfo.thrNum] = 0;
    pthread_mutex_unlock( &g_pthread_Mutex );
    fsock_CloseOpenssl(local_pstProcSockInfo.sslClient);
    fsock_CleanupOpenssl();
    fsock_Close(local_pstProcSockInfo.clientSock);
    fsock_Close(sockServer);

    pthread_exit(NULL);
}

void* fproxy_ThreadWorkd_NonEnc(void* Param_pstProcSockInfo)
{
    int		selectRes = 0;
    int		sockServer = 0;
    int 	reuseS = 1;
    int		byteAlreadySent = 0;
    int		bufferAllocResult = 0;
    int		byteRead = 0,byteSent = 0;
    int	 	epipeError = 0;
    const int     bufsize = 10240;
    int		rxt = 0;
    char local_err_msg[256] = {0x00,};


    void   *Buffer = NULL, *bufferBackup = NULL;
    char* 	*Data = NULL,  *dataBackup = NULL;
    CRhead  SHead;
    fd_set multiplexing, multiplexingBackup;
    _DAP_SOCK_PROXY_INFO local_pstProcSockInfo;

    memset(&local_pstProcSockInfo, 0x00, sizeof(_DAP_SOCK_PROXY_INFO));
    memset(&SHead, 0x00, sizeof(CRhead));

    memcpy(&local_pstProcSockInfo,(_DAP_SOCK_PROXY_INFO*)Param_pstProcSockInfo,sizeof(_DAP_SOCK_PROXY_INFO));

    WRITE_INFO(CATEGORY_DEBUG, "thread(%d:%d) work ", local_pstProcSockInfo.ClientIp, getpid());
    /**SETTING HANDLER TO NOT IGNORE SIGINT**/
    saSignInt.sa_handler=SIG_DFL;
    sigemptyset(&saSignInt.sa_mask);
    saSignInt.sa_flags=0;
    if(sigaction(SIGINT, &saSignInt, NULL)==-1)
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while sigaction");
        exit(FAILURE);
    }

    /**SOCKET CREATION**/
    sockServer = fsock_Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( sockServer < 0 )
    {
        WRITE_CRITICAL_IP(local_pstProcSockInfo.ClientIp, "Error while socket creation");
        exit(FAILURE);
    }

    /**SETSOCKOPT SO_REUSEADDR**/
    if(setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, &reuseS, sizeof(reuseS)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in socket option");
        exit(FAILURE);
    }

    /** 2021.04.09 ���ۻ���� 10240�̹Ƿ� �ʿ䰡 ������� �ּ�ó��. **/
    if(setsockopt(sockServer,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(sockServer,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_RCVBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    if(setsockopt(local_pstProcSockInfo.clientSock,SOL_SOCKET,SO_SNDBUF,(void *)&bufsize,sizeof(bufsize)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error on socket option.");
        exit(FAILURE);
    }

    /**PROXY has to connect to server**/
    if( fsock_Connect(sockServer, (SA*)&server, sizeof(server) ) !=0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in connect");
        fsock_Close(sockServer);
        exit(FAILURE);
    }

    WRITE_INFO(CATEGORY_INFO,"PID[%d]: Succeed in connected target, csock(%d)ssock(%d)cip(%s)",
               getpid(),
               local_pstProcSockInfo.clientSock,
               sockServer,
               local_pstProcSockInfo.ClientIp);


    FD_ZERO(&multiplexing);
    FD_SET(local_pstProcSockInfo.clientSock,&multiplexing);
    FD_SET(sockServer,&multiplexing);

    multiplexingBackup = multiplexing; /* Backup*/

    while( 1 )
    {
        multiplexing = multiplexingBackup;	/*-- Restore --*/

        if ( g_ForkExit == FAILURE)
        {
            break;
        }

        if( ( selectRes = fsock_Select((local_pstProcSockInfo.clientSock > sockServer ?
                                        local_pstProcSockInfo.clientSock : sockServer) + 1,
                                       &multiplexing, NULL, NULL,
                                       g_ForkExit) ) == (-1) )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Select Return Failed Break.. ");
            break;
        }

        if(selectRes > 0)
        {
            if(FD_ISSET(local_pstProcSockInfo.clientSock,&multiplexing))
            {
                char    msgType[10] = {0x00,};
                int  	msgLeng = 0;
                short	msgCode = 0;
                int nByteAlreadyRead = 0;

                // Header Recv
                CRhead RHead;
                memset((void *)&RHead, 0x00, sizeof(RHead));


                rxt = fsock_Recv(local_pstProcSockInfo.clientSock, (char *)&RHead, sizeof(RHead),0);
                if ( rxt <= 0){
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Recv Invalid Size (%d) Break", rxt);
                    break;
                }
                memset(msgType, 0x00, sizeof(msgType));
                strcpy(msgType, RHead.msgtype);
                msgLeng = ntohl(RHead.msgleng);
                msgCode = ntohs(RHead.msgcode);
                WRITE_DEBUG(CATEGORY_DEBUG,"Msg Type : [%s] Msg Leng : [%d] Msg Code : [%d] \n",
                            msgType, msgLeng, msgCode);

                /** CLIENT WAKES UP **/
                if(fcom_malloc((void**)&Buffer, msgLeng) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    break;
                }

                nByteAlreadyRead = 0;
                while(msgLeng != nByteAlreadyRead) {
                    if ( nByteAlreadyRead > msgLeng )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", nByteAlreadyRead);
                        break;
                    }
                    rxt = fsock_Recv(local_pstProcSockInfo.clientSock, (char *)&Buffer[nByteAlreadyRead], msgLeng - nByteAlreadyRead,0);
                    if ( rxt <= 0){
                        break;
                    }
                    nByteAlreadyRead += rxt;
                }

                if ( rxt < 0) {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: byteRead(%d), csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                byteRead,
                                local_pstProcSockInfo.clientSock);
                    fcom_MallocFree((void**)&Buffer);
                    break;
                }
                else if (rxt == 0){
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by client, csock(%d)",
                                local_pstProcSockInfo.thrNum,
                                local_pstProcSockInfo.clientSock);
                    fcom_MallocFree((void**)&Buffer);
                    break;
                }

                bufferBackup = Buffer;
                byteRead     = nByteAlreadyRead;

                WRITE_DEBUG(CATEGORY_DEBUG,"Send To Prepare : [%s] [%d] ", Buffer,byteRead);
                /* PROXY has to send data to server */
                byteAlreadySent = 0;
                while( byteAlreadySent != byteRead )
                {
                    if ( byteAlreadySent > byteRead )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }

                    byteSent = fsock_Send(sockServer, Buffer,byteRead - byteAlreadySent);
                    WRITE_DEBUG(CATEGORY_DEBUG,"Send To Maria : [%s] [%d] ", Buffer,byteSent);

                    if( byteSent < 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (server-side), csock(%d) errno (%d) (%s)",
                                    local_pstProcSockInfo.thrNum,
                                    local_pstProcSockInfo.clientSock,
                                    errno,
                                    strerror(errno));
                        break;
                    }
                    Buffer    += byteSent;
                    byteAlreadySent += byteSent;
                }
                fcom_MallocFree((void**)&bufferBackup);
                Buffer = NULL;
            }

            if( FD_ISSET( sockServer, &multiplexing ) )
            {
                /** SERVER WAKES UP **/
                if(fcom_malloc((void**)&Data, BUFFER_LEN) != 0)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: Fail in bufferAllocReturning, ssock(%d)",
                               local_pstProcSockInfo.thrNum, sockServer);
                    break;
                }

                byteRead = recv(sockServer, Data, BUFFER_LEN, 0 );
                if(byteRead < 0)
                {
                    WRITE_INFO(CATEGORY_INFO,"THR[%d]: byteRead(%d), ssock(%d)",
                               local_pstProcSockInfo.thrNum, byteRead, sockServer);
                    fcom_MallocFree((void**)&Data);
                    break;
                }
                else if(byteRead == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection closed by server, ssock(%d)",
                                local_pstProcSockInfo.thrNum, sockServer);
                    fcom_MallocFree((void**)&Data);
                    WRITE_INFO(CATEGORY_DEBUG,"-> THR[%d]END: closed by server", local_pstProcSockInfo.thrNum);
                    break;
                }

                dataBackup      = (char *)Data;

                memset(&SHead, 0x00, sizeof(SHead));
                strcpy(SHead.msgtype, MSG_DB_TYPE);
                SHead.msgcode  = htons(111);
                SHead.msgleng = htonl(byteRead);
                rxt = fsock_Send(local_pstProcSockInfo.clientSock, (char *)&SHead, sizeof(SHead));
                if(rxt < 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (client-side), ssock(%d) errno(%d) (%s) ",
                                local_pstProcSockInfo.thrNum,sockServer, errno, strerror(errno));
                    break;
                }
                WRITE_DEBUG(CATEGORY_DEBUG,"Send To Manager Header [%d] Body : [%d]", rxt,byteRead);

                /*PROXY has to send data to server*/
                byteAlreadySent = 0;
                while( byteAlreadySent != byteRead )
                {
                    if ( byteAlreadySent > byteRead )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid Size (%d) Break", byteAlreadySent);
                        break;
                    }

                    byteSent = fsock_Send(local_pstProcSockInfo.clientSock, Data,  byteRead - byteAlreadySent );
                    WRITE_DEBUG(CATEGORY_DEBUG,"Send To Manager Body [%s] [%d] ", Data, byteSent);
                    if( byteSent < 0 )
                    {
                        WRITE_DEBUG(CATEGORY_DEBUG,"THR[%d]: Connection lost (client-side), ssock(%d) errno(%d) (%s) ",
                                    local_pstProcSockInfo.thrNum,sockServer, errno, strerror(errno));
                        break;
                    }

                    byteAlreadySent += byteSent;
                    Data      += byteSent;
                } //while

                fcom_MallocFree((void**)&dataBackup);
                Data   = NULL;
            }
        }
    }//while

    pthread_mutex_lock( &g_pthread_Mutex );
    thrFlag[local_pstProcSockInfo.thrNum] = 0;
    pthread_mutex_unlock( &g_pthread_Mutex );
    fsock_Close(local_pstProcSockInfo.clientSock);
    fsock_Close(sockServer);

    pthread_exit(NULL);
}