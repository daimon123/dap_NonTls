//
// Created by KimByoungGook on 2020-06-16.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"

#include "prmon.h"
#include "dap_version.h"
#include "sock/dap_sock.h"

static void fstHandleSignal(int sid);
static void fstSigHandler();
static void fstReloadCfgFile( );

int main(int argc, char **argv)
{
    int nRet;
    int nThrVal = 0;
    int nCurrMin;
    pthread_t t_ThreadMon;

    char    local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    nRet = RET_SUCC;

    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    /* Process Config, Log Init */
    fstPrmonInit();

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PRMON);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    WRITE_DEBUG(CATEGORY_DEBUG,"Make Master Pid : [%s/%s]", local_szPidPath, local_ptrProcessName);
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    fstPrmonSetTime(INIT_TIME);

    fstSigHandler();

    WRITE_INFO(CATEGORY_INFO, "Start programs");
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    if(fcom_ThreadCreate((void *)&t_ThreadMon, fstThreadMon, (void *)&nThrVal,4 * 1024 * 1024) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
        return -1;
    }

    while(1)
    {
        nCurrMin = fcom_GetSysMinute();

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcPrmonInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }

        nRet = fstRecvFromProc();
        if (nRet < 0)
            break;

        usleep(10000);

        WRITE_DEBUG(CATEGORY_DEBUG, "Prmon Main is Working.. ");
    }

    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
        g_pstNotiMaster = NULL;
    }

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    return RET_SUCC;

}


static int fstPrmonInit(void)
{
    int  nRetVal = 0;
    char szConfFile[127 +1]  = {0x00,};
    char local_szTmp[255 +1] = {0x00,};
    char local_szDefaultIP[15 +1] = {0x00,};
    _DAP_COMN_INFO*     pstComnInfo = NULL;
    _DAP_QUEUE_INFO*    pstQueueInfo = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    memset(szConfFile, 0x00, sizeof(szConfFile));

    pstComnInfo = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome          ,
             sizeof(pstComnInfo->szDapHome  ),
             "%s"                     ,
             getenv("DAP_HOME")       );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                     );


    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        nRetVal = (-1);
        goto __goto_end__;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    fsock_GetNic(local_szTmp);
    fsock_GetIpAddress(local_szTmp, local_szDefaultIP);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIP );

    g_stProcPrmonInfo.cfgCdrHeng = fcom_GetProfileInt("PRMON","TIMEOUT",600);
    g_stProcPrmonInfo.cfgProcCount = fcom_GetProfileInt("DBIF","PROC_COUINT",1);
//    g_stProcPrmonInfo.cfgLinkerActive = fcom_GetProfileInt("LINKER","ACTIVATION",1);
    g_stProcPrmonInfo.cfgProxyActive =  fcom_GetProfileInt("PROXY","ACTIVATION",1);
    g_stProcPrmonInfo.cfgSysmanActive = fcom_GetProfileInt("SYSMAN","ACTIVATION",1);
    g_stProcPrmonInfo.cfgFwderActive = fcom_GetProfileInt("FWDER","ACTIVATION",1);
    g_stProcPrmonInfo.cfgSchdActive = fcom_GetProfileInt("SCHD","ACTIVATION",1);
    g_stProcPrmonInfo.cfgReportActive = fcom_GetProfileInt("REPORT","ACTIVATION",1);
    g_stProcPrmonInfo.cfgAlarmActive = fcom_GetProfileInt("ALARM","ACTIVATION",1);
    g_stProcPrmonInfo.cfgReplmonActive = fcom_GetProfileInt("REPLMON","ACTIVATION",1);
    g_stProcPrmonInfo.cfgVwlogActive = fcom_GetProfileInt("VWLOG","ACTIVATION",1);

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    snprintf(pstQueueInfo->szDAPQueueHome,
             sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ",
             pstComnInfo->szDapHome);


    if(access(pstQueueInfo->szDAPQueueHome, R_OK) != 0 )
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s)|%s\n",
                   pstQueueInfo->szDAPQueueHome,
                           __func__ );
        }
    }

