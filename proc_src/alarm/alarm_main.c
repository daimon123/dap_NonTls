//
// Created by KimByoungGook on 2020-06-23.
//

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>


#include "com/dap_def.h"
#include "com/dap_com.h"
#include "com/dap_req.h"

#include "sock/dap_sock.h"

#include "ipc/dap_Queue.h"
#include "db/dap_mysql.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "alram.h"
#include "dap_version.h"



int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fstAlarmInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    WRITE_INFO(CATEGORY_INFO,"Start Program | %s",__func__ );
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    fstSigHandler();

    fstMainTask();

    return RET_SUCC;

}



static int fstMainTask()
{
    int nRet;
    int nThrval = 0;
    pthread_t Pthreadrecv_t         = NULL;
    char*   local_ptrProcessName    = NULL;

    char local_szPidPath[127 +1]    = {0x00,};

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_ALARM);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);


    pAlarmInfo = (_DAP_DB_ALARM_INFO*)alarmInfo;

    fipc_SQInit(g_stProcAlarmInfo.nCfgRsltQsize,
                g_stServerInfo.stDapQueueInfo.szSaveAlarmQueueHome);

    nRet = fipc_SQLoadQ();
    if(nRet == 0)
    {
        WRITE_INFO(CATEGORY_DEBUG,"Load rslt queue, nRet(%d)",
                nRet);
        nRet = fipc_SQRemove();
    }
    else if(nRet > 0)
    {
        nRet = fipc_SQRemove();
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in delete queue" );
    }


    if(fcom_ThreadCreate((void *)&Pthreadrecv_t, fstAlarmThread, (void *)&nThrval, 4 * 1024 * 1024) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
        return -1;
    }

    fstFromQToAlarm();

    fstHandleSignal(15);

    return RET_SUCC;

}

