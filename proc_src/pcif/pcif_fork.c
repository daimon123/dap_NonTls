//
// Created by KimByoungGook on 2020-10-13.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <string.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>



#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "json/dap_json.h"
#include "secure/dap_secure.h"

#include "pcif.h"


int fpcif_ForkWork(void)
{
    int		i = 0;
    int     nThreadIdx = 0;
    int     local_socket_mpt = 0;
    int     local_socket = 0;
    int     local_nForkIdx = 0;
    char        szTemp[64 +1] = {0x00,};
    pthread_t   pthreadloop;
    pthread_t   local_pthread_t ;
    char        local_accept_client_ipaddr[30+1] = {0x00,};
    struct proxy_conn  local_stThreadArg;
    struct sockaddr_in local_bind_sockaddr_in;

    memset(local_accept_client_ipaddr,  0x00, sizeof(local_accept_client_ipaddr));
    memset(&local_bind_sockaddr_in,     0x00, sizeof(struct sockaddr_in)        );
    memset(&local_stThreadArg,          0x00, sizeof(struct proxy_conn)         );

    g_ForkHangFlag = 0x00;
    local_nForkIdx = g_nPcifForkIdx;

    WRITE_INFO(CATEGORY_DEBUG,"Fork idx (%d) Created", g_nPcifForkIdx);

    /** 부모에서 등록한 SIGCHILD해제, system()함수 내부적으로 shell fork되면서 SIGCHILD 반횐된다. **/
    signal( SIGCHLD, SIG_IGN );

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "%s","Y");
    fcom_GetProfile("PCIF", "THREAD_HANG_CHECK_YN", g_stProcPcifInfo.cfgThreadHangCheck, szTemp);

    pthread_mutex_init(  &mutexsum,      NULL);
    pthread_mutex_init(  &mutexFile,     NULL);
    pthread_mutex_init(&g_pthread_Mutex, NULL);
    pthread_rwlock_init( &mutexpolicy,       NULL);

    sleep(1);

    /** File Write 처리 Init **/
    if ( fpcif_FileWriteInit( local_nForkIdx ) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG," File Write Init Failed ");
        return (-1);
    }

    /***************************************
    # OpenSSL Key File Check
    ****************************************/
    if (fcom_fileCheckStatus(g_stProcPcifInfo.certFullPath) != 0 ||
        fcom_fileCheckStatus(g_stProcPcifInfo.keyFullPath) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "OpenSSL KeyFile read error : %s : CertFile read %s not found. ",
                       g_stProcPcifInfo.certFullPath,
                       g_stProcPcifInfo.keyFullPath);
        close(local_socket);
        sleep(0);

        return (-1);
    }

    if(fcom_malloc((void**)&g_stThread_arg,sizeof(struct proxy_conn) * (g_stProcPcifInfo.cfgThreadCnt + 1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
    }

    signal(SIGPIPE, SIG_IGN);

    /* Thread Flag 초기화 */
    for(i = 0; i < MAX_THREAD; i++)
    {
        thrFlag[i] = 'N';
    }
    /* Thread 행체크 타임 초기화 */
    for(i = 0; i < MAX_THREAD; i++)
    {
        thrTimer[i] = time(NULL);
    }

    // Hang Check Thread
    if(fcom_ThreadCreate((void *)&pthreadloop, fpcif_ThreadLoop, (void *)NULL, g_stProcPcifInfo.nThreadStackSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fpcif_ThreadLoop Thread Create Fail " );
        return (-1);
    }

    while (1)
    {
        if(g_ForkHangFlag == 0x03)
        {
            while(1)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Is Hang Flag Is True.. Wait Finished Thread ");
                sleep (3);
            }
        }

        memset(&local_bind_sockaddr_in,      0x00, sizeof(struct sockaddr_in));
        memset( local_accept_client_ipaddr , 0x00, sizeof(local_accept_client_ipaddr));
        memset(&local_stThreadArg,           0x00, sizeof(struct proxy_conn));

        local_socket_mpt = sizeof(local_bind_sockaddr_in);
        if( (local_socket = accept ( g_stProcPcifInfo.listenSock , ( struct sockaddr *)&local_bind_sockaddr_in ,
                (socklen_t *)&local_socket_mpt )) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_INFO,"accept error / check bind port & ip address ");
        }
        else
        {
            pthread_mutex_lock( &g_pthread_Mutex );
            while(1)
            {
                nThreadIdx = fpcif_ThreadGetIdx(g_stProcPcifInfo.cfgThreadCnt);
                if (nThreadIdx >= 0)
                {
                    break;
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Is Full");
                    pthread_mutex_unlock( &g_pthread_Mutex );
                    exit(0);
                }
            }
            thrFlag[nThreadIdx]     = 'Y';
            thrTimer[nThreadIdx]    = time(NULL);
            fpcif_PrintThreadStatus(g_stProcPcifInfo.cfgThreadCnt);
            pthread_mutex_unlock( &g_pthread_Mutex );

            strcpy( local_accept_client_ipaddr , inet_ntoa(local_bind_sockaddr_in.sin_addr) );

            WRITE_DEBUG(CATEGORY_DEBUG,"Accept Success Client ip(%s) fd(%d) ", local_accept_client_ipaddr, local_socket );

            g_stThread_arg[nThreadIdx].cli_sock = local_socket;
            sprintf(g_stThread_arg[nThreadIdx].cli_ip,"%s",local_accept_client_ipaddr);
            g_stThread_arg[nThreadIdx].cli_thread = nThreadIdx;
            local_stThreadArg.cli_thread = nThreadIdx;
            g_stThread_arg[nThreadIdx].threadStepStatus = 0;
            g_stThread_arg[nThreadIdx].threadMsgCode = 0;

            sleep(0);

            if(fcom_ThreadCreate(&local_pthread_t, (void *)fpcif_PcifThread, &local_stThreadArg, g_stProcPcifInfo.nThreadStackSize) < 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"error thread can't create thread service create ");
                if(local_socket > 0)
                {
                    close(local_socket);
                    local_socket = 0;
                }

                pthread_mutex_lock( &g_pthread_Mutex );
                thrFlag[nThreadIdx] = 'N';
                pthread_mutex_unlock( &g_pthread_Mutex );

                fcom_SleepWait(5);
                sleep(0);
            }
            else
            {
                WRITE_DEBUG(CATEGORY_DEBUG,"thread service detach complete ");
            }

        }
        fcom_SleepWait(3);
    }

    ERR_remove_state(0);

    fdb_SqlClose(g_stMyCon);


    return 0;
}