//    /* Master Process PID 저장 */
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

    snprintf(pstQueueInfo->szDblogQueueHome,
             sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
             pstQueueInfo->szDAPQueueHome);

    if(fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)|%s\n",
                       pstQueueInfo->szDblogQueueHome,
                       __func__ );
    }

    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),"%s/PRMONQ",
            pstQueueInfo->szDAPQueueHome);


    if (fipc_FQGetInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)|%s\n",
               pstQueueInfo->szPrmonQueueHome,__func__ );
    }

    snprintf(g_stProcPrmonInfo.cfgAccount, sizeof(g_stProcPrmonInfo.cfgAccount),
            "%s",
            getenv("USER"));

    snprintf(g_stProcPrmonInfo.cfgDapTmpFile, sizeof(g_stProcPrmonInfo.cfgDapTmpFile),
             "/tmp/.%s_proc_lock.tmp",
             g_stProcPrmonInfo.cfgAccount);

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

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */
    return RET_SUCC;

__goto_end__:
    printf("%s Fail RetValue = %d \n", __FUNCTION__, nRetVal);
    return RET_FAIL;

}

void fstPrmonSetTime(enum ENUM_TIME_FORMAT nPrmonType)
{
    switch(nPrmonType)
    {
        case INIT_TIME:
        {
            g_stProcPrmonInfo.nLastProxyTime =
//            g_stProcPrmonInfo.nLastLinkerTime =
            g_stProcPrmonInfo.nLastDbif01Time =
            g_stProcPrmonInfo.nLastDbif02Time =
            g_stProcPrmonInfo.nLastDbif03Time =
            g_stProcPrmonInfo.nLastDbif04Time =
            g_stProcPrmonInfo.nLastDbif05Time =
            g_stProcPrmonInfo.nLastPcifTime =
            g_stProcPrmonInfo.nLastSysmanTime =
            g_stProcPrmonInfo.nLastSchdTime =
            g_stProcPrmonInfo.nLastDblogTime =
            g_stProcPrmonInfo.nLastReportTime =
            g_stProcPrmonInfo.nLastFwderTime =
            g_stProcPrmonInfo.nLastAlarmTime =
            g_stProcPrmonInfo.nLastReplmonTime =
            g_stProcPrmonInfo.nLastVwlogTime =  time(0);
            break;
        }
        case PROXY_TIME:
            g_stProcPrmonInfo.nLastProxyTime = time(0);
            break;
//        case LINKER_TIME:
//            g_stProcPrmonInfo.nLastLinkerTime = time(0);
//            break;
        case DBIF01_TIME:
            g_stProcPrmonInfo.nLastDbif01Time = time(0);
            break;
        case DBIF02_TIME:
            g_stProcPrmonInfo.nLastDbif02Time = time(0);
            break;
        case DBIF03_TIME:
            g_stProcPrmonInfo.nLastDbif03Time = time(0);
            break;
        case DBIF04_TIME:
            g_stProcPrmonInfo.nLastDbif04Time = time(0);
            break;
        case DBIF05_TIME:
            g_stProcPrmonInfo.nLastDbif05Time = time(0);
            break;
        case PCIF_TIME:
            g_stProcPrmonInfo.nLastPcifTime = time(0);
            break;
        case SYSMAN_TIME:
            g_stProcPrmonInfo.nLastSysmanTime = time(0);
            break;
        case SCHD_TIME:
            g_stProcPrmonInfo.nLastSchdTime = time(0);
            break;
        case DBLOG_TIME:
            g_stProcPrmonInfo.nLastDblogTime = time(0);
            break;
        case REPORT_TIME:
            g_stProcPrmonInfo.nLastReportTime = time(0);
            break;
        case FWDER_TIME:
            g_stProcPrmonInfo.nLastFwderTime = time(0);
            break;
        case ALARM_TIME:
            g_stProcPrmonInfo.nLastAlarmTime = time(0);
            break;
        case REPLMON_TIME:
            g_stProcPrmonInfo.nLastReplmonTime = time(0);
            break;
        case VWLOG_TIME:
            g_stProcPrmonInfo.nLastVwlogTime = time(0);
            break;
    }
}

