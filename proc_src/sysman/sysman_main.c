//
// Created by KimByoungGook on 2020-06-19.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>


#include "com/dap_def.h"
#include "com/dap_def.h"
#include "com/dap_com.h"

#include "ipc/dap_Queue.h"
#include "linuxke/dap_linux.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "db/dap_defield.h"

#include "sock/dap_sock.h"

#include "sysman.h"
#include "dap_version.h"

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fstSysmanInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fstGuardSignal();

    fstMainTask();

    return RET_SUCC;
}

int fstMainTask(void)
{
    int nRet;
    int nLoopCnt;
    int nCurrMin = 0;
    struct	statics statics;
    char*   local_ptrProcessName = NULL;
    pFs         = fs;
    pQs			= qs;

    char local_szPidPath[127 +1]    = {0x00,};

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_SYSMAN);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    g_pstThreshold = &(g_stThreshold[0]);
    memset(g_pstThreshold, 0x00, sizeof(_DAP_DB_THRESHOLD_INFO) * MAX_THRESHOLD_CATEGORY);


    if(machine_init(&statics) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Machine Init Fail" );
        exit(1);
    }

    nRet = fdb_ConnectDB();
    if(nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in db connection");
        exit(0);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB,"Succeed in db connection" );
    }

    nLoopCnt = 0;
    memset(&g_stSystemLoadInfo, 0x00, sizeof(g_stSystemLoadInfo));

    while(1)
    {
        nCurrMin = fcom_GetSysMinute();

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcSysmanInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }

        nLoopCnt++;
        if((nLoopCnt * g_stProcSysmanInfo.cfgSysmanInterval) == g_stProcSysmanInfo.cfgPrmonInterval )
        {
            nLoopCnt = 0;
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcSysmanInfo.last_send_time);
        }

        fipc_KeepSession(g_stServerInfo.stDapComnInfo.nCfgKeepSession);

        if(fstReloadConfig() < 0)
        {
            fdb_SqlClose(g_stMyCon);
            return (-1);
        }

        if(fstSaveDiskInfo() < 0)
        {
            fdb_SqlClose(g_stMyCon);
            return (-2);
        }

        if(fstSaveSystemInfo() < 0)
        {
            fdb_SqlClose(g_stMyCon);
            return (-2);
        }
        if(fstSaveQueueInfo() < 0)
        {
            fdb_SqlClose(g_stMyCon);
            return (-2);
        }
        fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);
        if(fstCheckNotiFile() < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in noti file" );
            fdb_SqlClose(g_stMyCon);
            return	-3;
        }

        fstCheckAlaram(&g_stSystemLoadInfo,
                pFs,
                pQs,
                g_stProcSysmanInfo.nFileSystem,
                g_stProcSysmanInfo.nQueue,
               g_pstThreshold);

        /* sms_alarm(); */
        fcom_Msleep(g_stProcSysmanInfo.cfgSysmanInterval * 1000);

    }

    fstEndProc();

    return RET_SUCC;

}

static int fstGetPerMemory(int *stats, _DAP_DB_SYSTEM_LOAD_INFO* data)
{
    data->mem_tot  = stats[0];
    data->mem_free = stats[2];
    data->mem_used = data->mem_tot - data->mem_free;
    data->mem_perused = (( (double)data->mem_used) / (double)data->mem_tot ) *  100;

    WRITE_INFO(CATEGORY_INFO,"tot(%d) free(%d) used(%d) perused(%d)",
               data->mem_tot,
               data->mem_free,
               data->mem_used,
               data->mem_perused);

    return RET_SUCC;

}
static int fstGetPerCpuStaus(register int *states, _DAP_DB_SYSTEM_LOAD_INFO *data)
{
    data->cpu_idle = states[3] / 10;
    data->cpu_user = states[0] / 10;
    data->cpu_sys  = states[2] / 10;

    WRITE_INFO(CATEGORY_INFO,"idle(%d) user(%d) sys(%d)",
            data->cpu_idle,
            data->cpu_user,
            data->cpu_sys );

    return RET_SUCC;

}

