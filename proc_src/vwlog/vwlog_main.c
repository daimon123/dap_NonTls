//
// Created by KimByoungGook on 2020-06-22.
//
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "com/dap_com.h"
#include "com/dap_req.h"

#include "sock/dap_sock.h"

#include "ipc/dap_Queue.h"
#include "db/dap_checkdb.h"


#include "vwlog.h"

static int fstVwlogInit(void);

static int fstSigHandler(void);
static void fstReloadCfgFile( );
static void fstHandleSignal(int sid);
static int fstMainTask();
static int fstFromQToTail();

pthread_t PthreadRecvT;

int main(int argc, char**argv)
{
    fcom_ArgParse(argv);

    fstVwlogInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fstMainTask();

    return RET_SUCC;
}

static int fstMainTask()
{
    int nRet = 0, nThrArg = 0;
    char local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_VWLOG);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    WRITE_INFO(CATEGORY_INFO,"Start Program " );

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    nThrArg = 0;

    fipc_SQInit(g_stProcVwlogInfo.nCfgRsltQsize, g_stProcVwlogInfo.szCfgSaveFilePath);
    nRet = fipc_SQLoadQ();

    if(nRet == 0)
    {
        WRITE_CRITICAL(CATEGORY_IPC,"Load rslt queue, ret(%d)",nRet );
        nRet = fipc_SQRemove();

    }
    else if(nRet > 0)
    {
        nRet = fipc_SQRemove();
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_IPC, "Fail in delete queue" );
    }

    if(fcom_ThreadCreate( (void *)&PthreadRecvT, fstThreadRecv, (void *)&nThrArg, 4 * 1024 * 1024) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
        return -1;
    }

    nRet = fstFromQToTail();

    WRITE_CRITICAL(CATEGORY_DEBUG,"VwLog Program Exited(%d)",nRet );

    fstHandleSignal(15);
    fipc_SQRemove();

    return 0;

}

void* fstThreadRecv(void *thrid)
{
    int			rxt;
    int         thrIdx;
    int			retryCnt;

    _DAP_QUEUE_BUF  TailQData;

    thrIdx = *(int*)thrid;

    while (1)
    {
        if ( g_pthreadStopFlag != 0x00 )
        {
            break;
        }

        memset((void *)&TailQData, 0x00, sizeof(TailQData));
        rxt = fipc_FQGetData(TAIL_QUEUE, (char *)&TailQData, sizeof(TailQData));
        if(rxt == 0)
        {
            sleep(1);
            continue;
        }
        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in receive, rxt(%d)",rxt );
            sleep(1);
            continue;
        }
        else
        {
            WRITE_INFO(CATEGORY_IPC,"Succeed in receive, rxt(%d)", rxt );

            rxt = fipc_SQPut(&TailQData);
            if (rxt == -1)
            {
                WRITE_CRITICAL(CATEGORY_IPC,"Queue is full, current(%d)",fipc_SQGetElCnt() );
                for(retryCnt=1; retryCnt <= g_stProcVwlogInfo.nCfgRetryQCount; retryCnt++)
                {
                    sleep(1);

                    rxt = fipc_SQPut(&TailQData);
                    if (rxt == -1)
                    {
                        WRITE_CRITICAL(CATEGORY_IPC, "Queue is full, after sleep(%d/%d)",
                                retryCnt,
                                g_stProcVwlogInfo.nCfgRetryQCount);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                WRITE_CRITICAL(CATEGORY_IPC,"Put queue current(%d)",fipc_SQGetElCnt() );
            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ",thrIdx);

    pthread_exit(NULL);

}

void fstCleanTail(char* clientIp, char* clientPath)
{
    int     pid = 0;
    char    cmd[256];
    char    buff[10];
    FILE*   fp = NULL;

    memset  (cmd, 0x00, sizeof(cmd));
    sprintf (cmd, "ps -eo pid,ppid,comm,cmd |grep dap_tail|grep %s|grep %s|grep -v grep|awk '{print $1}'", clientIp,clientPath);


    fp = popen(cmd, "r");
    if( fp == NULL )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in popen, cmd(%s)",cmd );
        return;
    }

    memset(buff, 0x00, sizeof(buff));
    while(fgets(buff, 10, fp) != NULL)
    {
        pid = atoi(buff);

        WRITE_DEBUG(CATEGORY_DEBUG,"Clean tail pid(%d)",pid );
//        if( kill(pid, SIGTERM) < 0 )
        if( kill(pid, SIGTERM) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Can't kill process ip(%s)pasth(%s), pid(%d)sig(%d)",
                    clientIp,
                    clientPath,
                    pid,
                    SIGTERM);
        }
        else
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Killed process ip(%s)path(%s), pid(%d)sig(%d)",
                    clientIp,
                    clientPath,
                    pid,
                    SIGTERM);
        }
    }
    pclose(fp);

}