int fstCheckDaemon(enum ENUM_TIME_FORMAT nPrmonType)
{
    long check_time = 0;
    long LastTime = 0;

    check_time = time(0) - g_stProcPrmonInfo.cfgCdrHeng;

    switch(nPrmonType)
    {
        case INIT_TIME:
            break;
        case PROXY_TIME:
            LastTime = g_stProcPrmonInfo.nLastProxyTime;
            break;
//        case LINKER_TIME:
//            LastTime = g_stProcPrmonInfo.nLastLinkerTime;
//            break;
        case DBIF01_TIME:
            LastTime = g_stProcPrmonInfo.nLastDbif01Time;
            break;
        case DBIF02_TIME:
            LastTime = g_stProcPrmonInfo.nLastDbif02Time;
            break;
        case DBIF03_TIME:
            LastTime = g_stProcPrmonInfo.nLastDbif03Time;
            break;
        case DBIF04_TIME:
            LastTime = g_stProcPrmonInfo.nLastDbif04Time;
            break;
        case DBIF05_TIME:
            LastTime = g_stProcPrmonInfo.nLastDbif05Time;
            break;
        case PCIF_TIME:
            LastTime = g_stProcPrmonInfo.nLastPcifTime;
            break;
        case SYSMAN_TIME:
            LastTime = g_stProcPrmonInfo.nLastSysmanTime;
            break;
        case SCHD_TIME:
            LastTime = g_stProcPrmonInfo.nLastSchdTime;
            break;
        case DBLOG_TIME:
            LastTime = g_stProcPrmonInfo.nLastDblogTime;
            break;
        case REPORT_TIME:
            LastTime = g_stProcPrmonInfo.nLastReportTime;
            break;
        case FWDER_TIME:
            LastTime = g_stProcPrmonInfo.nLastFwderTime;
            break;
        case ALARM_TIME:
            LastTime = g_stProcPrmonInfo.nLastAlarmTime;
            break;
        case REPLMON_TIME:
            LastTime = g_stProcPrmonInfo.nLastReplmonTime;
            break;
        case VWLOG_TIME:
            LastTime = g_stProcPrmonInfo.nLastVwlogTime;
            break;
    }

    if (check_time > LastTime)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in check_time (%ld) > %s (%ld)",
                       check_time,
                       (nPrmonType == PROXY_TIME) ?  "PROXY"   :
//                       (nPrmonType == LINKER_TIME) ? "LINKER"  :
                       (nPrmonType == DBIF01_TIME) ? "DBIF01"  :
                       (nPrmonType == DBIF02_TIME) ? "DBIF02"  :
                       (nPrmonType == DBIF03_TIME) ? "DBIF03"  :
                       (nPrmonType == DBIF04_TIME) ? "DBIF04"  :
                       (nPrmonType == DBIF05_TIME) ? "DBIF05"  :
                       (nPrmonType == PCIF_TIME)   ? "PCIF"    :
                       (nPrmonType == SYSMAN_TIME) ? "SYSMAN"  :
                       (nPrmonType == SCHD_TIME)   ? "SCHD"    :
                       (nPrmonType == DBLOG_TIME)  ? "DBLOG"   :
                       (nPrmonType == REPORT_TIME) ? "REPORT"  :
                       (nPrmonType == FWDER_TIME)  ? "FWDER"   :
                       (nPrmonType == ALARM_TIME)  ? "ALARM"   :
                       (nPrmonType == REPLMON_TIME)? "REPLMON" : "VWLOG",
                       LastTime  );
        return RET_FAIL;
    }

    return RET_SUCC;
}
void* fstThreadMon(void *thrid)
{
    int thrIdx;

    thrIdx = *(int *)thrid;

    while(1)
    {
        if ( g_pthreadStopFlag != 0x00 )
            break;

        if (g_stProcPrmonInfo.cfgProxyActive == TRUE)
        {
            if (g_stProcPrmonInfo.cfgProcCount > 0 && fstCheckDaemon(PROXY_TIME) < 0)
            {
                fstPrmonSetTime(PROXY_TIME);
                fstKillProc("dap_proxy");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_proxy, maybe lock state" );
            }
        }
//        if (g_stProcPrmonInfo.cfgLinkerActive == 1)
//        {
//            if (g_stProcPrmonInfo.cfgProcCount > 0 && fstCheckDaemon(LINKER_TIME) < 0)
//            {
//                fstPrmonSetTime(LINKER_TIME);
//                fstKillProc("dap_linker");
//                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_linker, maybe lock state|%s",__func__ );
//
//            }
//        }

        if (fstCheckDaemon(PCIF_TIME) < 0)
        {
            fstPrmonSetTime(PCIF_TIME);
            fstKillProc("dap_pcif");
            WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_pcif, maybe lock state" );
        }

        if (g_stProcPrmonInfo.cfgSysmanActive == 1)
        {
            if (fstCheckDaemon(SYSMAN_TIME) < 0)
            {
                fstPrmonSetTime(SYSMAN_TIME);
                fstKillProc("dap_sysman");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_sysman, maybe lock state" );
            }
        }
        if (g_stProcPrmonInfo.cfgSchdActive == 1)
        {
            if (fstCheckDaemon(SCHD_TIME) < 0)
            {
                fstPrmonSetTime(SCHD_TIME);
                fstKillProc("dap_schd");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_schd, maybe lock state" );
            }
        }
        if (fstCheckDaemon(DBLOG_TIME) < 0)
        {
            fstPrmonSetTime(DBLOG_TIME);
            fstKillProc("dap_dblog");
            WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_dblog, maybe lock state" );
        }

        if (g_stProcPrmonInfo.cfgReportActive == 1)
        {
            if (fstCheckDaemon(REPORT_TIME) < 0)
            {
                fstPrmonSetTime(REPORT_TIME);
                fstKillProc("dap_report");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_report, maybe lock state" );
            }
        }

        if (g_stProcPrmonInfo.cfgFwderActive == 1)
        {
            if (fstCheckDaemon(FWDER_TIME) < 0)
            {
                fstCheckDaemon(FWDER_TIME);
                fstKillProc("dap_fwder");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_fwder, maybe fwder lock state" );
            }
        }

        if (g_stProcPrmonInfo.cfgAlarmActive == 1)
        {
            if (fstCheckDaemon(ALARM_TIME) < 0)
            {
                fstPrmonSetTime(ALARM_TIME);
                fstKillProc("dap_alarm");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_alarm, maybe alarm lock state" );
            }
        }
        if (g_stProcPrmonInfo.cfgReplmonActive == 1)
        {
            if (fstCheckDaemon(REPLMON_TIME) < 0)
            {
                fstPrmonSetTime(REPLMON_TIME);
                fstKillProc("dap_replmon");
                WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_replmon, maybe replmon lock state" );
            }
        }

        if (fstCheckDaemon(VWLOG_TIME) < 0)
        {
            fstPrmonSetTime(VWLOG_TIME);
            fstKillProc("dap_vwlog");
            WRITE_CRITICAL(CATEGORY_DEBUG,"Killed dap_vwlog, maybe vwlog lock state" );
        }

        sleep(5);
    }

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ",thrIdx);

    pthread_exit(NULL);
}