static int fstSaveSystemInfo()
{
    static int nFirstFlag = 0;

    WRITE_INFO(CATEGORY_INFO,"Start " );

    get_system_info(&g_stSystemInfo);

    fstGetPerMemory(g_stSystemInfo.memory, &g_stSystemLoadInfo);

    if(nFirstFlag > 0)
    {
        fstGetPerCpuStaus(g_stSystemInfo.cpustates, &g_stSystemLoadInfo);
    }
    else
    {
        nFirstFlag = 1;
    }

    g_stSystemLoadInfo.mgwid = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    fsysdb_UpdateSysInfo(&g_stSystemLoadInfo);

    WRITE_INFO(CATEGORY_INFO, "End " );

    return	TRUE;
}

static int fstSaveDiskInfo()
{
    if(fsysdb_GetSysDiskInfo(pFs, g_stProcSysmanInfo.nFileSystem) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in getSysDiskInfo");
        return RET_FAIL;
    }

    if(fsysdb_UpdateFileStat(pFs, g_stProcSysmanInfo.nFileSystem) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in update_fileStat");
        return RET_FAIL;
    }
    return RET_SUCC;

}
static int fstSaveQueueInfo()
{

    if((g_stProcSysmanInfo.nQueue = fsysdb_GetQueueStat(pQs)) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in selectQueueInfo");
        return -1;
    }

    return TRUE;
}
static int fstReloadConfig(void)
{
    int		i;
    struct stat		statBuf;


    if(g_pstNotiMaster[FILESYSTEM_CONFIG_CHANGE].reload)
    {
        if((g_stProcSysmanInfo.nFileSystem = fsysdb_GetFileStat(pFs)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in load filesystem config" );
            return	-4;
        }
        stat(g_pstNotiMaster[FILESYSTEM_CONFIG_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[FILESYSTEM_CONFIG_CHANGE].lastModify	=	statBuf.st_mtime;
        g_pstNotiMaster[FILESYSTEM_CONFIG_CHANGE].reload		=	FALSE;

        WRITE_INFO(CATEGORY_INFO,"[FILESYSTEM INFO]" );
        for(i=0; i < MAX_FILESYSTEM; i++)
        {
            if(strlen(pFs[i].filesystem) == 0)
                break;
        }
    }

    if(g_pstNotiMaster[THRESHOLD_CHANGE].reload)
    {
        if((g_stProcSysmanInfo.nThreshold = fsysdb_LoadThreshold(g_pstThreshold)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in load threshold config");
        }

        stat(g_pstNotiMaster[THRESHOLD_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[THRESHOLD_CHANGE].lastModify	=	statBuf.st_mtime;
        g_pstNotiMaster[THRESHOLD_CHANGE].reload		=	FALSE;

    }

    return		TRUE;
}

static void fstHandleSignal(int sigNo)
{
    WRITE_CRITICAL(CATEGORY_DEBUG,"Got signal(%d)",sigNo );
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG,
                     "Recv Signal (%d) Process Stop Status", sigNo);

    fstEndProc();
}
void fstGuardSignal()
{
    signal(SIGHUP,    fstHandleSignal);   /* 1 : hangup */
    signal(SIGINT,    fstHandleSignal);   /* 2 : interrupt (rubout) */
    signal(SIGQUIT,   fstHandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL,    fstHandleSignal);   /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP,   fstHandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT,    fstHandleSignal);   /* 6 : IOT instruction */
    signal(SIGABRT,   fstHandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGBUS,    fstHandleSignal);   /* 7 : BUS error (4.2 BSD). */
    signal(SIGFPE,    fstHandleSignal);   /* 8 : floating point exception */

    signal(SIGKILL,   fstHandleSignal);   /* 9 : kill (cannot be caught or ignored) */

    signal(SIGUSR1,   fstHandleSignal);   /* 10: User-defined signal 1 */

    signal(SIGSEGV,   fstHandleSignal);   /* 11: segmentation violation */

    signal(SIGUSR2,   fstHandleSignal);   /* 12: User-defined signal 2 (POSIX).*/
    signal( SIGPIPE, SIG_IGN);   /* 13: write on a pipe with no one to read it */
//  signal( SIGPIPE,   fstHandleSignal());   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM,   fstHandleSignal);   /* 14: alarm clock */
    signal(SIGTERM,   fstHandleSignal);   /* 15: software termination signal from kill */
    signal(SIGSTKFLT, fstHandleSignal);   /* 16: Stack fault. */
    signal( SIGCHLD, SIG_IGN);                /* 17: Child status has changed */
    signal( SIGCONT, SIG_IGN);                /* 18: Continue (POSIX). */

    signal(SIGSTOP,   fstHandleSignal);   /* 19: Stop, unblockable */

    signal(SIGTSTP ,  fstHandleSignal);   /* 20: Keyboard stop */
    signal(SIGTTIN ,  fstHandleSignal);   /* 21: background tty read attempted */
    signal(SIGTTOU  , fstHandleSignal);   /* 22: Background write to tty  */
    signal(SIGURG  ,  fstHandleSignal);   /* 23: Urgent condition on socket  */
    signal(SIGXCPU ,  fstHandleSignal);   /* 24: exceeded cpu limit */
    signal(SIGXFSZ ,  fstHandleSignal);   /* 25: exceeded file size limit */
    signal(SIGVTALRM ,fstHandleSignal);   /* 26: Virtual alarm clock */
    signal(SIGPROF ,  fstHandleSignal);   /* 27: profiling timer expired */
    signal(SIGWINCH,  fstHandleSignal);   /* 28: Window size change */
    signal(SIGPOLL ,  fstHandleSignal);   /* 29: I/O now possible  */
    signal(SIGPWR ,   fstHandleSignal);   /* 30: power-fail restart */
    signal(SIGSYS ,   fstHandleSignal);   /* 31: Bad system call */
    //    sigignore( SIGWINCH );            /* 20: window size change */
}

void fstEndProc()
{
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName    = NULL;

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_SYSMAN);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    fdb_SqlClose(g_stMyCon);

    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);

    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
        g_pstNotiMaster = NULL;
    }

//    fFile_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_sysman");
    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    WRITE_CRITICAL(CATEGORY_DEBUG,"Process terminated" );

    exit(0);
}

int fstSysmanInit()
{
    int i = 0;
    char szTemp[255 +1] = {0x00,};
    char local_szDefaultIp[15 +1] = {0x00,};
    _DAP_COMN_INFO*  pstComnInfo    = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    g_stProcSysmanInfo.nThreshold = 0;
    g_stProcSysmanInfo.cur_time =
    g_stProcSysmanInfo.last_job_time =
    time(NULL);

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")             );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                     );


    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        return RET_FAIL;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    /* Sysman Noti File Init */
    if(fcom_malloc((void**)&g_pstNotiMaster, sizeof(_CONFIG_NOTI) * MAX_SYSMAN_NOTI) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }


    /* 0 : FILESYSTEM_CONFIG_CHANGE, 1 : THRESHOLD_CHANGE */
    memset(szTemp, 0x00, sizeof(szTemp));
    sprintf((char*)szTemp, "%s/config/file_change", pstComnInfo->szDapHome);
    fcom_GetProfile("COMMON", "FS_CHANGE", g_pstNotiMaster[FILESYSTEM_CONFIG_CHANGE].szNotiFileName, szTemp);

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp),"%s/config/threshold_change",pstComnInfo->szDapHome );
    fcom_GetProfile("COMMON", "THRESHOLD_CHANGE", g_pstNotiMaster[THRESHOLD_CHANGE].szNotiFileName, szTemp);

    for(i = 0; i < MAX_SYSMAN_NOTI; i++)
    {
        g_pstNotiMaster[i].lastModify = 0;
        g_pstNotiMaster[i].reload     = TRUE;
    }


    pstComnInfo->nCfgMgwId  = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    fsock_GetNic(szTemp);
    fsock_GetIpAddress(szTemp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );

    pstComnInfo->nCfgKeepSession            = fcom_GetProfileInt("MYSQL","KEEP_SESSION",5);
    g_stProcSysmanInfo.cfgSysmanInterval    = fcom_GetProfileInt("SYSMAN","CYCLE",10);
    g_stProcSysmanInfo.cfgPrmonInterval     = fcom_GetProfileInt("PRMON","INTERVAL",600);


    /* Sysman Queue Init */
    snprintf(pstQueueInfo->szDAPQueueHome,
            sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ",
             pstComnInfo->szDapHome);

    if (access(pstQueueInfo->szDAPQueueHome, R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            fstHandleSignal(15);
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in make queue directory(%s)",
                           pstQueueInfo->szDAPQueueHome);

        }
    }

    snprintf(pstQueueInfo->szPrmonQueueHome,
            sizeof(pstQueueInfo->szPrmonQueueHome),
            "%s/%s",
            pstQueueInfo->szDAPQueueHome,
            "PRMONQ");

    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue init, path(%s)",
                pstQueueInfo->szPrmonQueueHome);

        fstHandleSignal(15);
    }

    sprintf(pstQueueInfo->szDblogQueueHome,"%s/DBLOGQ", pstQueueInfo->szDAPQueueHome);

    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fqueue init, path(%s)",
                pstQueueInfo->szDblogQueueHome);

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
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_sysman");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_sysman");


   printf("Succeed in init, pid(%d)|%s\n",getpid(),__func__ );

    return RET_SUCC;
}


