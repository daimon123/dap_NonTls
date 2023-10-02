//
// Created by KimByoungGook on 2020-06-25.
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
#include <time.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "ipc/dap_Queue.h"
#include "secure/dap_secure.h"
#include "sock/dap_sock.h"

#include "schd.h"
#include "dap_version.h"

static int fstSchdInit();
static void fstHandleSignal(int sid);
static int fstKeepSession();
static int fstGetFileSync(char *fName);
static int fstMainTask();
static int fstReloadSync(void);
static int fstCheckSyncFile(void);
static void fstReloadCfgFile( );
static void fstSigHandler();

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fstSchdInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fdb_ConnectDB();

    fdb_LsystemInit();

    fstSigHandler();

    fstMainTask();

    return RET_SUCC;
}

static int fstMainTask()
{
    time_t  currTime = 0;
    time_t	sessionTime = 0;
    time_t	prmonTime = 0;
    time_t	makeTime = 0;
    time_t	moveTime = 0;
    time_t	statsTime = 0;
    int		cfgDbPort = 0;
    int		decrypto_size = 0;
    int     CurrMin = 0;
    int     nRet = 0, nRet2 = 0;

    char    currDay[14 +1] = {0x00,};
    char    currHourMin[6 +1] = {0x00,};
    char    sessionDay[14 +1] = {0x00,};
    char    prmonDay[14 +1] = {0x00,};
    char    statsDay[14 +1] = {0x00,};
    char    makeDay[14 +1] = {0x00,};
    char    moveDay[14 +1] = {0x00,};
    char	cmd[256 +1] = {0x00,};
    char	cfgDbIp[15 +1] = {0x00,};
    char	cfgTmpDbId[64 +1] = {0x00,};
    char	cfgDbId[30 +1] = {0x00,};
    char	cfgTmpDbPwd[63 +1] = {0x00,};
    char	cfgDbPwd[30 +1] = {0x00,};
    char    local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_SCHD);

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

    sessionTime = time((time_t) 0) - g_stProcSchdInfo.nCfgKeepSession;
    fcom_time2str(sessionTime, sessionDay, "YYYYMMDDhhmmss\0");

    WRITE_INFO(CATEGORY_INFO, "cfgKeepSession(%d) sessionDay(%s) ",
            g_stProcSchdInfo.nCfgKeepSession,
            sessionDay);

    prmonTime = time((time_t) 0) - g_stProcSchdInfo.nCfgPrmonInterval;

    fcom_time2str(prmonTime, prmonDay, "YYYYMMDDhhmmss\0");

    WRITE_INFO(CATEGORY_INFO, "cfgPrmonInterval(%d) prmonDay(%s) ",
            g_stProcSchdInfo.nCfgPrmonInterval,
            prmonDay);

    statsTime = time((time_t) 0) - g_stProcSchdInfo.nCfgSchdMakeStatsInterval;
    fcom_time2str(statsTime, statsDay, "YYYYMMDDhhmmss\0");

    WRITE_INFO(CATEGORY_INFO, "cfgSchdMakeStatsInterval(%d)sec, statsDay(%s) ",
            g_stProcSchdInfo.nCfgSchdMakeStatsInterval,
            statsDay);

    makeTime = time((time_t) 0) - g_stProcSchdInfo.nCfgSchdMakeHistoryInterval;
    fcom_time2str(makeTime, makeDay, "YYYYMMDDhhmmss\0");
    WRITE_INFO(CATEGORY_INFO, "cfgSchdMakeHistoryInterval(%d)sec, makeDay(%s) ",
            g_stProcSchdInfo.nCfgSchdMakeHistoryInterval,
            makeDay);

    moveTime = time((time_t) 0) - g_stProcSchdInfo.nCfgSchdMoveHistoryInterval;
    fcom_time2str(moveTime, moveDay, "YYYYMMDDhhmmss\0");

    WRITE_INFO(CATEGORY_INFO, "cfgSchdMoveHistoryInterval(%d)sec, moveDay(%s) ",
            g_stProcSchdInfo.nCfgSchdMoveHistoryInterval,
            moveDay);

    while(1)
    {
        currTime = time((time_t) 0);
        fcom_time2str(currTime, currDay, "YYYYMMDDhhmmss\0");
        fcom_time2str(currTime, currHourMin, "hhmmss\0");

        CurrMin = fcom_GetSysMinute();

        if((CurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcSchdInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }
        if (strcmp(currDay, sessionDay) > 0)
        {
            sessionTime = time((time_t) 0) + g_stProcSchdInfo.nCfgKeepSession;
            fcom_time2str(sessionTime, sessionDay, "YYYYMMDDhhmmss\0");
            fstKeepSession();
        }

        if (strcmp(currDay, prmonDay) > 0)
        {
            prmonTime = time((time_t) 0) + g_stProcSchdInfo.nCfgPrmonInterval;
            fcom_time2str(prmonTime, prmonDay, "YYYYMMDDhhmmss\0");
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcSchdInfo.nLastSendTime);
        }

        if (strcmp(currDay, statsDay) > 0)
        {
            WRITE_INFO(CATEGORY_INFO, "Run make stats " );
            statsTime = time((time_t) 0) + g_stProcSchdInfo.nCfgSchdMakeStatsInterval;
            fcom_time2str(statsTime, statsDay, "YYYYMMDDhhmmss\0");
            nRet = fschd_MakeStatsGwTb();
            if (nRet > 0)
            {
                memset(cmd, 0x00, sizeof(cmd));
                sprintf(cmd, "touch %s/config/gw_change", getenv("DAP_HOME"));
                system(cmd);
            }
            WRITE_INFO(CATEGORY_INFO, "Run make stats " );
        }

        if (strcmp(currDay, makeDay) > 0)
        {
            WRITE_INFO(CATEGORY_INFO, "Run create script " );

            makeTime = time((time_t) 0) + g_stProcSchdInfo.nCfgSchdMakeHistoryInterval;
            fcom_time2str(makeTime, makeDay, "YYYYMMDDhhmmss\0");
            fschd_CreateHistoryDatabase("");
            fschd_CreateHistoryTables();

            WRITE_INFO(CATEGORY_DB, "Run create script " );
        }

        if (strcmp(currDay, moveDay) > 0)
        {
            WRITE_INFO(CATEGORY_DB, "Run move history " );
            moveTime = time((time_t) 0) + g_stProcSchdInfo.nCfgSchdMoveHistoryInterval;
            fcom_time2str(moveTime, moveDay, "YYYYMMDDhhmmss\0");

            //break�ɸ�Ʒ������ȵ��ư�
            nRet = fschd_MoveHistoryBaseTb();
            nRet = fschd_MoveHistoryConfigTb();
            nRet = fschd_MoveHistoryRuleTb();
            nRet = fschd_MoveHistoryRuleExceptTb();
            nRet = fschd_MoveHistoryRuleDetectTb();
            nRet = fschd_MoveHistoryRuleDetectLinkTb();
            nRet = fschd_MoveHistoryRuleScheduleTb();
            nRet = fschd_MoveHistoryUpgradeAgentTb();
            nRet = fschd_MoveHistoryUserGroupTb();
            nRet = fschd_MoveHistoryUserGroupLinkTb();
            nRet = fschd_MoveHistoryUserTb();
            nRet = fschd_MoveHistoryUserLinkTb();
            nRet = fschd_MoveHistoryManagerTb();
            nRet = fschd_MoveHistoryDiskRegLinkTb();
            nRet = fschd_MoveHistoryDetectGroupLinkTb();
            nRet = fschd_MoveHistoryDetectGroupTb();
            nRet = fschd_MoveHistorydetectLinkTb();
            nRet = fschd_MoveHistoryDetectProcessTb();
            nRet = fschd_MoveHistoryWatchServerTb();
            nRet = fschd_MoveHistoryDetectPrinterPortTb();
            nRet = fschd_MoveHistoryManagerEventTb();
            nRet = fschd_MoveHistoryEventTb();
            nRet = fschd_MoveHistoryAlarmTb();

            WRITE_INFO(CATEGORY_DB, "Run move history " );

        }

        /* Config Sync Activation */
        if (g_stProcSchdInfo.nCfgSyncActive)
        {
            if (strlen(g_stProcSchdInfo.szCfgSyncRunTime) <= 2)
            {
                g_stProcSchdInfo.nIngJobTime = time(NULL);
                if ((g_stProcSchdInfo.nIngJobTime % atoi(g_stProcSchdInfo.szCfgSyncRunTime)) == 0)
                {
                    fstReloadSync();
                    fstCheckSyncFile();
                }
            }
            else if (strlen(g_stProcSchdInfo.szCfgSyncRunTime) == 6)
            {
                if (!strncmp(currHourMin, g_stProcSchdInfo.szCfgSyncRunTime, 6))
                {
                    nRet = fstGetFileSync(g_stProcSchdInfo.szCfgSyncGroupFileName);
                    nRet2 = fstGetFileSync(g_stProcSchdInfo.szCfgSyncUserFileName);
                    if (nRet > 0 || nRet2 > 0)
                    {
                        memset(cmd, 0x00, sizeof(cmd));
                        sprintf(cmd, "touch %s/config/cp_change", getenv("DAP_HOME"));
                        system(cmd);
                    }
                }
            }
        }

        if ((g_stProcSchdInfo.nCfgBackupActive == 1) &&
        (!strncmp(currHourMin, g_stProcSchdInfo.szCfgBackupTime, 6)))
        {
            WRITE_INFO(CATEGORY_DB, "Run database backup ");

            memset(cfgDbIp, 0x00, sizeof(cfgDbIp));
            memset(cfgTmpDbId, 0x00, sizeof(cfgTmpDbId));
            memset(cfgTmpDbPwd, 0x00, sizeof(cfgTmpDbPwd));


            fcom_GetProfile("MYSQL","DB_IP",cfgDbIp,"127.0.0.1");
            fcom_GetProfile("MYSQL","DB_ID",cfgTmpDbId,"master");
            fcom_GetProfile("MYSQL","DB_PASSWORD",cfgTmpDbPwd,"master");
            cfgDbPort = fcom_GetProfileInt("MYSQL","DB_PORT",3306);

            memset(cfgDbId, 0x00, sizeof(cfgDbId));
            fsec_DecryptStr((char *) &cfgTmpDbId, (char *) &cfgDbId, &decrypto_size);
            memset(cfgDbPwd, 0x00, sizeof(cfgDbPwd));
            fsec_DecryptStr((char *) &cfgTmpDbPwd, (char *) &cfgDbPwd, &decrypto_size);
            memset(cmd, 0x00, sizeof(cmd));
            sprintf(cmd, "%s/bin/sql_backup.sh %s %s %s %d &", getenv("DAP_HOME"),
                    cfgDbId, cfgDbPwd, cfgDbIp, cfgDbPort);
            system(cmd);

            WRITE_INFO(CATEGORY_INFO, "Run database backup ");
        }

        fcom_Msleep(g_stProcSchdInfo.nCfgSchdInterval * 1000);
    }


    return 0;
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
    signal( SIGPIPE, SIG_IGN );              /* 13: write on a pipe with no one to read it */

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