static void fstSigHandler()
{
    signal(SIGHUP, fstHandleSignal);    /* 1 : hangup */
    signal(SIGINT, fstHandleSignal);    /* 2 : interrupt (rubout) */
    signal(SIGQUIT, fstHandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL, fstHandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP, fstHandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT, fstHandleSignal);    /* 6 : IOT instruction */
    signal(SIGABRT, fstHandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGFPE, fstHandleSignal);    /* 8 : floating point exception */
    signal(SIGKILL, fstHandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal(SIGBUS, fstHandleSignal);   /* 10: bus error */
    signal(SIGSEGV, fstHandleSignal);   /* 11: segmentation violation */
    signal(SIGSYS, fstHandleSignal);   /* 12: bad argument to system call */
    signal(SIGPIPE, fstHandleSignal);   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM, fstHandleSignal);   /* 14: alarm clock */
    signal(SIGTERM, fstHandleSignal);   /* 15: software termination signal from kill */
    signal(SIGUSR1, fstHandleSignal);   /* 16: user defined signal 1 */
    signal(SIGUSR2, fstHandleSignal);   /* 17: user defined signal 2 */
    signal(SIGCLD, SIG_IGN);              /* 18: child status change */
    signal(SIGCHLD, SIG_IGN);             /* 18: child status change alias (POSIX) */
    signal(SIGPWR, fstHandleSignal);   /* 19: power-fail restart */
    signal(SIGWINCH, SIG_IGN);            /* 20: window size change */
    signal(SIGURG, fstHandleSignal);  /* 21: urgent socket condition */
    signal(SIGPOLL, fstHandleSignal);  /* 22: pollable event occured */
    signal(SIGIO, fstHandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal(SIGSTOP, fstHandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal(SIGTSTP, fstHandleSignal);  /* 24: user stop requested from tty */
    signal(SIGCONT, fstHandleSignal);  /* 25: stopped process has been continued */
    signal(SIGTTIN, fstHandleSignal);  /* 26: background tty read attempted */
    signal(SIGTTOU, fstHandleSignal);  /* 27: background tty write attempted */
    signal(SIGVTALRM, fstHandleSignal);/* 28: virtual timer expired */
    signal(SIGPROF, fstHandleSignal);  /* 29: profiling timer expired */
    signal(SIGXCPU, fstHandleSignal);  /* 30: exceeded cpu limit */
    signal(SIGXFSZ, fstHandleSignal);  /* 31: exceeded file size limit */
}

void* fstAlarmThread(void *thrid)
{
    int			rxt;
    int			retryCnt;
    int         thrIdx;

    thrIdx  = *(int*)thrid;

    _DAP_QUEUE_BUF AlarmQData;

    while (1)
    {
        if ( g_pthreadStopFlag != 0x00 )
        {
            break;
        }

        // Wait for the first run to complete the memory load.
        if(g_stProcAlarmInfo.bFirstReloadFinish != 1)
        {
            usleep(5000);
            continue;
        }

        memset((void *)&AlarmQData, 0x00, sizeof(AlarmQData));
        rxt = fipc_FQGetData(ALARM_QUEUE, (char *)&AlarmQData, sizeof(AlarmQData));

        if ( g_pthreadStopFlag != 0x00 )
        {
            break;
        }

        if(rxt == 0)
        {
            sleep(1);
            continue;
        }
        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in receive, rxt(%d)",
                    rxt);
            sleep(1);
            continue;
        }
        else
        {
            WRITE_INFO(CATEGORY_INFO,"Succeed in receive, rxt(%d)",rxt );

            rxt = fipc_SQPut(&AlarmQData);
            if (rxt == -1)
            {

                WRITE_INFO(CATEGORY_IPC,"Queue is full, current(%d)",
                        fipc_SQGetElCnt());

                for(retryCnt=1; retryCnt <= g_stProcAlarmInfo.nCfgRetryQCount; retryCnt++)
                {
                    sleep(1);
                    rxt = fipc_SQPut(&AlarmQData);
                    if (rxt == -1)
                    {
                        WRITE_WARNING(CATEGORY_IPC,"Queue is full, after sleep(%d/%d)",
                                retryCnt,
                                g_stProcAlarmInfo.nCfgRetryQCount);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                WRITE_INFO(CATEGORY_IPC,"Put queue current(%d)",fipc_SQGetElCnt() );
            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ", thrIdx);

    pthread_exit(NULL);

}

int fstFromQToAlarm()
{
    int			bFirstReload = 1;
    int         curMin = 0;
    char		readBuf[1024+1] = {0x00,};
    char*		delayMsg    = NULL;
    FILE*		rfp         = NULL;

    _DAP_QUEUE_BUF AlarmQData;
    _DAP_EventParam EventParam;

    while(1)
    {
        g_stProcAlarmInfo.nIngJobTime = time(NULL);
        curMin = fcom_GetSysMinute();

        memset((void *)&AlarmQData, 0x00, sizeof(AlarmQData));

        if((curMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcAlarmInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }


        if((g_stProcAlarmInfo.nIngJobTime % g_stProcAlarmInfo.nCfgAlarmNotiInterval) == 0)
        {
            if(bFirstReload == 1)
            {
                if( fdb_ConnectDB() >= 0 )
                {
                    fstReloadConfig();
                    fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);

                    g_stProcAlarmInfo.bFirstReloadFinish = 1;
                }
                else
                {
                    bFirstReload = 1;
                    g_stProcAlarmInfo.bFirstReloadFinish = 0;
                }
            }
            else
            {
                fstReloadConfig();
                fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);
            }
            fstCheckNotiFile();
            bFirstReload = 0;

        }

        if( (g_stProcAlarmInfo.nIngJobTime % g_stProcAlarmInfo.nCfgPrmonInterval) == 0 ) //foreach 1min
        {
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcAlarmInfo.nLastSendTime);
        }

        if( g_stProcAlarmInfo.nCfgSendMailDelayTime > 0 &&
           (g_stProcAlarmInfo.nIngJobTime % g_stProcAlarmInfo.nCfgSendMailDelayTime) == 0 )
        {

            if(fcom_malloc((void**)&delayMsg, sizeof(char)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            rfp = fopen(g_stProcAlarmInfo.szCfgSendMailTempPath, "rb");
            if( rfp != NULL )
            {
                flock(fileno(rfp), LOCK_SH);
                while(fgets(readBuf, 1024, rfp) != NULL)
                {
                    WRITE_INFO(CATEGORY_DEBUG,"readBuf: %s ",
                                readBuf);

                    delayMsg = (char *)realloc(delayMsg, sizeof(char)*(strlen(delayMsg)+strlen(readBuf)+1));
                    sprintf(delayMsg+strlen(delayMsg), "%s", readBuf);
                }
                delayMsg[strlen(delayMsg)-1] = 0x00; // 마지막 \n을 뺀다.
                flock(fileno(rfp), LOCK_UN);
                fclose(rfp);

                if( unlink(g_stProcAlarmInfo.szCfgSendMailTempPath) )
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in unlink mail file(%s)error(%s)",
                            g_stProcAlarmInfo.szCfgSendMailTempPath,
                            strerror(errno));
                    usleep(5000);

                    if( remove(g_stProcAlarmInfo.szCfgSendMailTempPath) )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in remove mail file(%s)error(%s)",
                                g_stProcAlarmInfo.szCfgSendMailTempPath,
                                strerror(errno));
                    }
                }

                if( strlen(delayMsg) > 5 )
                {
                    fstSendDelayMail(delayMsg);
                }
            }
            fcom_MallocFree((void**)&delayMsg);
        }

        if(fipc_SQGet((_DAP_QUEUE_BUF *)&AlarmQData) == NULL) // 큐가 비어있으면
        {
            sleep(1);
            continue;
        }
        else
        {
            g_stProcAlarmInfo.nLastJobTime = time(NULL);
            WRITE_INFO(CATEGORY_INFO,"Get queue current(%d)",
                    fipc_SQGetElCnt());
        }

        if(AlarmQData.packtype == EVENT_ALARM)
        {
            WRITE_INFO(CATEGORY_INFO,"[DQUE](%d) EVENT_ALARM ",
                    fipc_SQGetElCnt());


            memset((void *)&EventParam, 0x00, sizeof(EventParam));
            memcpy((void *)&EventParam, (void *)AlarmQData.buf, sizeof(EventParam));


            WRITE_INFO(CATEGORY_INFO,"EventParam.user_key   : %s ",
                    EventParam.user_key );

            WRITE_INFO(CATEGORY_INFO,"EventParam.user_ip   : %s ",
                       EventParam.user_ip );

            WRITE_INFO(CATEGORY_INFO,"EventParam.detect_time   : %s ",
                       EventParam.detect_time );

            WRITE_INFO(CATEGORY_INFO,"EventParam.ev_type   : %d",
                       EventParam.ev_type );

            WRITE_INFO(CATEGORY_INFO,"EventParam.ev_level   : %d",
                       EventParam.ev_level );

            WRITE_INFO(CATEGORY_INFO,"EventParam.prefix   : %s ",
                       EventParam.prefix );

            WRITE_INFO(CATEGORY_INFO,"EventParam.postfix   : %s",
                       EventParam.postfix );

            fstCheckSmsAlarm(&EventParam);
            fstCheckMailAlarm(&EventParam);

        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Unknown alarm queue type(%d) ",
                    AlarmQData.packtype);
        }
    }

    return 0;
}