static int fstFromQToTail()
{

    int     nCurrMin;
    char	cmd[256];

    _DAP_QUEUE_BUF  TailQData;
    _DAP_TAIL_INFO 	TailInfo;

    while(1)
    {
        g_stProcVwlogInfo.nIngJobTime = time(NULL);
        nCurrMin = fcom_GetSysMinute();

        memset((void *)&TailQData, 0x00, sizeof(TailQData));


        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcVwlogInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }


        if( (g_stProcVwlogInfo.nIngJobTime % g_stProcVwlogInfo.nCfgPrmonInterval) == 0 ) //foreach 1min
        {
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcVwlogInfo.nLastSendTime);

        }

        if(fipc_SQGet((_DAP_QUEUE_BUF *)&TailQData) == NULL) // 큐가 비어있으면
        {
            sleep(1);
            continue;
        }
        else
        {
            g_stProcVwlogInfo.nLastJobTime = time(NULL);

            WRITE_INFO(CATEGORY_IPC,"Get queue current(%d)",
                    fipc_SQGetElCnt());
        }

        if( TailQData.packtype == MANAGE_REQ_SERVER_LOGTAIL ||
            TailQData.packtype == MANAGE_FORWARD_SERVER_LOGTAIL )
        {
            if( TailQData.packtype == MANAGE_REQ_SERVER_LOGTAIL )
            {
                WRITE_INFO(CATEGORY_DEBUG,"(%d) MANAGE_REQ_SERVER_LOGTAIL",fipc_SQGetElCnt() );
            }
            else
            {
                WRITE_INFO(CATEGORY_DEBUG,"(%d) MANAGE_FORWARD_SERVER_LOGTAIL",fipc_SQGetElCnt() );
            }

            memset((void *)&TailInfo, 0x00, sizeof(TailInfo));
            memcpy((void *)&TailInfo, (void *)TailQData.buf, sizeof(TailInfo));

            WRITE_DEBUG(CATEGORY_DEBUG,"Tail PAth : [%s],manager_fq : [%d] ", TailInfo.tail_path, TailInfo.manager_fq);
            sprintf	(cmd, "%s/bin/dap_tail %s %s %d %s &", g_stServerInfo.stDapComnInfo.szDapHome,
                                TailInfo.server_ip,TailInfo.manager_ip,TailInfo.manager_fq,TailInfo.tail_path);
            system(cmd);
            WRITE_DEBUG(CATEGORY_DEBUG,"Tail [%s] ",cmd);
        }
        else
        {
            WRITE_MAJOR(CATEGORY_DEBUG,"Unknown tail queue type(%d)",TailQData.packtype );
        }
    }

    return 0;
}

static int fstVwlogInit(void)
{
    _DAP_COMN_INFO* pstComnInfo;
    _DAP_QUEUE_INFO* pstQueueInfo;
    _DAP_CERT_INFO* pstCertInfo;
    char local_szTmp[255 +1]        = {0x00,};
    char local_szDefaultIp[15 +1]   = {0x00,};

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;
    pstCertInfo = &g_stServerInfo.stDapCertInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")      );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                          ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return RET_FAIL;
    }

    fstSigHandler();

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    pstComnInfo->nCfgMgwId = fcom_GetProfileInt("COMMON","SERVER_ID",1);

    fsock_GetNic(local_szTmp);
    fsock_GetIpAddress(local_szTmp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );

    fcom_GetProfile("SSL","CERT_PATH",pstCertInfo->szCertPath,"/home/intent/dap/config/cert");
    fcom_GetProfile("SSL","CA_FILE",pstCertInfo->szCaFile,"root.crt");
    fcom_GetProfile("SSL","CERT_FILE",pstCertInfo->szCertFile,"server.crt");
    fcom_GetProfile("SSL","KEY_FILE",pstCertInfo->szKeyFile,"server.key");
    g_stProcVwlogInfo.nCfgRsltQsize = fcom_GetProfileInt("TAIL","SQ_SIZE",10);
    g_stProcVwlogInfo.nCfgRetryQCount = fcom_GetProfileInt("TAIL","SQ_RETRY_LIMIT_COUNT",3);
    snprintf(g_stProcVwlogInfo.szCfgSaveFilePath, sizeof(g_stProcVwlogInfo.szCfgSaveFilePath),
             "%s/%s/SAVEQTAIL", pstComnInfo->szDapHome, "back");

    g_stProcVwlogInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 60);

    sprintf(pstQueueInfo->szDAPQueueHome, "%s/.DAPQ", pstComnInfo->szDapHome);
    if (access(pstQueueInfo->szDAPQueueHome,R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",pstQueueInfo->szDAPQueueHome,__func__ );
            fstHandleSignal(15);
        }
    }

    memset(pstQueueInfo->szPrmonQueueHome, 0x00, sizeof(pstQueueInfo->szPrmonQueueHome));
    sprintf(pstQueueInfo->szPrmonQueueHome, "%s/PRMONQ", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",pstQueueInfo->szPrmonQueueHome, __func__ );
        fstHandleSignal(15);
    }
    memset(pstQueueInfo->szTailQueueHome, 0x00, sizeof(pstQueueInfo->szTailQueueHome));
    sprintf(pstQueueInfo->szTailQueueHome,"%s/TAILQ", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQGetInit(TAIL_QUEUE, pstQueueInfo->szTailQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",pstQueueInfo->szTailQueueHome, __func__ );
        fstHandleSignal(15);
    }

    snprintf(pstQueueInfo->szDblogQueueHome,
             sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
             pstQueueInfo->szDAPQueueHome);
    if(fipc_FQPutInit(DBLOG_QUEUE,pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)", pstQueueInfo->szDblogQueueHome);
        exit(0);
    }

//    /* 프로세스.pid 디렉토리 검사 */
//    sprintf(g_stServerInfo.stDapComnInfo.szPidPath,"%s%s",g_stServerInfo.stDapComnInfo.szDapHome,"/bin/var");
//    if(access(g_stServerInfo.stDapComnInfo.szPidPath, W_OK) != 0 )
//    {
//        if (mkdir(g_stServerInfo.stDapComnInfo.szPidPath,S_IRWXU|S_IRWXG|S_IRWXO) != 0)
//        {
//            printf("Fail in Pid File directory(%s)|%s\n",
//                   g_stServerInfo.stDapComnInfo.szPidPath,
//                   __func__ );
//            return 0;
//        }
//    }
//
//    /* 프로세스.pid 파일검사 */
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_vwlog");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_vwlog");

    printf("Succeed in init, pid(%d)qsize(%d)|%s\n",getpid(),g_stProcVwlogInfo.nCfgRsltQsize, __func__ );

    return RET_SUCC;
}

