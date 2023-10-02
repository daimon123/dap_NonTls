/*
 * DAP Master
 */

/* ------------------------------------------------------------------- */
/* System Header                                                       */
/* ------------------------------------------------------------------- */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>


/* ------------------------------------------------------------------- */
/* User Header                                                         */
/* ------------------------------------------------------------------- */
#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "linuxke/dap_linux.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "sock/dap_sock.h"
#include "dap_version.h"
#include "master.h"

/* ------------------------------------------------------------------- */
/* Static Function                                                     */
/* ------------------------------------------------------------------- */
static int fstMainTask(void);
static int fstMasterInit(void);
static void* fst_ThreadLoop(void* arg);
static void fst_InitProcessInvoke(void);
char RestartFlag;

int main(int argc, char **argv)
{
    int nMaxLimit = 0;
    _DAP_COMN_INFO* pstComnInfo = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    memset(&g_stServerInfo, 0x00, sizeof(g_stServerInfo));

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    /* Argument Parsing */
    fcom_ArgParse(argv);

    /* DAP Process Init */
    fstMasterInit();

    /* DAP Process Log Init */
    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    /* Get Max Open Files Limit */
    nMaxLimit = fmaster_GetPrintRlim();

    fcom_SetDeamon(nMaxLimit, pstComnInfo->nCfgMaxOpenFileLimit);

    if(nMaxLimit < pstComnInfo->nCfgMaxOpenFileLimit)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program because, currOpenFileLimit(%d) < maxOpenFileLimit(%d)",
                nMaxLimit,
                pstComnInfo->nCfgMaxOpenFileLimit);

        fipc_FQClose(DBLOG_QUEUE);
        fcom_MallocFree((void**)&g_pstNotiMaster);

        exit(0);
    }

    /* Daemonize */
    /* Exception Signal */
    fmaster_GuardSignal();

    fstMainTask();

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */

    return RET_SUCC;
}


static int fstMainTask(void)
{
    int nRetval = 0;
    char szDelFileDay[10]           = {0x00,};
    char szcurrDay[10]              = {0x00,};
    char local_szPidPath[127 +1]    = {0x00,};

    time_t	currTime, delFileTime;
    time_t  nCfgLastModify;

    char* local_ptrProcessName = NULL;

    g_ptrProcessInfo = (_PROCESS_INFO*)g_stProcessInfo;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_INFO(CATEGORY_DEBUG, "Function Start " );

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_MASTER);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    if(machine_init(&g_stServerInfo.stStatics) == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in init machine " );
        nRetval = 1;
        goto __goto__end__;
    }

    if(fmaster_GetOneprocessInfo("dap_master", "") > 1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in restart " );
        nRetval = 0;
        goto __goto__end__;
    }

    /* Maria DB Connect */
    if(fdb_ConnectDB() != RET_SUCC)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in db connection " );
        fdb_RetryConnectDb();
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Success Db Connection ");
    }

    if (fdb_CheckTbTables() < 0)
    {
        fcom_ExitServer(0);
    }

    /** FRD Dump 처리 **/
    fmaster_FrdDumpExec();

    /** Master 프로세스 초기 기동시 Process 기동시켜준다. **/
    fst_InitProcessInvoke();

    WRITE_INFO(CATEGORY_DEBUG,"Process Start " );
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Process Start ");

    //check 후 linker를 통한 db접속이 제대로 안될때가 있어 sleep 줌.
    sleep(1);

    delFileTime = time((time_t) 0)- g_stServerInfo.stDapLogInfo.nCfgJobTime;
    fcom_time2str(delFileTime, szDelFileDay,"YYYYMMDD\0");

    // Resart Thread Create
    pthread_t   pthreadloop;
    const int ThreadStackSize = 4*1024*1024;
    if(fcom_ThreadCreate((void *)&pthreadloop, fst_ThreadLoop, (void *)NULL, ThreadStackSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fpcif_ThreadLoop Thread Create Fail " );
        return (-1);
    }


    /** Master Process는 여기부터 dblog 기능 사용가능. Master process가 dblog process를 띄우기전이라 이전에는 불가. **/
    while(1)
    {
        struct tm stTm;
        currTime = time((time_t) 0);
        delFileTime = fcom_CalcTime(PERIOD_STAT);

        memset(&stTm, 0x00, sizeof(struct tm));
        localtime_r(&currTime,&stTm);

        if(fmaster_ReloadConfig() < 0)
        {
            fmaster_DbReconnect();
        }

        if ( RestartFlag != 0x01 ) {
            if(fmaster_SaveProcessInfo() < 0)
            {
                fmaster_DbReconnect();
            }
        }

        fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);

        if(fmaster_CheckNotiFile() < 0)
        {
            fmaster_DbReconnect();
        }

        if ( RestartFlag != 0x01 )
        {
            fmaster_CheckProcAlarm(g_ptrProcessInfo, gProcess);
            fmaster_KillProcess(g_ptrProcessInfo);
            fmaster_InvokeProcess(g_ptrProcessInfo);
        }

        fcom_time2str(currTime, szcurrDay, "YYYYMMDD\0");

        if(strcmp(szcurrDay,szDelFileDay) > 0)
        {
            delFileTime    =   time((time_t) 0);
            fcom_time2str(delFileTime, szDelFileDay, "YYYYMMDD\0");
            fmaster_DeleteLogProcess();
            fmaster_DeleteDbProcess();
            fdb_DeleteSyncTableByDay();
        }


        /* 로그파일 Rename 기능 */
        /*
        if(cur_min != old_min )
        {
            fstRenameLogProcess();
            old_min = cur_min;
        }
        */

        if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile,&nCfgLastModify) == 1)
        {
            fmaster_ReloadCfgFile();
        }

        // 1분 주기여서 fmaster_RestartProcess() 안탈수도있어서 주석처리