static int fstSchdInit()
{
    int nRet = 0, loop = 0;

    char szCfgTmp[255 +1]           = {0x00,};
    char szFqName[ 10 +1]           = {0x00,};
    char local_szDefaultIp[15 +1]   = {0x00,};
    _DAP_COMN_INFO*     pstComnInfo     = NULL;
    _DAP_QUEUE_INFO*    pstQueueInfo    = NULL;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                     );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    fsock_GetNic(szCfgTmp);
    fsock_GetIpAddress(szCfgTmp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );

    g_stProcSchdInfo.nCfgKeepSession                = fcom_GetProfileInt("MYSQL","KEEP_SESSION",5);
    g_stProcSchdInfo.nCfgPrmonInterval              = fcom_GetProfileInt("PRMON","INTERVAL",600);
    g_stProcSchdInfo.nCfgSchdInterval               = fcom_GetProfileInt("SCHD","CYCLE", 10);
    g_stProcSchdInfo.nCfgSchdMakeStatsInterval      = fcom_GetProfileInt("SCHD", "MAKE_STATS_INTERVAL",3600); //sec
    g_stProcSchdInfo.nCfgSchdMakeHistoryInterval    = fcom_GetProfileInt("SCHD", "MAKE_HISTORY_INTERVAL",86400); //sec
    g_stProcSchdInfo.nCfgSchdMoveHistoryInterval    = fcom_GetProfileInt("SCHD", "MOVE_HISTORY_INTERVAL",600);
    g_stProcSchdInfo.nCfgSyncActive                 = fcom_GetProfileInt("SYNC","ACTIVATION",0);
    g_stProcSchdInfo.nCfgSyncInsBatch               = fcom_GetProfileInt("SYNC","INSERT_BATCH_ROW",100);

    /* BACKUP -> BACKUP_ACTIVATION 변경.*/
    g_stProcSchdInfo.nCfgBackupActive = fcom_GetProfileInt("BACKUP","BACKUP_ACTIVATION", 0);

    fschd_SetSafeFetchRow();

    fcom_GetProfile("BACKUP","RUN_TIME",g_stProcSchdInfo.szCfgBackupTime,"0100");
    fcom_GetProfile("SYNC","RUN_TIME",g_stProcSchdInfo.szCfgSyncRunTime,"0500");

    if(strlen(g_stProcSchdInfo.szCfgBackupTime) == 4)
        strcat(g_stProcSchdInfo.szCfgBackupTime,"00");

    if(strlen(g_stProcSchdInfo.szCfgSyncRunTime) == 4)
        strcat(g_stProcSchdInfo.szCfgSyncRunTime,"00");

    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome),
            "%s/.DAPQ", pstComnInfo->szDapHome);

    if (access(pstQueueInfo->szDAPQueueHome, R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",
                    pstQueueInfo->szDAPQueueHome,
                    __func__);
            exit(0);
        }
    }

    fcom_GetProfile("PRMON","FQ_LINK_NAME",szFqName,"PRMONQ");
    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
            "%s/%s",
            pstQueueInfo->szDAPQueueHome,
            szFqName);
    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n", pstQueueInfo->szPrmonQueueHome,__func__);
        exit(0);
    }

    snprintf(pstQueueInfo->szDblogQueueHome, sizeof(pstQueueInfo->szDblogQueueHome),
            "%s/DBLOGQ", pstQueueInfo->szDAPQueueHome);


    fcom_GetProfile("SYNC","DOWN_FILE_PATH",g_stProcSchdInfo.szCfgSyncDownFilePath,"/tmp/dap");
    snprintf(szCfgTmp, sizeof(szCfgTmp), "%s/",g_stProcSchdInfo.szCfgSyncDownFilePath);

    if (access(szCfgTmp, W_OK) != 0)
    {
        nRet = fcom_MkPath(szCfgTmp, 0755);
        if (nRet < 0)
        {
            printf("Fail in make path(%s)|%s\n", szCfgTmp, __func__);
        }
        else
        {
            chmod(szCfgTmp, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
            printf("Succeed in make path(%s)|%s\n", szCfgTmp, __func__);
        }
    }

    if(fcom_malloc((void**)&g_pstNotiMaster, MAX_SYNC_NOTI * sizeof(_CONFIG_NOTI)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }


    fcom_GetProfile("SYNC","GROUP_FILE_NAME",g_stProcSchdInfo.szCfgSyncGroupFileName,"");

    memset(szCfgTmp, 0x00, sizeof(szCfgTmp));
    snprintf(szCfgTmp, sizeof(szCfgTmp),
            "%s/%s",
            g_stProcSchdInfo.szCfgSyncDownFilePath,
            g_stProcSchdInfo.szCfgSyncGroupFileName);
    fcom_GetProfile("COMMON","SYNC_GROUP_CHANGE",g_pstNotiMaster[SYNC_GROUP_CHANGE].szNotiFileName, szCfgTmp);
    fcom_GetProfile("SYNC","USER_FILE_NAME",g_stProcSchdInfo.szCfgSyncUserFileName,"");

    memset(szCfgTmp, 0x00, sizeof(szCfgTmp));
    snprintf(szCfgTmp, sizeof(szCfgTmp),
             "%s/%s",
             g_stProcSchdInfo.szCfgSyncDownFilePath,
             g_stProcSchdInfo.szCfgSyncUserFileName);
    fcom_GetProfile("COMMON","SYNC_USER_CHANGE",g_pstNotiMaster[SYNC_USER_CHANGE].szNotiFileName, szCfgTmp);

    for(loop = 0; loop < MAX_SYNC_NOTI; loop++)
    {
        g_pstNotiMaster[loop].lastModify = 0;
        g_pstNotiMaster[loop].reload = TRUE;
    }
    fcom_GetProfile("SYNC","DOWN_FILE_CHARSET",g_stProcSchdInfo.szCfgSyncDownFileCharset,"");

    g_stProcSchdInfo.nCurTime = g_stProcSchdInfo.nLastJobTime =  time(NULL);

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
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_schd");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_schd");

    if(fipc_FQPutInit(DBLOG_QUEUE,pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)", pstQueueInfo->szDblogQueueHome);
        exit(0);
    }

    printf("Success Init | %s\n",__func__ );

    return TRUE;

}


