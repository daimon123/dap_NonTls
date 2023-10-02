//
// Created by KimByoungGook on 2020-06-23.
//

#include <stdio.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "ipc/dap_qdef.h"
#include "ipc/dap_Queue.h"
#include "db/dap_mysql.h"
#include "sock/dap_sock.h"

#include "db/dap_trandb.h"
#include "dblog.h"
#include "dap_version.h"

static void fstSigHandler();
static int fstDblogInit(void);
static void fstHandleSignal(int sid);
static void fstReloadCfgFile( );
static int fstMainTask();
static int fstFromQToDBMS();

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fstDblogInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fstSigHandler();

    fstMainTask();


    return RET_SUCC;

}

static int fstMainTask()
{
    int nRet;
    int nThrVal = 0;
    pthread_t PthreadRecv_t;
    char    local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName       = NULL;

    g_stProcDblogInfo.nCurTime =
    g_stProcDblogInfo.nLastJobTime =
    time(NULL);

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_DBLOG);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    nRet = fdb_ConnectDB();

    if(nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in db connection" );
        exit(0);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB,"Succeed in db connection" );
    }

    fdb_LsystemInit();

    WRITE_INFO(CATEGORY_IPC,"Start program rsltQsize[%d] saveFilePath[%s] ",
                             g_stProcDblogInfo.nCfgRsltQsize,
                             g_stServerInfo.stDapQueueInfo.szSaveDblogQueueHome);

    fipc_SQInit(g_stProcDblogInfo.nCfgRsltQsize, g_stServerInfo.stDapQueueInfo.szSaveDblogQueueHome);

    nRet = fipc_SQLoadQ();
    if(nRet == 0)
    {
        WRITE_INFO(CATEGORY_IPC,"Load Rslt Queue, nRet(%d) ", nRet);
        nRet = fipc_SQRemove();
    }
    else if(nRet > 0)
    {
        nRet = fipc_SQRemove();
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_IPC,"Failed to delete the queue " );
    }

    nRet = fcom_ThreadCreate(&PthreadRecv_t, fstDblogThread,(void*)&nThrVal, 4 * 1024 * 1024);
    if(nRet != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
        return -1;
    }

    fstFromQToDBMS();

    fstHandleSignal(15);

    WRITE_CRITICAL(CATEGORY_DEBUG,"Main Exited Status " );

    return RET_SUCC;


}