/*
static int fstKillProc( char *p_name )
{
    char CmdStr[128];
    FILE *r_file;

    char buff[256];
    char pbuff[10];
    char tbuff[10];
    int  i,j,slen;
    int  pid;

    sprintf(CmdStr, "ps -u %s | grep %s > %s",  g_stProcPrmonInfo.cfgAccount,
                                                p_name,
                                                g_stProcPrmonInfo.cfgDapTmpFile);
    system(CmdStr);

    r_file = fopen(g_stProcPrmonInfo.cfgDapTmpFile, "rt");
    if (r_file == NULL)
        return -1;

    if (fgets(buff, 256, r_file) == NULL)
    {
        fclose(r_file);
        return -1;
    }


    memset(tbuff, 0x00, sizeof(tbuff));
    memset(pbuff, 0x00, sizeof(pbuff));
    memcpy(pbuff, buff, 6);

    slen = strlen(pbuff);

    if (slen < 2)
        return -2;

    for (i=0,j=0;i<slen;i++)
    {
        if ((pbuff[i] >= '0') && (pbuff[i] <= '9'))
        {
            tbuff[j++] = pbuff[i];
        }
    }

    pid = atoi(tbuff);

    if (pid > 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Try in kill -9 %d",pid);
        kill(pid, 9);
    }

    fclose(r_file);

    return 0;
}
*/

static int fstKillProc( char *p_name )
{
    int nRetPid;

    nRetPid = fFile_GetMasterPid(g_stServerInfo.stDapComnInfo.szPidPath, p_name);
    if(nRetPid > 0)
    {
        WRITE_INFO(CATEGORY_INFO,"Try In Kill %d ",nRetPid);
        kill(nRetPid, 15);
    }
    else
        return 0;

    nRetPid = fFile_GetMasterPid(g_stServerInfo.stDapComnInfo.szPidPath, p_name);
    if(nRetPid > 0)
    {
        WRITE_INFO(CATEGORY_INFO,"Try In Kill %d ",nRetPid);
        kill(nRetPid, 15);
    }
    else
        return 0;

    return 0;
}