int fstCheckNotiFile(void)
{
    register int i;
    struct stat		statBuf;

    for(i = 0; i < MAX_SYSMAN_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in stat noti files(%s)",
                           g_pstNotiMaster[i].szNotiFileName);
            return	-1;
        }

        if(statBuf.st_mtime > g_pstNotiMaster[i].lastModify)
        {
            g_pstNotiMaster[i].reload	=	TRUE;
        }
        else
        {
            g_pstNotiMaster[i].reload	=	FALSE;
        }
    }
    return		TRUE;
}

int fstCheckAlaram(_DAP_DB_SYSTEM_LOAD_INFO     *pSysLoad,
                   _DAP_DB_FILSYSTEM_STAT       *pFs,
                   _DAP_DB_QUEUE_STAT 		    *pQs,
                   int                          nMount,
                   int                          nQueue,
                   _DAP_DB_THRESHOLD_INFO      *pThreshold
)
{
    int		i = 0;
    unsigned  int		maxValue	=	0;
    unsigned  int		maxQueue	=	0;
    char	alarmMountPath[256 +1] = {0x00,};
    char	temp[64 +1] = {0x00,};

    WRITE_INFO(CATEGORY_INFO,"Threshold: 0(%s)1(%s)2(%s)",
            pThreshold[0].category,
            pThreshold[1].category,
            pThreshold[2].category);

    if(!strcmp(pThreshold[0].category, "CPU"))
    {
        sprintf(temp, "%d", pSysLoad->cpu_idle);

        WRITE_CRITICAL(CATEGORY_INFO,"Checking(%s)",pThreshold[0].category );

        fsysdb_UpdateSysAlarm(&pThreshold[0], pSysLoad->cpu_idle, temp);
    }

    if(!strcmp(pThreshold[1].category, "DISK"))
    {
        for(i = 0; i < nMount; i++)
        {
            maxValue = fcom_GetBiggerNumber((int)maxValue, pFs[i].capacity);
            if(maxValue == pFs[i].capacity)
            {
                memset(alarmMountPath, 0x00, sizeof(alarmMountPath));
                sprintf(alarmMountPath, "%s %d", pFs[i].filesystem,pFs[i].capacity);
            }
        }

        WRITE_INFO(CATEGORY_DB,"Checking (%s)",
                   pThreshold[1].category);

        fsysdb_UpdateSysAlarm(&pThreshold[1], maxValue, alarmMountPath);
    }

    if(!strcmp(pThreshold[2].category, "MEMORY"))
    {
        sprintf(temp, "%d", pSysLoad->mem_perused);

        WRITE_INFO(CATEGORY_DB,"Checking (%s)",
                   pThreshold[2].category);

        fsysdb_UpdateSysAlarm(&pThreshold[2], pSysLoad->mem_perused, temp);
    }

    if(!strcmp(pThreshold[3].category, "QUEUE"))
    {
        for(i = 0; i < nQueue; i++)
        {
            maxQueue = fcom_GetBiggerNumber((int)maxQueue, pQs[i].used);
            if(maxQueue == pQs[i].used)
            {
                memset(temp, 0x00, sizeof(temp));
                sprintf(temp, "%s:%d", pQs[i].name,pQs[i].used);
            }
        }

        WRITE_INFO(CATEGORY_DB,"Checking (%s)",
                pThreshold[3].category);

        fsysdb_UpdateSysAlarm(&pThreshold[3], maxQueue, temp);
    }

    return 1;
}

static void fstReloadCfgFile( )
{
    g_stProcSysmanInfo.cfgSysmanInterval            = fcom_GetProfileInt("SYSMAN","CYCLE",10);
    g_stProcSysmanInfo.cfgPrmonInterval             = fcom_GetProfileInt("PRMON","INTERVAL",600);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize  = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel      = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel    = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay         = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n SYSMAN_CYCLE : [%d] \n PRMON_INTERVAL : [%d] \n "
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcSysmanInfo.cfgSysmanInterval,
              g_stProcSysmanInfo.cfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}