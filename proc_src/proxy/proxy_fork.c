//
// Created by KimByoungGook on 2021-04-09.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <pthread.h>
#include <sys/errno.h>
#include <sys/poll.h>

#include "com/dap_com.h"
#include "sock/dap_sock.h"

#include "proxy.h"
#include "dap_version.h"

static int fproxy_GetThreadIdx(void);

static int fproxy_GetThreadIdx(void)
{
    int local_nLoop = 0;

    for ( local_nLoop = 0; g_stProcProxyInfo.cfgThreadCnt; local_nLoop++ )
    {
        if(thrFlag[local_nLoop] == 0)
        {
            return local_nLoop;
        }
    }

    return (-1);
}

int fproxy_ForkWork()
{
    int		pollRet = 0;
    int     nRet = 0;
    int     nPthreadIdx = 0;
    int     Loop = 0;
    int     nCurrMin = 0;
    char	tmpIp[15 +1] = {0x00,};
    struct 	pollfd fds;
    _DAP_SOCK_PROXY_INFO stProcSockInfo;
    int pid;

    fds.fd = g_listenSock;
    fds.events = POLLIN;

    pid = getpid();
    memset(&stProcSockInfo, 0x00, sizeof(_DAP_SOCK_PROXY_INFO));

    /* thrFlag
     * 0 : Unuse
     * 1 : Use */
    for(Loop=0; Loop < g_stProcProxyInfo.cfgThreadCnt; Loop++)
    {
        thrFlag[Loop] = 0;
    }

    pthread_mutex_init(&mutexsum, NULL);
    pthread_mutex_init(&g_pthread_Mutex, NULL);


    for(;;)
    {
        if(g_ForkExit == FAILURE)
        {
            break;
        }

        nCurrMin = fcom_GetSysMinute();

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcProxyInfo.nConfigLastModify) == 1)
            {
                fproxy_ReloadCfgFile();
            }
        }

        pollRet = poll(&fds, 1, 60);
        if(pollRet == 0)
        {
            fcom_SleepWait(5);
            continue;
        }
        else if(pollRet < 0)
        {
            if(errno != EINTR)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"PID[%d]: Fail in poll, return(%d) ",
                               pid,pollRet);
            }
            fcom_SleepWait(5);
            continue;
        }
        stProcSockInfo.clientSock = fsock_Accept(g_listenSock);
        WRITE_INFO(CATEGORY_INFO,"PID[%d]: Accept, sock(%d) ",pid, stProcSockInfo.clientSock);

        if(stProcSockInfo.clientSock < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"PID[%d]: Fail in socket connect, sock(%d) ",
                           pid, stProcSockInfo.clientSock);
            nRet = FAILURE;
        }

        nRet = SUCCESS;
        if(nRet == SUCCESS)
        {
            memset(tmpIp, 0x00, sizeof(tmpIp));
            fsock_GetPeerAddr(stProcSockInfo.clientSock, tmpIp);
            strcpy(stProcSockInfo.ClientIp,tmpIp);

            stProcSockInfo.sslClient = SSL_new(g_stProcProxyInfo.ctx);
            SSL_set_fd(stProcSockInfo.sslClient, stProcSockInfo.clientSock);
            if(SSL_accept(stProcSockInfo.sslClient) <= 0)
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"PID[%d]: Fail in SSL_accept, sock(%d)",
                            pid,stProcSockInfo.clientSock);

                if(stProcSockInfo.sslClient != NULL)
                {
                    fsock_CloseOpenssl(stProcSockInfo.sslClient);
                }
                fsock_CleanupOpenssl();
                fsock_Close(stProcSockInfo.clientSock);
                fcom_SleepWait(5);
                continue;
            }
            WRITE_DEBUG(CATEGORY_DEBUG,"PID[%d]: SSL connection using(%s)(%s)",
                        pid,
                        SSL_get_cipher(stProcSockInfo.sslClient),
                        SSL_CIPHER_get_name(SSL_get_current_cipher(stProcSockInfo.sslClient)));
            fsock_OpensslLibShowCerts(stProcSockInfo.sslClient);

            WRITE_DEBUG(CATEGORY_DEBUG,"Succeed in accept, pid(%d)cip(%s)csock(%d)",
                        pid, tmpIp, stProcSockInfo.clientSock);

            pthread_mutex_lock( &g_pthread_Mutex );
            nPthreadIdx = fproxy_GetThreadIdx();
            WRITE_DEBUG(CATEGORY_DEBUG,"Check Thread Flag [%d]", nPthreadIdx);
            if ( nPthreadIdx < 0 )
            {
                pthread_mutex_unlock( &g_pthread_Mutex );
                WRITE_CRITICAL(CATEGORY_DEBUG,"Get Thread Idx Failed");
                fsock_Close(stProcSockInfo.clientSock);
                fcom_SleepWait(5);
                continue;
            }
            thrFlag[nPthreadIdx] = 1; // Thread Flag 사용중으로 변경
            stProcSockInfo.thrNum = nPthreadIdx;
            pthread_mutex_unlock( &g_pthread_Mutex );

            // Trust Channel
//            fcom_ThreadCreate(&pthreadrecv[nPthreadIdx], fproxy_ThreadWorkd, (void *)&stProcSockInfo, 4 * 1024 * 1024);

            // Non Enc
//            fcom_ThreadCreate(&pthreadrecv[nPthreadIdx], fproxy_ThreadWorkd_NonEnc, (void *)&stProcSockInfo, 4 * 1024 * 1024);

            // OpenSSL
            fcom_ThreadCreate(&pthreadrecv[nPthreadIdx], fproxy_ThreadWorkd_SSL, (void *)&stProcSockInfo, 4 * 1024 * 1024);

        }

        sleep(0);
        fcom_SleepWait(5); //0.1 sec
    }

    fcom_MallocFree((void**)&g_ChildPid);

    return RET_SUCC;

}