static void fstHandleSignal(int sid)
{
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_SCHD);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_CRITICAL(CATEGORY_DEBUG, "Got signal(%d)", sid);
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);

    fdb_SqlClose(g_stMyCon);


    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
    }

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(sid);
}



static int fstKeepSession()
{
    int rxt;

    g_stProcSchdInfo.nCurTime = time(NULL);

    if ((g_stProcSchdInfo.nCurTime - g_stProcSchdInfo.nLastJobTime) > g_stProcSchdInfo.nCfgKeepSession)
    {
        g_stProcSchdInfo.nLastJobTime = time(NULL);
        rxt = fdb_PutSessionQuery();
        if (rxt !=  0)
        {
            WRITE_CRITICAL(CATEGORY_DB, "Fail in session query ");
        }
    }

    return RET_SUCC;
}

static int fstGetFileSync(char *fName)
{
    int				rxt;
    int				fileSize = 0;
    char			fileFullPath[256];
    char			cmd[256];
    struct stat		buf;

    memset	(fileFullPath, 0x00, sizeof(fileFullPath));
    sprintf	(fileFullPath, "%s/%s", g_stProcSchdInfo.szCfgSyncDownFilePath,fName);

    if (stat(fileFullPath, &buf) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Stat error, file(%s) ", fileFullPath);
        return -1;
    }

    fileSize = buf.st_size;
    if (fileSize == 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Size error, file(%s)size(%d) ", fileFullPath,fileSize);
        return -1;

    }

    if (!strcmp(fName, g_stProcSchdInfo.szCfgSyncGroupFileName))
    {
        /* dap_dir 결과 실패시 0 리턴 */
        rxt = fschd_LoadFileSyncTb(1, fileFullPath); // group
        if (rxt != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Sync Load Group Fail " );
        }
        else
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Sync Load Group Success " );
        }
    }
    else if (!strcmp(fName, g_stProcSchdInfo.szCfgSyncUserFileName))
    {
        /* dap_dir 결과 실패시 0 리턴 */
        rxt = fschd_LoadFileSyncTb(2,  fileFullPath); // user
        if (rxt == 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Sync Load User Success " );
            rxt = fschd_UgdGroupByRule(); // 그룹이삭제되었을경우RULE비활성화
            if (rxt > 0)
            {
                memset	(cmd, 0x00, sizeof(cmd));
                sprintf	(cmd, "touch %s/config/rule_change", getenv("DAP_HOME"));
                system	(cmd);
            }
        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DB,"Sync Load User Fail " );
        }

    }

    return 1;
}