/** Queue에서 꺼내와서 Queue에 넣는다. **/
void* fstDblogThread(void* thrid)
{
    int			rxt;
    int			retryCnt;
    int         thrIdx;

    _DAP_QUEUE_BUF RsltQData;

    thrIdx    = *(int*)thrid;

    while (1)
    {
        memset((void *)&RsltQData,0x00,sizeof(RsltQData));

        if(g_pthreadStopFlag != 0x00)
        {
            break;
        }
        rxt = fipc_FQGetData(DBLOG_QUEUE, (char *)&RsltQData, sizeof(RsltQData));

        if(g_pthreadStopFlag != 0x00)
        {
            break;
        }
        if(rxt == 0)
        {
            fcom_SleepWait(3);
            continue;
        }

        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in receive, rxt(%d)",rxt );
            fcom_SleepWait(3);
            continue;
        }
        else
        {
            WRITE_INFO(CATEGORY_INFO,"Succeed in receive, rxt(%d)",rxt );
            rxt = fipc_SQPut(&RsltQData);
            if (rxt == -1)
            {
                WRITE_WARNING(CATEGORY_IPC,"Queue is full, current(%d)",
                        fipc_SQGetElCnt());

                for(retryCnt=1; retryCnt <= g_stProcDblogInfo.nCfgRetryQCount; retryCnt++)
                {
                    fcom_SleepWait(5);
                    rxt = fipc_SQPut(&RsltQData);
                    if (rxt == -1)
                    {
                        WRITE_CRITICAL(CATEGORY_IPC,"Queue is full, after sleep(%d/%d)",
                                retryCnt,  g_stProcDblogInfo.nCfgRetryQCount);
                    }
                    else
                    {
                        break;
                    }
                }
            } else
            {
                WRITE_INFO(CATEGORY_IPC,"Put queue current(%d) ",fipc_SQGetElCnt());

            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ",thrIdx);

    pthread_exit(NULL);

}

static int fstFromQToDBMS()
{
    int		rxt = 0;
    int		retryCnt = 0;
    int     nCurrMin = 0;
    time_t  nChkTime = 0;
    char	prefix[6 +1] = {0x00,};
    char	postfix[4 +1] = {0x00,};
    char    histTableName[50];
    _DAP_DB_SERVERLOG_INFO local_stServerLogData;
    _DAP_DB_AGENTLOG_INFO  local_stAgentLogData;
    _DAP_QUEUE_BUF         RsltQData;

    while(1)
    {
        g_stProcDblogInfo.nIngJobTime = time(NULL);
        nCurrMin = fcom_GetSysMinute();

        memset((void *)&RsltQData,0x00,sizeof(RsltQData));  //정상데이터
        memset((void *)&local_stServerLogData, 0x00, sizeof(local_stServerLogData));
        memset((void *)&local_stAgentLogData,  0x00, sizeof(local_stAgentLogData) );

        if(difftime(g_stProcDblogInfo.nIngJobTime,nChkTime) > g_stProcDblogInfo.nCfgPrmonInterval)
        {
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcDblogInfo.nLastSendTime);
            nChkTime = time(NULL);
        }

        if((nCurrMin % 3) == 0)
        {
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile,&g_stProcDblogInfo.nCfgLastModify) == 1)
            {
                fstReloadCfgFile();
            }
        }

        if(fipc_SQGet((_DAP_QUEUE_BUF *)&RsltQData) == NULL)  // 큐가 비어있으면
        {
//            if(g_stProcDblogInfo.nLastJobTime - g_stProcDblogInfo.nIngJobTime == 0 &&
//               pre_type == PROCESS_ERROR_LOG)
//            {
//                WRITE_CRITICAL(CATEGORY_DEBUG,"Try in commit db because queue is null" );
//                pre_type = 0;
//            }

            fcom_SleepWait(4);
//            sleep(1);
            fstKeepSession(); // 세션 유지
            continue;
        }
        else
        {
            g_stProcDblogInfo.nLastJobTime = time(NULL);
//            pre_type = RsltQData.packtype;
            WRITE_INFO(CATEGORY_IPC, "Get queue current(%d)", fipc_SQGetElCnt());
        }

        memset(prefix, 0x00, sizeof(prefix));
        memset(postfix, 0x00, sizeof(postfix));

        fdb_GetHistoryDbTime(prefix, 0);
        fdb_GetHistoryTableTime(postfix, NULL, 0);

        switch(RsltQData.packtype)
        {
            case PROCESS_SERVER_LOG:
            {
                WRITE_INFO(CATEGORY_DB,"[DQUE](%d) PROCESS_SERVER_LOG ",fipc_SQGetElCnt() );
                memset(&local_stServerLogData, 0x00, sizeof(local_stServerLogData));
                memcpy((void *)&local_stServerLogData, (void *)&RsltQData.buf, sizeof(local_stServerLogData));

                WRITE_INFO(CATEGORY_DB,
                           "pid : %d, \nprocess : %s, \nip : %s \nerrorlevel : %s \n"
                           "msg : %s \nlogtime : %s \n",
                           local_stServerLogData.pid,
                           local_stServerLogData.process,
                           local_stServerLogData.logip,
                           local_stServerLogData.loglevel,
                           local_stServerLogData.logmsg,
                           local_stServerLogData.logdate);

                memset(histTableName, 0x00, sizeof(histTableName));

                fdb_GetHistoryTableName("SERVER_LOG_TB", histTableName, prefix, postfix);

                rxt = fdb_InsertServerErrorlogHistory(&local_stServerLogData, histTableName);
                if(rxt < 0)
                {
                    WRITE_INFO(CATEGORY_IPC,"Retry put queue " );
                    while(retryCnt < g_stProcDblogInfo.nCfgRetryQCount)
                    {
                        fcom_Msleep(500);
                        rxt = fipc_SQPut(&RsltQData);
                        if(rxt < 0)
                        {
                            retryCnt++;
                        }
                        else
                        {
                            break;
                        }
                        if(retryCnt == g_stProcDblogInfo.nCfgRetryQCount)
                        {
                            WRITE_CRITICAL(CATEGORY_IPC,"Fail in put queue, retry(%d/%d) ",
                                           retryCnt, g_stProcDblogInfo.nCfgRetryQCount);
                        }
                    }
                }

                break;
            }

            case PROCESS_AGENT_LOG:
            {
                WRITE_INFO(CATEGORY_DB,"[DQUE](%d) PROCESS_AGENT_LOG ",fipc_SQGetElCnt() );
                memset(&local_stAgentLogData, 0x00, sizeof(local_stAgentLogData));
                memcpy((void *)&local_stAgentLogData, (void *)&RsltQData.buf, sizeof(local_stAgentLogData));

                WRITE_INFO(CATEGORY_DB,
                           "hbsq : %s \nuser_key : %s \nip : %s \nprocess : %s \n"
                           "time : %s \nlevel : %s \nmsg : %s",
                           local_stAgentLogData.hbsq,
                           local_stAgentLogData.hbunq,
                           local_stAgentLogData.ip,
                           local_stAgentLogData.process,
                           local_stAgentLogData.logdate,
                           local_stAgentLogData.loglevel,
                           local_stAgentLogData.logmsg);

                memset(histTableName, 0x00, sizeof(histTableName));

                fdb_GetHistoryTableName("AGENT_LOG_TB", histTableName, prefix, postfix);

                rxt = fdb_InsertServerErrorlogHistory(&local_stServerLogData, histTableName);
                if(rxt < 0)
                {
                    WRITE_INFO(CATEGORY_IPC,"Retry put queue " );
                    while(retryCnt < g_stProcDblogInfo.nCfgRetryQCount)
                    {
                        fcom_Msleep(500);
                        rxt = fipc_SQPut(&RsltQData);
                        if(rxt < 0)
                        {
                            retryCnt++;
                        }
                        else
                        {
                            break;
                        }
                        if(retryCnt == g_stProcDblogInfo.nCfgRetryQCount)
                        {
                            WRITE_CRITICAL(CATEGORY_IPC,"Fail in put queue, retry(%d/%d) ",
                                           retryCnt, g_stProcDblogInfo.nCfgRetryQCount);
                        }
                    }
                }

                break;
            }

            default:
            {
                WRITE_CRITICAL(CATEGORY_IPC,"Unknown errorlog type(%d)", RsltQData.packtype);
                break;
            }
        }

        fcom_SleepWait(5); // 0.1
    }

    return 0;
}