static void fstHandleSignal(int sid)
{
    int     status = 0;
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;


    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_VWLOG);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    if ( g_pthreadStopFlag == 0x00 )
    {
        g_pthreadStopFlag = 0x01;
    }

    sleep(2);

    WRITE_CRITICAL(CATEGORY_DEBUG,"Exit program, received signal [%d],pid [%d]",sid, getpid());
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    fipc_FQClose(DBLOG_QUEUE);
    fipc_SQClear();
    fipc_SQRemove();


    pthread_join(PthreadRecvT,(void **)&status);

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(0);
}

static int fstSigHandler(void)
{
    signal( SIGCLD, SIG_IGN   );                 /* 18: child status change */
    signal( SIGWINCH, SIG_IGN );                 /* 20: window size change */
    signal( SIGHUP,    SIG_IGN        );   /* 1 : hangup */
    signal( SIGINT,    fstHandleSignal   );   /* 2 : interrupt (rubout) */
    signal( SIGQUIT,   fstHandleSignal   );   /* 3 : quit (ASCII FS) */
    signal( SIGILL,    fstHandleSignal   );   /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP,   fstHandleSignal   );   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT,    fstHandleSignal   );   /* 6 : IOT instruction */
    signal( SIGABRT,   fstHandleSignal   );   /* 6 : used by abort,replace SIGIOT in the future */
//    signal( SIGEMT,    fstHandleSignal   );   /* 7 : EMT instruction */
    signal( SIGFPE,    fstHandleSignal   );   /* 8 : floating point exception */
    signal( SIGKILL ,  fstHandleSignal   );   /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS ,   fstHandleSignal   );   /* 10: bus error */
    signal( SIGSEGV,   fstHandleSignal   );   /* 11: segmentation violation */
    signal( SIGSYS ,   fstHandleSignal   );   /* 12: bad argument to system call */
    signal( SIGPIPE,   fstHandleSignal   );   /* 13: write on a pipe with no one to read it */
    signal( SIGALRM,   fstHandleSignal   );   /* 14: alarm clock */
    signal( SIGTERM,   fstHandleSignal   );   /* 15: software termination signal from kill */
    signal( SIGUSR1,   fstHandleSignal   );   /* 16: user defined signal 1 */
    signal( SIGUSR2,   fstHandleSignal   );   /* 17: user defined signal 2 */
    signal( SIGPWR ,   fstHandleSignal   );   /* 19: power-fail restart */
    signal( SIGURG  ,  fstHandleSignal   );   /* 21: urgent socket condition */
    signal( SIGPOLL ,  fstHandleSignal   );   /* 22: pollable event occured */
    signal( SIGIO   ,  fstHandleSignal   );   /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP ,  fstHandleSignal   );   /* 23: stop (cannot be caught or ignored) */
    signal( SIGTTIN ,  fstHandleSignal   );   /* 26: background tty read attempted */
    signal( SIGTTOU ,  fstHandleSignal   );   /* 27: background tty write attempted */
    signal( SIGVTALRM, fstHandleSignal   );   /* 28: virtual timer expired */
    signal( SIGPROF ,  fstHandleSignal   );   /* 29: profiling timer expired */
    signal( SIGXCPU ,  fstHandleSignal   );   /* 30: exceeded cpu limit */
    signal( SIGXFSZ ,  fstHandleSignal   );   /* 31: exceeded file size limit */

    return RET_SUCC;

}

static void fstReloadCfgFile( )
{
   /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n "
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}