int fstAlarmInit()
{
    int nRetVal, i;
    char szCfgTmp[256+1] = {0x00,};
    char local_szDefaultIP[15 +1] = {0x00,};

    _DAP_COMN_INFO* pstComnInfo;
    _DAP_QUEUE_INFO* pstQueueInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"             );


    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        nRetVal = (-1);
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);


    pstComnInfo->nCfgMgwId  = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    fsock_GetNic(szCfgTmp);
    fsock_GetIpAddress(szCfgTmp, local_szDefaultIP);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIP );
    g_stProcAlarmInfo.nCfgRsltQsize = fcom_GetProfileInt("ALARM","SQ_SIZE",10);
    g_stProcAlarmInfo.nCfgRetryQCount = fcom_GetProfileInt("ALARM","SQ_RETRY_LIMIT_COUNT",3);
    g_stProcAlarmInfo.nCfgSendMailDelayTime = fcom_GetProfileInt("ALARM","SEND_MAIL_DELAY_TIME",0);
    g_stProcAlarmInfo.nCfgSendMailDelayTime = g_stProcAlarmInfo.nCfgSendMailDelayTime * 60; //min to sec

    fcom_GetProfile("ALARM","SEND_MAIL_TEMP_PATH",g_stProcAlarmInfo.szCfgSendMailTempPath,"/home/intent/dap/script/alarm/mail");
    g_stProcAlarmInfo.nCfgKeepSession = fcom_GetProfileInt("MYSQL","KEEP_SESSION",5);
    g_stProcAlarmInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 600);

    if (access(g_stProcAlarmInfo.szCfgSendMailTempPath, W_OK) != 0 )
    {
        nRetVal = fcom_MkPath(g_stProcAlarmInfo.szCfgSendMailTempPath, 0755);
        if (nRetVal < 0)
        {
            printf("Fail in make path(%s)|%s\n",g_stProcAlarmInfo.szCfgSendMailTempPath, __func__ );
            return -1;
        }
        else
        {
            chmod(g_stProcAlarmInfo.szCfgSendMailTempPath, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
            printf("Succeed in make path(%s)|%s\n",g_stProcAlarmInfo.szCfgSendMailTempPath, __func__ );
        }
    }
    strcat(g_stProcAlarmInfo.szCfgSendMailTempPath, "/sendmail.dat");

    g_stProcAlarmInfo.nCfgAlarmNotiInterval = fcom_GetProfileInt("ALARM","CHECK_NOTI_INTERVAL",10);


    if(fcom_malloc((void**)&g_pstNotiMaster, sizeof(_CONFIG_NOTI)) != 0)
    {
        printf("fcom_malloc Failed |%s\n",__func__ );
        exit(0);
    }


    snprintf(szCfgTmp, sizeof(szCfgTmp), "%s/config/alarm_change",pstComnInfo->szDapHome);
    fcom_GetProfile("COMMON","ALARM_CHANGE",g_pstNotiMaster[ALARM_CHANGE].szNotiFileName,szCfgTmp);

    for(i = 0; i < MAX_ALARM_NOTI; i++)
    {
        g_pstNotiMaster[i].lastModify    =   0;
        g_pstNotiMaster[i].reload        =   TRUE;
    }

    snprintf(pstQueueInfo->szSaveAlarmQueueHome,
            sizeof(pstQueueInfo->szSaveAlarmQueueHome),
             "%s/%s/SAVEQALARM",
             pstComnInfo->szDapHome,
             "back");

    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ",
             pstComnInfo->szDapHome);

    if (access(pstQueueInfo->szDAPQueueHome,R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",
                           pstQueueInfo->szDAPQueueHome,
                           __func__ );
            fstHandleSignal(15);
        }
    }

    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
             "%s/PRMONQ",
             pstQueueInfo->szDAPQueueHome);

    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",pstQueueInfo->szPrmonQueueHome,
                __func__ );

        fstHandleSignal(15);
    }

    snprintf(pstQueueInfo->szAlaramQueueHome,
            sizeof(pstQueueInfo->szAlaramQueueHome),
             "%s/ALARMQ",
             pstQueueInfo->szDAPQueueHome);


    if (fipc_FQGetInit(ALARM_QUEUE, pstQueueInfo->szAlaramQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n",
                pstQueueInfo->szAlaramQueueHome,
                __func__ );

        fstHandleSignal(15);
    }

    snprintf(pstQueueInfo->szDblogQueueHome, sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
             pstQueueInfo->szDAPQueueHome);

    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)|%s\n",
                       pstQueueInfo->szDblogQueueHome,
                       __func__);

        fstHandleSignal(15);
    }

    g_stProcAlarmInfo.nCurTime =
    g_stProcAlarmInfo.nLastJobTime =
    time(NULL);

    g_stProcAlarmInfo.bFirstReloadFinish = 0;

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
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_alarm");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_alarm");

    printf("Succeed in init, pid(%d)qsize(%d) |%s\n",
            getpid(),
            g_stProcAlarmInfo.nCfgRsltQsize,
            __func__ );

    return TRUE;

}