int fstRecvFromProc(void)
{
    int		  nRet;
    int		  msgType;
    _DAP_PING stPing;

    nRet = fipc_FQGetData(PRMON_QUEUE, (char *)&stPing, sizeof(stPing));
    if(nRet == 0)
    {
        sleep(1);
        return 0;
    }
    if(nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue get data" );
        return -1;
    }
    //LogDRet(5, "=====> FQ����[%d]\n", rxt);
    msgType = atoi(stPing.msgtype);
    if(msgType != PRMON_LINKER)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Unknown MsgType(%d)",msgType );
        return 0;
    }

    WRITE_CRITICAL(CATEGORY_DEBUG,"Receive Prmon From(%s) ",
            stPing.procname);

    if(strncmp(stPing.procname, "proxy", 5) == 0)
        fstPrmonSetTime(PROXY_TIME);
//    else if(strncmp(stPing.procname, "linker", 6) == 0)
//        fstPrmonSetTime(LINKER_TIME);
    else if(strncmp(stPing.procname, "dbif01", 6) == 0)
        fstPrmonSetTime(DBIF01_TIME);
    else if(strncmp(stPing.procname, "dbif02", 6) == 0)
        fstPrmonSetTime(DBIF02_TIME);
    else if(strncmp(stPing.procname, "dbif03", 6) == 0)
        fstPrmonSetTime(DBIF03_TIME);
    else if(strncmp(stPing.procname, "dbif04", 6) == 0)
        fstPrmonSetTime(DBIF04_TIME);
    else if(strncmp(stPing.procname, "dbif05", 6) == 0)
        fstPrmonSetTime(DBIF05_TIME);
    else if(strncmp(stPing.procname, "pcif", 4) == 0)
        fstPrmonSetTime(PCIF_TIME);
    else if(strncmp(stPing.procname, "sysman", 6) == 0)
        fstPrmonSetTime(SYSMAN_TIME);
    else if(strncmp(stPing.procname, "schd", 4) == 0)
        fstPrmonSetTime(SCHD_TIME);
    else if(strncmp(stPing.procname, "dblog", 5) == 0)
        fstPrmonSetTime(DBLOG_TIME);
    else if(strncmp(stPing.procname, "report", 5) == 0)
        fstPrmonSetTime(REPORT_TIME);
    /*
    else if(strncmp(stPing.procname, "thread", 6) == 0)
    {
        init_thread_time();
    }
    */
    else if(strncmp(stPing.procname, "fwder", 5) == 0)
        fstPrmonSetTime(FWDER_TIME);
    else if(strncmp(stPing.procname, "alarm", 5) == 0)
        fstPrmonSetTime(ALARM_TIME);
    else if(strncmp(stPing.procname, "replmon", 7) == 0)
        fstPrmonSetTime(REPLMON_TIME);
    else if(strncmp(stPing.procname, "vwlog", 5) == 0)
        fstPrmonSetTime(VWLOG_TIME);

    return RET_SUCC;
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
    signal(SIGPIPE,SIG_IGN );              /* 13: write on a pipe with no one to read it */

    signal( SIGALRM, fstHandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fstHandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fstHandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fstHandleSignal);   /* 17: user defined signal 2 */
    signal(SIGCLD, SIG_IGN );              /* 18: child status change */
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
    char*   local_ptrProcessName = NULL;

    WRITE_CRITICAL(CATEGORY_DEBUG,"Got signal(%d)",sid );
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PRMON);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Stop Status");

    fipc_FQClose(DBLOG_QUEUE);


    if (g_pthreadStopFlag == 0x00)
    {
        g_pthreadStopFlag = 0x01;
    }

    sleep(2);

    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
        g_pstNotiMaster = NULL;
    }

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(0);
}


static void fstReloadCfgFile( )
{
    /* Alarm Set Reload */
    g_stProcPrmonInfo.cfgCdrHeng = fcom_GetProfileInt("PRMON","TIMEOUT",600);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_TIMEOUT : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcPrmonInfo.cfgCdrHeng,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}