int fpcif_ForkTask(void)
{
    int rxt = 0, loop = 0;
    int		cpId = 0;
    char	result[100] = {0x00,};
    char    local_szPidPath[127 +1]    = {0x00,};
    pthread_t local_Pthread_t;
    struct sigaction    saDeadChild;
    _MYSQL_CONINFO      local_stMyCon;
    char*   local_ptrProcessName = NULL;

    memset(&local_stMyCon,0x00, sizeof(_MYSQL_CONINFO));
    memset(&saDeadChild,  0x00, sizeof(saDeadChild));

    g_stProcPcifInfo.nListenPort = fsock_atoPort(g_stProcPcifInfo.pcPort, "tcp");
    if(g_stProcPcifInfo.nListenPort < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in system port(%d) ", g_stProcPcifInfo.nListenPort);
        exit(1);
    }
    rxt = fsock_MakeSocket(SOCK_STREAM,
                           g_stProcPcifInfo.nListenPort,
                           &g_stProcPcifInfo.listenSock,
                           result);
    if(rxt < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in make socket, port(%d)(%s) ",
                       g_stProcPcifInfo.pcPort,
                       result);
        close(g_stProcPcifInfo.listenSock);
        exit(1);
    }

    WRITE_INFO(CATEGORY_DEBUG,"Start Program Listen port : [%d] [%s] ",
               g_stProcPcifInfo.listenSock, g_stProcPcifInfo.pcPort );

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    saDeadChild.sa_handler = fpcif_SigchldHandler; // reap all dead processes
    sigemptyset(&saDeadChild.sa_mask);
    saDeadChild.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saDeadChild, NULL)==-1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit(1);
    }

    g_stProcPcifInfo.parentPid = getpid();

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PCIF);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    snprintf(g_stProcPcifInfo.caFullPath,
             sizeof(g_stProcPcifInfo.caFullPath),
             "%s/%s",
             g_stProcPcifInfo.certPath,
             g_stProcPcifInfo.caFile);
    snprintf(g_stProcPcifInfo.certFullPath,
             sizeof(g_stProcPcifInfo.certPath),
             "%s/%s",
             g_stProcPcifInfo.certPath,
             g_stProcPcifInfo.certFile);
    snprintf(g_stProcPcifInfo.keyFullPath,
             sizeof(g_stProcPcifInfo.keyFullPath),
             "%s/%s",
             g_stProcPcifInfo.certPath,
             g_stProcPcifInfo.keyFile);

    WRITE_INFO(CATEGORY_DEBUG, "caFullPath(%s) ",  g_stProcPcifInfo.caFullPath);
    WRITE_INFO(CATEGORY_DEBUG, "certFullPath(%s) ", g_stProcPcifInfo.certFullPath);
    WRITE_INFO(CATEGORY_DEBUG, "keyFullPath(%s) ", g_stProcPcifInfo.keyFullPath);

    g_nPcifForkIdx = 0;
    for(loop=0; loop < g_stProcPcifInfo.cfgForkCnt; loop++)
    {
        if((cpId = fork()) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
            continue;
        }

        else if(cpId > 0) //parent
        {
            g_stProcPcifInfo.ptrChildPid[loop] = cpId;
            /** Fork Process구별용 Index **/
            g_nPcifForkIdx++;
            continue;
        }
        // child process
        else if(cpId == 0)
        {
            rxt = fdb_ConnectDB();
            WRITE_INFO(CATEGORY_INFO,"fstMainTask - pid %d handle : %d",getpid(),&g_stMyCon);
            if( rxt < 0 )
            {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in db connection ");
                exit(0);
            }

            fpcif_ForkWork();
            exit(0);
        }
    }

    rxt = fdb_ConnectDB();
    if( rxt < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connection ");
        exit(0);
    }

    fcom_ThreadCreate(&local_Pthread_t, fpcif_MainThread, NULL, 4*1024*1024);

    while(1)
    {
        /* Nothing */
        sleep(1);
    }



    if(pUserInfo != NULL)
    {
        free(pUserInfo);  pUserInfo = NULL;
    }
    if(pGroupLink != NULL)
    {
        free(pGroupLink); pGroupLink = NULL;
    }
    if(pBaseInfo != NULL)
    {
        free(pBaseInfo);  pBaseInfo = NULL;
    }

    return 0;

}

void* fpcif_MainThread(void* param_ThreadArg)
{
    int		bFirstReload = 1;
//    int     nCurrMin = 0;
    time_t  local_nCurrTime = 0;
    time_t  local_nExecTimePrmon = 0;
    time_t  local_nExecTimeCheckNoti = 0;
    time_t  local_nExecTimeReload    = 0;

    while(1)
    {
        local_nCurrTime = time(NULL);

        if( difftime(local_nCurrTime, local_nExecTimePrmon) >= g_stProcPcifInfo.cfPrmonInterval)
        {
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcPcifInfo.last_send_time);
            local_nExecTimePrmon = time(NULL);
        }

        if(bFirstReload == 1 ||
           difftime(local_nCurrTime, local_nExecTimeCheckNoti) >= g_stProcPcifInfo.cfgCheckNotiInterval)
        {
            fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);
            bFirstReload = 0;
            local_nExecTimeCheckNoti = time(NULL);
        }

        if( difftime(local_nCurrTime, local_nExecTimeReload) >= 180)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcPcifInfo.nConfigLastModify) == 1)
            {
                fpcif_ReloadCfgFile();
            }

            local_nExecTimeReload = time(NULL);
        }
        sleep(1);
    }
}