//        fcom_Msleep(g_stServerInfo.stDapComnInfo.nArgSecInterval*1000);
        sleep(10);
    }

__goto__end__:
    fcom_ExitServer(nRetval);
    return RET_SUCC;
}

static int fstMasterInit(void)
{
    int  nRetVal   = 0;
    int  i         = 0;
    char szConfFile[128 +1]         =   {0x00,};
    char local_szCfgTmp[255 +1]           =   {0x00,};
    char local_szDefaultIp[15 +1]   = {0x00,};
    _DAP_COMN_INFO*  pstComnInfo    = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    i = nRetVal = RET_SUCC;
    memset(szConfFile, 0x00, sizeof(szConfFile));

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome, sizeof(pstComnInfo->szDapHome  ),
                                getenv("DAP_HOME"));

    snprintf(pstComnInfo->szComConfigFile                     ,
                        sizeof(pstComnInfo->szComConfigFile)  ,
                         "%s%s"                        ,
                         pstComnInfo->szDapHome               ,
                         "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        nRetVal = (-1);
        goto __goto_end__;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    pstComnInfo->nCfgMgwId            = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    fsock_GetNic(local_szCfgTmp);
    fsock_GetIpAddress(local_szCfgTmp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );
    pstComnInfo->nCfgMaxOpenFileLimit = fcom_GetProfileInt("COMMON","MAX_OPEN_FILE_LIMIT", 65536);
    pstComnInfo->nArgSecInterval      = fcom_GetProfileInt("COMMON","PROCESS_CHECK_INTERVAL",15);


    /* Master Noti File Init */
    /* --------------------------------------------------------------------- */
    /* 0 : PROCESS_CHANGE */
    if(fcom_malloc((void**)&g_pstNotiMaster, sizeof(_CONFIG_NOTI) * MAX_MASTER_NOTI) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }


    snprintf(g_pstNotiMaster[PROCESS_CHANGE].szNotiFileName,
            sizeof(g_pstNotiMaster[PROCESS_CHANGE].szNotiFileName),
            "%s/config/proc_change",
            g_stServerInfo.stDapComnInfo.szDapHome);

    for(i = 0; i < MAX_MASTER_NOTI; i++)
    {
        g_pstNotiMaster[i].lastModify = 0;
        g_pstNotiMaster[i].reload     = TRUE;
    }

    snprintf(pstQueueInfo->szDAPQueueHome,
            sizeof(pstQueueInfo->szDAPQueueHome),
            "%s/.DAPQ",
            pstComnInfo->szDapHome);

    if(access(pstQueueInfo->szDAPQueueHome, R_OK) != 0 )
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            WRITE_CRITICAL(CATEGORY_INFO,"Fail in make queue directory(%s) ",
                    pstQueueInfo->szDAPQueueHome);

            exit(0);
        }
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


    /* Process Info Initialize */
    memset(g_stProcessInfo, 0x00, sizeof(g_stProcessInfo));

    printf("Success Init | %s\n", __func__ );

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */
__goto_end__:
    printf("%s RetValue = %d \n", __FUNCTION__, nRetVal);
    return nRetVal;

}

static void fst_InitProcessInvoke(void)
{
    fmaster_ReloadConfig();
    fmaster_SaveProcessInfo();
    fmaster_InvokeProcess(g_ptrProcessInfo);

    return;
}

void* fst_ThreadLoop(void* arg)
{
    struct tm stTm;
    time_t currTime = time(NULL);

    while(1){
        currTime = time((time_t) 0);

        memset(&stTm, 0x00, sizeof(struct tm));
        localtime_r(&currTime,&stTm);

        if ( stTm.tm_hour == 04 && stTm.tm_min == 00) {
            fmaster_RestartProcess();
        }
        sleep(10);
    }

}