static int fstReloadSync(void)
{
    int     rxt = 0;
    char	cmd[256 +1] = {0x00,};
    struct stat     statBuf;

    memset(&statBuf, 0x00, sizeof(struct stat));

    WRITE_INFO(CATEGORY_INFO, "start ");
    if(g_pstNotiMaster[SYNC_GROUP_CHANGE].reload)
    {
        // set sync group
        rxt = fstGetFileSync(g_stProcSchdInfo.szCfgSyncGroupFileName);

        stat(g_pstNotiMaster[SYNC_GROUP_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[SYNC_GROUP_CHANGE].lastModify	= statBuf.st_mtime;
        g_pstNotiMaster[SYNC_GROUP_CHANGE].reload		= FALSE;

        if (rxt > 0)
        {
            memset	(cmd, 0x00, sizeof(cmd));
            sprintf	(cmd, "touch %s/config/cp_change", getenv("DAP_HOME"));
            system	(cmd);
        }
    }

    if(g_pstNotiMaster[SYNC_USER_CHANGE].reload)
    {
        // set sync user
        rxt = fstGetFileSync(g_stProcSchdInfo.szCfgSyncUserFileName);

        stat(g_pstNotiMaster[SYNC_USER_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[SYNC_USER_CHANGE].lastModify	= statBuf.st_mtime;
        g_pstNotiMaster[SYNC_USER_CHANGE].reload       = FALSE;

        if (rxt > 0)
        {
            memset	(cmd, 0x00, sizeof(cmd));
            sprintf	(cmd, "touch %s/config/cp_change", getenv("DAP_HOME"));
            system	(cmd);
        }
    }
    WRITE_INFO(CATEGORY_INFO, "End ");


    return TRUE;
}

static int fstCheckSyncFile(void)
{
    register int i = 0;
    struct stat statBuf;

    memset(&statBuf, 0x00, sizeof(struct stat));

    WRITE_INFO(CATEGORY_INFO, "start");
    for(i = 0; i < MAX_SYNC_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC, "Fail in stat noti files(%s)(%d)",
                    g_pstNotiMaster[i].szNotiFileName,
                    i);

        }
        else
        {
            if(statBuf.st_mtime > g_pstNotiMaster[i].lastModify)
            {
                g_pstNotiMaster[i].reload = TRUE;
            }
            else
            {
                g_pstNotiMaster[i].reload = FALSE;
            }
        }
    }
    WRITE_INFO(CATEGORY_INFO, "End ");

    return TRUE;
}


static void fstReloadCfgFile( )
{
    g_stProcSchdInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL",600);
    g_stProcSchdInfo.nCfgSchdInterval = fcom_GetProfileInt("SCHD","CYCLE", 10);
    g_stProcSchdInfo.nCfgSchdMakeStatsInterval = fcom_GetProfileInt("SCHD", "MAKE_STATS_INTERVAL",3600); //sec
    g_stProcSchdInfo.nCfgSchdMakeHistoryInterval = fcom_GetProfileInt("SCHD", "MAKE_HISTORY_INTERVAL",86400); //sec
    g_stProcSchdInfo.nCfgSchdMoveHistoryInterval = fcom_GetProfileInt("SCHD", "MOVE_HISTORY_INTERVAL",600);
    g_stProcSchdInfo.nCfgSyncInsBatch = fcom_GetProfileInt("SYNC","INSERT_BATCH_ROW",100);
    fcom_GetProfile("SYNC","RUN_TIME",g_stProcSchdInfo.szCfgSyncRunTime,"0500");
    if(strlen(g_stProcSchdInfo.szCfgSyncRunTime) == 4)
        strcat(g_stProcSchdInfo.szCfgSyncRunTime,"00");

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n SCHD_CYCLE : [%d] \n SCHD_MAKE_STATS_INTERVAL :[%d] \n"
                            "SCHD_MAKE_HISTORY_INTERVAL : [%d] \n SCHD_MOVE_HISTORY_INTERVAL : [%d] \n SYNC_INSERT_BATCH_ROW : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d]",
              g_stProcSchdInfo.nCfgPrmonInterval,
              g_stProcSchdInfo.nCfgSchdInterval,
              g_stProcSchdInfo.nCfgSchdMakeStatsInterval,
              g_stProcSchdInfo.nCfgSchdMakeHistoryInterval,
              g_stProcSchdInfo.nCfgSchdMoveHistoryInterval,
              g_stProcSchdInfo.nCfgSyncInsBatch,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}