void fstHandleSignal(int sid)
{
    int     rxt = 0;
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_ALARM);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Stop Status");
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    if( g_pthreadStopFlag == 0x00 )
    {
        g_pthreadStopFlag = 0x01;
    }

    sleep(2);

    rxt = fipc_SQSaveQ();

    WRITE_WARNING(CATEGORY_IPC, "Try save queue, rxt(%d)",rxt );

    fipc_SQClear();

    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);
    fipc_FQClose(ALARM_QUEUE);

    fdb_SqlClose(g_stMyCon);

    WRITE_WARNING(CATEGORY_DEBUG, "Exit program, received signal(%d)pid(%d)",sid, getpid());

    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
    }

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(0);

}


int fstReloadConfig(void)
{
    int rxt;
    struct stat     statBuf;

    if(g_pstNotiMaster[ALARM_CHANGE].reload)
    {
        memset(pAlarmInfo, 0x00, sizeof(_DAP_DB_ALARM_INFO));
        if( (g_stProcAlarmInfo.nAlarm = falarm_LoadAlarm(pAlarmInfo)) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load_alarm" );
            return  -4;
        }


        if( (rxt = falarm_GetAlarmInfo( g_stProcAlarmInfo.szConfSmsServerIp,
                                      g_stProcAlarmInfo.nConfSmsServerPort,
                                      g_stProcAlarmInfo.szConfSmsFrom,
                                      g_stProcAlarmInfo.szConfSmsLang,
                                      g_stProcAlarmInfo.szConfMailFrom,
                                      g_stProcAlarmInfo.szConfMailLang)) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in get_alarm_info" );
            return  -4;
        }

        stat(g_pstNotiMaster[ALARM_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[ALARM_CHANGE].lastModify    =   statBuf.st_mtime;
        g_pstNotiMaster[ALARM_CHANGE].reload        =   FALSE;
    }

    return TRUE;
}


static int fstCheckNotiFile(void)
{
    register int i;
    struct stat     statBuf;

    for(i = 0; i < MAX_ALARM_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {

            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in stat noti files(%s)(%d)",
                    g_pstNotiMaster[i].szNotiFileName,
                    i);

            printf("%s\n", strerror(errno));
            return  -1;
        }

        if(statBuf.st_mtime > g_pstNotiMaster[i].lastModify)
        {
            g_pstNotiMaster[i].reload = TRUE;
        }
        else
        {
            g_pstNotiMaster[i].reload = FALSE;
        }
    }

    return TRUE;
}

static int fstCheckSmsAlarm(_DAP_EventParam *p_EP)
{
    int				i;
    int				tokenCnt;
    unsigned char	msg[90] = {0x00,};
    char			tmpBuf[100] = {0x00,};
    char			cellPhone[15] = {0x00,};
    char			strEvLevel[10] = {0x00,};
    char			strEvType[30] = {0x00,};
    char*			tokenPhone = NULL;

    WRITE_INFO(CATEGORY_INFO,"start .. " )


    fcom_SetCustLang(g_stProcAlarmInfo.szConfSmsLang);



    for( i=0; i < g_stProcAlarmInfo.nAlarm; i++ )
    {

        if( pAlarmInfo[i].al_detect_type == p_EP->ev_type &&
            (pAlarmInfo[i].al_detect_level-'0')%48 <= p_EP->ev_level )
        {
            if( ( (pAlarmInfo[i].al_use-'0')%48 == 1 || (pAlarmInfo[i].al_use-'0')%48 == 3 ) &&
                pAlarmInfo[i].mn_cell_phone != NULL ) // SMS
            {

                memset(strEvLevel, 0x00, sizeof(strEvLevel));
                memset(strEvType, 0x00, sizeof(strEvType));

                fcom_GetStrLevel(p_EP->ev_level, strEvLevel);
                fcom_GetStrType2(p_EP->ev_type, strEvType, g_stProcAlarmInfo.szConfMailLang);

                tokenCnt = fcom_TokenCnt(pAlarmInfo[i].mn_cell_phone, ",");

                if( tokenCnt > 0 )
                {
                    memset(tmpBuf, 0x00, sizeof(tmpBuf));
                    strcpy(tmpBuf, pAlarmInfo[i].mn_cell_phone);
                    tokenPhone = strtok(tmpBuf, ",");
                    while( tokenPhone != NULL )
                    {
                        if( fcom_IsNumber(tokenPhone) && strlen(tokenPhone) > 9 )
                        {
                            memset	(cellPhone, 0x00, sizeof(cellPhone));

                            fcom_ReplaceAll(tokenPhone, "-", "", cellPhone);
                            memset	(msg, 0x00, sizeof(msg));
                            sprintf	(msg, "%s,%s [%d:EVENT](%s)%s %s",
                                    cellPhone,
                                    p_EP->detect_time,
                                    g_stServerInfo.stDapComnInfo.nCfgMgwId,
                                    strEvLevel,
                                    p_EP->user_ip,
                                    strEvType);
                            /* 주석처리 되어있음. */
//                            send_sms(sockSms, msg);
                        }
                        else
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid phone number(%s)",
                                    tokenPhone);
                        }
                        tokenPhone = strtok(NULL, ",");
                    }
                }
                else
                {
                    if( fcom_IsNumber(pAlarmInfo[i].mn_cell_phone) && (int)strlen(pAlarmInfo[i].mn_cell_phone) > 9 )
                    {
                        memset	(cellPhone, 0x00, sizeof(cellPhone));

                        fcom_ReplaceAll(pAlarmInfo[i].mn_cell_phone, "-", "", cellPhone);
                        memset	(msg, 0x00, sizeof(msg));
                        sprintf	(msg, "%s,%s [%d:EVENT](%s)%s %s",
                                cellPhone,
                                p_EP->detect_time,
                                g_stServerInfo.stDapComnInfo.nCfgMgwId,
                                strEvLevel,
                                p_EP->user_ip,
                                strEvType);
                        /* sms 처리 주석처리 되어있음. */
//                        send_sms(sockSms, msg);
                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid phone number(%s) ",pAlarmInfo[i].mn_cell_phone );
                    }
                }
            }
        }
    }

    WRITE_INFO(CATEGORY_DEBUG,"%s finction end . ","SMS_ALARM" );

    return 0;
}