void fstKeepSession()
{
    int rxt;

    g_stProcDblogInfo.nCurTime = time(NULL);

    if ((g_stProcDblogInfo.nCurTime - g_stProcDblogInfo.nLastJobTime) > g_stProcDblogInfo.nCfgKeepSession)
    {
        g_stProcDblogInfo.nLastJobTime = time(NULL);
        rxt = fdb_PutSessionQuery();
        if (rxt !=  0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in session query" );
        }
    }
}


static int fstDblogInit(void)
{
    _DAP_COMN_INFO* pstComnInfo;
    _DAP_QUEUE_INFO* pstQueueInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")              );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"             );


    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    g_stProcDblogInfo.nCfgRsltQsize = fcom_GetProfileInt("DBLOG","SQ_SIZE",100);
    g_stProcDblogInfo.nCfgRetryQCount = fcom_GetProfileInt("DBLOG","SQ_RETRY_LIMIT_COUNT",3);
    g_stProcDblogInfo.nCfgKeepSession = fcom_GetProfileInt("DBLOG","KEEP_SESSION",5);
    g_stProcDblogInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL",60);

    /* Queue Info Init */
    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome),
            "%s/.DAPQ",
            pstComnInfo->szDapHome);

    if (access(pstQueueInfo->szDAPQueueHome, R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",
                   pstQueueInfo->szDAPQueueHome,
                    __func__ );

            fstHandleSignal(15);
        }
    }

    snprintf(pstQueueInfo->szSaveDblogQueueHome,
                sizeof(pstQueueInfo->szSaveDblogQueueHome),
                "%s/%s/SAVEQDBLOG",
                pstComnInfo->szDapHome,
                "back");

    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
            "%s/PRMONQ",
             pstQueueInfo->szDAPQueueHome);


    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",pstQueueInfo->szPrmonQueueHome,__func__ );
        fstHandleSignal(15);
    }

    snprintf(pstQueueInfo->szDblogQueueHome,
                sizeof(pstQueueInfo->szDblogQueueHome),
                "%s/DBLOGQ",
                pstQueueInfo->szDAPQueueHome);


    if (fipc_FQGetInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",
                       pstQueueInfo->szDblogQueueHome,
                       __func__ );
        fstHandleSignal(15);
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
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_dblog");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_dblog");

    printf("Succeed in init, pid(%d)qsize(%d)|%s\n", getpid(), g_stProcDblogInfo.nCfgRsltQsize, __func__);

    fstSigHandler();

    return TRUE;

}