static void fstReloadCfgFile( )
{
    /* Alarm Set Reload */
    g_stProcAlarmInfo.nCfgAlarmNotiInterval        = fcom_GetProfileInt("ALARM","CHECK_NOTI_INTERVAL",10);
    g_stProcAlarmInfo.nCfgPrmonInterval            = fcom_GetProfileInt("PRMON","INTERVAL", 600);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n CHECK_NOTI_INTERVAL : [%d] \n"
                              "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                              "MAX_LOG_KEEP_DAY : [%d] ",
                g_stProcAlarmInfo.nCfgAlarmNotiInterval,
                g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
                g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
                g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
                g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}

static int fstCheckMailAlarm(_DAP_EventParam *p_EP)
{
    int				i;
    int				tokenCnt;
    char			tmpBuf[100];
    char*			tokenMail = NULL;
    unsigned char	msg[90];
    char			strEvLevel[10];
    char			strEvType[30];
    FILE*			wfp = NULL;

    if( g_stProcAlarmInfo.nCfgSendMailDelayTime > 0 )
    {
        wfp = fopen(g_stProcAlarmInfo.szCfgSendMailTempPath, "a+, ccs=UTF-8");
        if(wfp == NULL)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fopen error(%s) ",strerror(errno) );

            return -1;
        }
    }


    // make msg
    memset	(strEvLevel, 0x00, sizeof(strEvLevel));
    memset	(strEvType, 0x00, sizeof(strEvType));
    fcom_GetStrLevel(p_EP->ev_level, strEvLevel);
    fcom_GetStrType2(p_EP->ev_type, strEvType,g_stProcAlarmInfo.szConfMailLang);

    memset	(msg, 0x00, sizeof(msg));
    sprintf	(msg, "<li>%s [%d:EVENT](%s)%s %s</li>",
                (char *)p_EP->detect_time,
                g_stServerInfo.stDapComnInfo.nCfgMgwId,
                strEvLevel,
                p_EP->user_ip,
                strEvType);

    for( i=0; i < g_stProcAlarmInfo.nAlarm; i++ )
    {
        if( pAlarmInfo[i].al_detect_type == p_EP->ev_type &&
            (pAlarmInfo[i].al_detect_level-'0')%48 <= p_EP->ev_level )
        {
            if( ( (pAlarmInfo[i].al_use-'0')%48 == 2 || (pAlarmInfo[i].al_use-'0')%48 == 3 ) &&
                pAlarmInfo[i].mn_email != NULL ) // MAIL
            {
                tokenCnt = fcom_TokenCnt(pAlarmInfo[i].mn_email, ",");
                if( tokenCnt > 0 )
                {
                    memset(tmpBuf, 0x00, sizeof(tmpBuf));
                    strcpy(tmpBuf, pAlarmInfo[i].mn_email);
                    tokenMail = strtok(tmpBuf, ",");
                    while( tokenMail != NULL )
                    {
                        if( strstr(tokenMail, "@") != NULL && strlen(tokenMail) > 5 )
                        {
                            if( g_stProcAlarmInfo.nCfgSendMailDelayTime <= 0 )
                            {
                                fstSendMail(tokenMail, msg);
                            }
                        }
                        else
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s) ",tokenMail );
                        }
                        tokenMail = strtok(NULL, ",");
                    }
                }
                else
                {
                    if( strstr(pAlarmInfo[i].mn_email, "@") != NULL && strlen(pAlarmInfo[i].mn_email) > 5 )
                    {
                        if( g_stProcAlarmInfo.nCfgSendMailDelayTime <= 0 )
                        {
                            fstSendMail(pAlarmInfo[i].mn_email, msg);
                        }

                    }
                    else
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s) ",
                                pAlarmInfo[i].mn_email);
                    }
                }
            }
        }
    } // for

    if( g_stProcAlarmInfo.nCfgSendMailDelayTime > 0 )
    {
        //lock = flock(wfp, LOCK_SH);
        fprintf(wfp, "%s\n", msg);
        WRITE_INFO(CATEGORY_INFO,"[PUT] Mail msg(%s)",msg);
        //release = flock(wfp, LOCK_UN);
        fclose(wfp);
    }


    return 0;
}
static int fstSendMail(char* mailTo, char* msg)
{
    char command[8192];

    memset(command, 0x00, sizeof(command));
    /*sprintf(command, "%s/bin/sendalarm.sh %s %s %s \"%s\"",
            dapHomeDir,
            confMailFrom,
            mailTo,
            confMailLang,
            msg);*/

    sprintf(command, "%s/bin/sendalarm.sh %s %s %s \"%s\"",
            g_stServerInfo.stDapComnInfo.szDapHome,
            g_stProcAlarmInfo.szConfMailFrom,
            mailTo,
            g_stProcAlarmInfo.szConfMailLang,
            msg);

    WRITE_INFO(CATEGORY_INFO,"[MAIL] from(%s)to(%s)lang(%s)msg(%s)",
            g_stProcAlarmInfo.szConfMailFrom,
            mailTo,
            g_stProcAlarmInfo.szConfMailLang,
            msg);

    system(command);

    return 0;
}