static void fstSigHandler()
{
    signal( SIGHUP, fstHandleSignal);    /* 1 : hangup */
    signal( SIGINT, fstHandleSignal);    /* 2 : interrupt (rubout) */
    signal( SIGQUIT, fstHandleSignal);   /* 3 : quit (ASCII FS) */
    signal( SIGILL, fstHandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP, fstHandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT, fstHandleSignal);    /* 6 : IOT instruction */
    signal( SIGABRT, fstHandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal( SIGFPE, fstHandleSignal);    /* 8 : floating point exception */
    signal( SIGKILL , fstHandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS , fstHandleSignal);   /* 10: bus error */
    signal( SIGSEGV, fstHandleSignal);   /* 11: segmentation violation */
    signal( SIGSYS , fstHandleSignal);   /* 12: bad argument to system call */
    signal( SIGPIPE, fstHandleSignal);   /* 13: write on a pipe with no one to read it */
    signal( SIGALRM, fstHandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fstHandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fstHandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fstHandleSignal);   /* 17: user defined signal 2 */
    signal( SIGCLD, SIG_IGN );              /* 18: child status change */
    signal( SIGCHLD, SIG_IGN );             /* 18: child status change alias (POSIX) */
    signal( SIGPWR , fstHandleSignal);   /* 19: power-fail restart */
    signal( SIGWINCH, SIG_IGN );            /* 20: window size change */
    signal( SIGURG  , fstHandleSignal);  /* 21: urgent socket condition */
    signal( SIGPOLL , fstHandleSignal);  /* 22: pollable event occured */
    signal( SIGIO   , fstHandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP , fstHandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal( SIGTSTP , fstHandleSignal);  /* 24: user stop requested from tty */
    signal( SIGCONT , fstHandleSignal);  /* 25: stopped process has been continued */
    signal( SIGTTIN , fstHandleSignal);  /* 26: background tty read attempted */
    signal( SIGTTOU , fstHandleSignal);  /* 27: background tty write attempted */
    signal( SIGVTALRM , fstHandleSignal);/* 28: virtual timer expired */
    signal( SIGPROF , fstHandleSignal);  /* 29: profiling timer expired */
    signal( SIGXCPU , fstHandleSignal);  /* 30: exceeded cpu limit */
    signal( SIGXFSZ , fstHandleSignal);  /* 31: exceeded file size limit */
}

static void fstHandleSignal(int sid)
{
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName    = NULL;

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_DBLOG);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Stop Status");

    if ( g_pthreadStopFlag == 0x00 )
    {
        g_pthreadStopFlag = 0x01;
    }

    sleep(2);

    fdb_SqlClose(g_stMyCon);

    fipc_SQSaveQ();

    fipc_SQClear();
    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);


    WRITE_CRITICAL(CATEGORY_DEBUG,"Exit program, received signal(%d)pid(%d)",
                   sid, getpid());

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(0);

}


static void fstReloadCfgFile( )
{
    /* Alarm Set Reload */
    g_stProcDblogInfo.nCfgPrmonInterval             = fcom_GetProfileInt("PRMON","INTERVAL",60);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);

    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcDblogInfo.nCfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}