static int fstSendDelayMail(char* delayMsg)
{
    int 	rxt = 0;
    int 	i = 0;
    int		tokenCnt = 0;
    int		nLevel = 0;
    char	strType[30 +1] = {0x00,};
    char	strLevel[10 +1] = {0x00,};
    char	strPrevMail[64 +1] = {0x00,};
    char	tmpPrevMail[64 +1] = {0x00,};
    char*	tokenMsg = NULL;
    char*	tokenMail = NULL;
    char*	tmpDelayMsg = NULL;
    char* 	mergeMsg = NULL;
    char* 	mergeTotMsg = NULL;

    if(fcom_malloc((void**)&mergeTotMsg, sizeof(char)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    memset(strPrevMail, 0x00, sizeof(strPrevMail));

    for( i=0; i<g_stProcAlarmInfo.nAlarm; i++ )
    {
        if( ( 	(pAlarmInfo[i].al_use-'0')%48 == 2 ||
                 (pAlarmInfo[i].al_use-'0')%48 == 3	) &&
            pAlarmInfo[i].mn_email != NULL )
        {
            WRITE_INFO(CATEGORY_INFO,"Check mail, curr(%s)prev(%s) ",
                    pAlarmInfo[i].mn_email,
                    strPrevMail);

            // put prev mail
            if( strncmp(strPrevMail, pAlarmInfo[i].mn_email, strlen(pAlarmInfo[i].mn_email)) != 0 ) // 다르면
            {
                if( strlen(mergeTotMsg) > 0 )
                {
                    tokenCnt = fcom_TokenCnt(strPrevMail, ",");

                    if( tokenCnt > 0 )
                    {
                        memset(tmpPrevMail, 0x00, sizeof(tmpPrevMail));
                        strcpy(tmpPrevMail, strPrevMail);
                        tokenMail = strtok(tmpPrevMail, ",");
                        while( tokenMail != NULL ) {
                            if( strstr(tokenMail, "@") != NULL && strlen(tokenMail) > 5 )
                            {
//                                send_mail(tokenMail, mergeTotMsg);

                            }
                            else
                            {
                                WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s)",
                                        tokenMail);
                            }
                            tokenMail = strtok(NULL, ",");
                        }
                    }
                    else
                    {
                        if( strstr(strPrevMail, "@") != NULL && strlen(strPrevMail) > 5 )
                        {
                            fstSendMail(strPrevMail, mergeTotMsg);
                        }
                        else
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s)",
                                    strPrevMail);
                        }
                    }
                    // init
                    mergeTotMsg = (char *)realloc(mergeTotMsg, sizeof(char)*1);
                    memset(mergeTotMsg, 0x00, sizeof(char)*1);
                }
                strcpy(strPrevMail, pAlarmInfo[i].mn_email);
            }

            memset	(strType, 0x00, sizeof(strType));

            fcom_GetStrType2(pAlarmInfo[i].al_detect_type, strType, g_stProcAlarmInfo.szConfMailLang);
            tokenCnt = fcom_TokenCnt(delayMsg, "\n");

            if( tokenCnt > 0 )
            {

                if(fcom_malloc((void**)&mergeMsg, sizeof(char)) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }

                if(fcom_malloc((void**)&tmpDelayMsg, sizeof(char)*(strlen(delayMsg)+1)) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }

                strcpy(tmpDelayMsg, delayMsg);
                tokenMsg = strtok(tmpDelayMsg, "\n");
                while( tokenMsg != NULL )
                {
                    memset	(strLevel, 	0x00, sizeof(strLevel));
                    rxt = fcom_GetTagValue(tokenMsg, "(", ")", strLevel, sizeof(strLevel) );
//                    rxt = fcom_GetTagValue(tokenMsg, "(", ")", strLevel  );
                    if( rxt < 0 )
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid str level(%s), forced set level(INFO) ",
                                        strLevel );
                        strcpy(strLevel, "INFO");
                    }
                      nLevel = fcom_GetNumberLevel(strLevel);

                    if( strstr(tokenMsg, strType)	!= NULL &&
                        (pAlarmInfo[i].al_detect_level-'0')%48 <= nLevel )
                    {
                        mergeMsg = (char *)realloc(mergeMsg, sizeof(char)*(strlen(mergeMsg)+strlen(tokenMsg)+1));
                        sprintf(mergeMsg+strlen(mergeMsg), "%s", tokenMsg);
                        //LogRet("-> [GET] token mergeMsg: %s\n", mergeMsg);
                    }
                    tokenMsg = strtok(NULL, "\n");
                }

                fcom_MallocFree((void**)&tmpDelayMsg);

                if( !strncmp(strPrevMail, pAlarmInfo[i].mn_email, strlen(pAlarmInfo[i].mn_email)) ) // 같으면
                {
                    mergeTotMsg = (char *)realloc(mergeTotMsg, sizeof(char)*(strlen(mergeTotMsg)+strlen(mergeMsg)+1));
                    sprintf(mergeTotMsg+strlen(mergeTotMsg), "%s", mergeMsg);
                    if( strlen(mergeTotMsg) > 0 )
                    {
                        WRITE_INFO(CATEGORY_INFO,"Check msg, mergeTotMsg(%s) ",
                                   mergeTotMsg);
                    }

                }
                fcom_MallocFree((void**)&mergeMsg);
            }
            else
            {
                memset	(strLevel, 	0x00, sizeof(strLevel));
                rxt = fcom_GetTagValue(delayMsg, "(", ")", strLevel, sizeof(strLevel) );
//                rxt = fcom_GetTagValue(delayMsg, "(", ")", strLevel );
                if( rxt < 0 )
                {

                    WRITE_INFO(CATEGORY_INFO,"Invalid str level(%s), forced set level(INFO) ",
                            strLevel );

                    strcpy(strLevel, "INFO");
                }
                nLevel = fcom_GetNumberLevel(strLevel);
                if( strstr(delayMsg, strType) 	!= NULL &&
                    (pAlarmInfo[i].al_detect_level-'0')%48 <= nLevel )
                {
                    if( !strncmp(strPrevMail, pAlarmInfo[i].mn_email, strlen(strPrevMail)) ) // 같으면
                    {
                        mergeTotMsg = (char *)realloc(mergeTotMsg, sizeof(char)*(strlen(mergeTotMsg)+strlen(delayMsg)+1));
                        sprintf(mergeTotMsg+strlen(mergeTotMsg), "%s", delayMsg);
                        if( strlen(mergeTotMsg) > 0 )
                        {

                            WRITE_INFO(CATEGORY_INFO,"Check msg, mergeTotMsg(%s) ",
                                    mergeTotMsg);
                        }

                    }
                }
            }

            if( i == (g_stProcAlarmInfo.nAlarm - 1) ) // 마지막이면
            {
                if( strlen(mergeTotMsg) > 0 )
                {
                    tokenCnt = fcom_TokenCnt(strPrevMail, ",");
                    if( tokenCnt > 0 )
                    {
                        memset(tmpPrevMail, 0x00, sizeof(tmpPrevMail));
                        strcpy(tmpPrevMail, strPrevMail);
                        tokenMail = strtok(tmpPrevMail, ",");
                        while( tokenMail != NULL )
                        {
                            if( strstr(tokenMail, "@") != NULL && strlen(tokenMail) > 5 )
                            {
                                fstSendMail(tokenMail, mergeTotMsg);
                            }
                            else
                            {
                                WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s) ",tokenMail );
                            }
                            tokenMail = strtok(NULL, ",");
                        }
                    }
                    else
                    {
                        if( strstr(strPrevMail, "@") != NULL && strlen(strPrevMail) > 5 )
                        {

                            fstSendMail(strPrevMail, mergeTotMsg);
                        }
                        else
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Invalid mail format(%s) ",strPrevMail );
                        }
                    }
                }
            }

        } // if
    } // for

    fcom_MallocFree((void**)&mergeTotMsg);

    return 0;
}
