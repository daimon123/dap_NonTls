//
// Created by KimByoungGook on 2020-10-30.
//


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "ipc/dap_qdef.h"

#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_req.h"
#include "json/dap_json.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "dbif.h"
#include "dap_version.h"



void fdbif_HandleSignal(int sid)
{
    int rxt = 0,CheckCnt = 0;
    int idx = 0;
    char szProcessName[32 +1] = {0x00,};


    /* 스레드 자원 해제 */
    /* 0x00 : 프로세스 초기값 */
    /* 0x10 : 프로세스 사용중 */
    /* 0x20 : 프로세스 종료상태 */
    for(idx = 0; idx < MAX_THREAD; idx++)
    {
        /* Thread 종료 상태로 변경 */
        g_pthread_stop_sign[idx] = 0x20;
    }
    idx = 0;
    for(;;)
    {
        if(CheckCnt > 3)
            sleep(1);

        if(idx > MAX_THREAD || CheckCnt > 5)
        {
            break;
        }
        /* 아직 스레드 사용중이면 처음부터 재 체크 */
        if(g_pthread_stop_sign[idx] == 0x20 || g_pthread_stop_sign[idx] == 0x10)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Running Status");
            idx = 0;
        }
        else
            idx++;
        CheckCnt++;
        sleep(0);
    }
    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Stop Status");

    fdb_SqlClose(g_stMyCon);
    rxt = fipc_SQSaveQ();

    WRITE_CRITICAL(CATEGORY_IPC, "Try save queue, rxt(%d)", rxt);

    fipc_SQClear();
    fipc_FQClose(ALARM_QUEUE);
    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);

    if      (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif01") == 0)
    { fipc_FQClose(DBIF01_QUEUE); strcpy(szProcessName,"dap_dbif01"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif02") == 0)
    { fipc_FQClose(DBIF02_QUEUE); strcpy(szProcessName,"dap_dbif02"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif03") == 0)
    { fipc_FQClose(DBIF03_QUEUE); strcpy(szProcessName,"dap_dbif03"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif04") == 0)
    { fipc_FQClose(DBIF04_QUEUE); strcpy(szProcessName,"dap_dbif04"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif05") == 0)
    { fipc_FQClose(DBIF05_QUEUE); strcpy(szProcessName,"dap_dbif05"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif06") == 0)
    { fipc_FQClose(DBIF06_QUEUE); strcpy(szProcessName,"dap_dbif06"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif07") == 0)
    { fipc_FQClose(DBIF07_QUEUE); strcpy(szProcessName,"dap_dbif07"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif08") == 0)
    { fipc_FQClose(DBIF08_QUEUE); strcpy(szProcessName,"dap_dbif08"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif09") == 0)
    { fipc_FQClose(DBIF09_QUEUE); strcpy(szProcessName,"dap_dbif09"); }
    else if (strcmp(g_stServerInfo.stDapComnInfo.szDebugName, "dbif10") == 0)
    { fipc_FQClose(DBIF10_QUEUE); strcpy(szProcessName,"dap_dbif10"); }

    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program, received signal(%d)pid(%d)",
                   sid,getpid());

    fFile_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,szProcessName);

    sleep(1);
    exit(sid);



}

int fdbif_ReloadCfgFile(void)
{
    g_stProcDbifInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 120);

    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel = fcom_GetProfileInt("DEBUG","LEVEL",1);

    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcDbifInfo.nCfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

    return 0;


}


void fdbif_SigHandler(void)
{
    signal(SIGHUP, fdbif_HandleSignal);    /* 1 : hangup */
    signal(SIGINT, fdbif_HandleSignal);    /* 2 : interrupt (rubout) */
    signal(SIGQUIT, fdbif_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL, fdbif_HandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP, fdbif_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT, fdbif_HandleSignal);    /* 6 : IOT instruction */
    signal(SIGABRT, fdbif_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGFPE, fdbif_HandleSignal);    /* 8 : floating point exception */
    signal(SIGKILL, fdbif_HandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal(SIGBUS, fdbif_HandleSignal);   /* 10: bus error */
    signal(SIGSEGV, fdbif_HandleSignal);   /* 11: segmentation violation */
    signal(SIGSYS, fdbif_HandleSignal);   /* 12: bad argument to system call */
    signal(SIGPIPE, fdbif_HandleSignal);   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM, fdbif_HandleSignal);   /* 14: alarm clock */
    signal(SIGTERM, fdbif_HandleSignal);   /* 15: software termination signal from kill */
    signal(SIGUSR1, fdbif_HandleSignal);   /* 16: user defined signal 1 */
    signal(SIGUSR2, fdbif_HandleSignal);   /* 17: user defined signal 2 */
    signal(SIGCLD, SIG_IGN);              /* 18: child status change */
    signal(SIGCHLD, SIG_IGN);             /* 18: child status change alias (POSIX) */
    signal(SIGPWR, fdbif_HandleSignal);   /* 19: power-fail restart */
//    sigignore(SIGWINCH);            /* 20: window size change */
    signal(SIGWINCH,SIG_IGN);            /* 20: window size change */
    signal(SIGURG, fdbif_HandleSignal);  /* 21: urgent socket condition */
    signal(SIGPOLL, fdbif_HandleSignal);  /* 22: pollable event occured */
    signal(SIGIO, fdbif_HandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal(SIGSTOP, fdbif_HandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal(SIGTSTP, fdbif_HandleSignal);  /* 24: user stop requested from tty */
    signal(SIGCONT, fdbif_HandleSignal);  /* 25: stopped process has been continued */
    signal(SIGTTIN, fdbif_HandleSignal);  /* 26: background tty read attempted */
    signal(SIGTTOU, fdbif_HandleSignal);  /* 27: background tty write attempted */
    signal(SIGVTALRM, fdbif_HandleSignal);/* 28: virtual timer expired */
    signal(SIGPROF, fdbif_HandleSignal);  /* 29: profiling timer expired */
    signal(SIGXCPU, fdbif_HandleSignal);  /* 30: exceeded cpu limit */
    signal(SIGXFSZ, fdbif_HandleSignal);  /* 31: exceeded file size limit */
}



int fdbif_DbifInit(void)
{
    char szCfgTmp[256 +1] = {0x00,};
    char szProcessName[32 +1] = {0x00,};
    _DAP_COMN_INFO* pstComnInfo = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo = NULL;

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")             );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        printf("Init Config Fail |%s\n",__func__ );
        return -1;
    }
    fcom_SetIniName(pstComnInfo->szComConfigFile);

    pstComnInfo->nCfgMgwId = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    pstComnInfo->nCfgKeepSession = fcom_GetProfileInt("MYSQL","KEEP_SESSION", 5);
    g_stProcDbifInfo.nEorActive = fcom_GetProfileInt("COMMON","EOR_ACTIVATION",1);
    g_stProcDbifInfo.nCfgAlarmActivation = fcom_GetProfileInt("ALARM","ACTIVATION", 0);
    g_stProcDbifInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 120);

    /*Set EOR Path */
    fcom_GetProfile("COMMON","EOR_PATH",g_stProcDbifInfo.szEorHomeDir,"/home/intent/dap/log/eor");
    snprintf(g_stProcDbifInfo.szCpEorFile, sizeof(g_stProcDbifInfo.szCpEorFile),
//             "%s/%s\0",
             "%s/%s",
             g_stProcDbifInfo.szEorHomeDir,"eor");

    fcom_SetEorFile(g_stProcDbifInfo.szCpEorFile);

    g_stProcDbifInfo.nEorFormat = fcom_GetProfileInt("COMMON","EOR_FORMAT",2);
    g_stProcDbifInfo.nRsltQsize = fcom_GetProfileInt("DBIF","SQ_SIZE", 1000);
    g_stProcDbifInfo.nRetryQLimitFailCount = fcom_GetProfileInt("DBIF","SQ_RETRY_LIMIT_COUNT",3);
    snprintf(g_stProcDbifInfo.szSaveFilePath,
             sizeof(g_stProcDbifInfo.szSaveFilePath),
             "%s/%s/%s",
             pstComnInfo->szDapHome,
             "back",
             strcmp(pstComnInfo->szDebugName,"dbif01") == 0 ? "SAVEQ01" :
             strcmp(pstComnInfo->szDebugName,"dbif02") == 0 ? "SAVEQ02" :
             strcmp(pstComnInfo->szDebugName,"dbif03") == 0 ? "SAVEQ03" :
             strcmp(pstComnInfo->szDebugName,"dbif04") == 0 ? "SAVEQ04" :
             strcmp(pstComnInfo->szDebugName,"dbif05") == 0 ? "SAVEQ05" :
             strcmp(pstComnInfo->szDebugName,"dbif06") == 0 ? "SAVEQ06" :
             strcmp(pstComnInfo->szDebugName,"dbif07") == 0 ? "SAVEQ07" :
             strcmp(pstComnInfo->szDebugName,"dbif08") == 0 ? "SAVEQ08" :
             strcmp(pstComnInfo->szDebugName,"dbif09") == 0 ? "SAVEQ09" : "SAVEQ10" );

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
            exit(0);
        }
    }
    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
             "%s/PRMONQ", pstQueueInfo->szDAPQueueHome);

    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s) |%s\n",
               pstQueueInfo->szPrmonQueueHome,
               __func__);
        exit(0);
    }

    memset(szCfgTmp, 0x00, sizeof(szCfgTmp));
    switch(pstComnInfo->szDebugName[5])
    {
        case '1':
            sprintf(szCfgTmp,"%s/DBIFQ01", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF01_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif01");
            break;
        case '2':
            sprintf(szCfgTmp,"%s/DBIFQ02", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF02_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s\n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif02");
            break;
        case '3':
            sprintf(szCfgTmp,"%s/DBIFQ03", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF03_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif03");
            break;
        case '4':
            sprintf(szCfgTmp,"%s/DBIFQ04", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF04_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif04");
            break;
        case '5':
            sprintf(szCfgTmp,"%s/DBIFQ05", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF05_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif05");
            break;
        case '6':
            sprintf(szCfgTmp,"%s/DBIFQ06", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF06_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif06");
            break;
        case '7':
            sprintf(szCfgTmp,"%s/DBIFQ07", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF07_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif07");
            break;
        case '8':
            sprintf(szCfgTmp,"%s/DBIFQ08", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF08_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif08");
            break;
        case '9':
            sprintf(szCfgTmp,"%s/DBIFQ09", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF09_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif09");
            break;
        case '0':
            sprintf(szCfgTmp,"%s/DBIFQ10", pstQueueInfo->szDAPQueueHome);
            if (fipc_FQGetInit(DBIF10_QUEUE, szCfgTmp) < 0)
            {
                printf("Fail in init, path(%s)|%s \n", szCfgTmp, __func__);
                exit(0);
            }
            strcpy(szProcessName,"dap_dbif10");
            break;
    }

    snprintf(pstQueueInfo->szAlaramQueueHome,sizeof(pstQueueInfo->szAlaramQueueHome),
             "%s/ALARMQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(ALARM_QUEUE, pstQueueInfo->szAlaramQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szAlaramQueueHome,__func__);
        exit(0);
    }

    snprintf(pstQueueInfo->szDblogQueueHome, sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szDblogQueueHome, __func__);
        exit(0);
    }

    g_stProcDbifInfo.nCurTime =
    g_stProcDbifInfo.nLastJobTime =
            time(NULL);

    /* ���μ���.pid ���丮 �˻� */
    sprintf(g_stServerInfo.stDapComnInfo.szPidPath,"%s%s",g_stServerInfo.stDapComnInfo.szDapHome,"/bin/var");
    if(access(g_stServerInfo.stDapComnInfo.szPidPath, W_OK) != 0 )
    {
        if (mkdir(g_stServerInfo.stDapComnInfo.szPidPath,S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in Pid File directory(%s)|%s\n",
                   g_stServerInfo.stDapComnInfo.szPidPath,
                   __func__ );
            return 0;
        }
    }

    /* ���μ���.pid ���ϰ˻� */
    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,szProcessName);
    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,szProcessName);

    memset(szCfgTmp, 0x00, sizeof(szCfgTmp));
    snprintf(szCfgTmp, sizeof(szCfgTmp), "%s/%s/%s",pstComnInfo->szDapHome,"log","stdout.log");

    fcom_InitStdLog(szCfgTmp);

    printf("Success Init .. |%s\n",__func__ );

    return TRUE;

}



void fdbif_WriteEor(_DAP_EventParam * p_EP)
{
    fcom_EorRet(g_stProcDbifInfo.nEorFormat,
                "%s\t%s\t%s\t%d\t%d\n",
                p_EP->user_key,
                p_EP->user_ip,
                p_EP->detect_time,
                p_EP->ev_type,
                p_EP->ev_level);
}



int fdbif_GetLengthDiskMobileRead(_DAP_DISK *p_Disk)
{
    int i = 0, len = 0;

    for(i=0; i<p_Disk->size; i++)
    {
        if(p_Disk->DiskValue[i].dk_drive_type == DK_REMOVABLE_DISK)
        {
            if(p_Disk->DiskValue[i].dk_access == DK_READABLE)		len = 1;
            else if(p_Disk->DiskValue[i].dk_access == DK_READ_WRITE)len = 1;
        }
    }

    return len;
}

int fdbif_GetLengthDiskMobileWrite(_DAP_DISK *p_Disk)
{
    int i = 0, len = 0;

    for (i = 0; i < p_Disk->size; i++)
    {
        if (p_Disk->DiskValue[i].dk_drive_type == DK_REMOVABLE_DISK)
        {
            if (p_Disk->DiskValue[i].dk_access == DK_WRITEABLE) len = 1;
            else if (p_Disk->DiskValue[i].dk_access == DK_READ_WRITE)len = 1;
            else if (p_Disk->DiskValue[i].dk_access == DK_WRITE_ONCE)len = 1;
        }
    }

    return len;
}
/* PCIF���� �ߺ�üũ �����Ͽ� ���� ����. */
/*int fdbif_CheckChangeBaseinfo(char* p_userKey, char *p_userIp, char *p_userMac, int *resExt)
{
    char    sqlBuf[128];
    int		rowCnt=0;
    int		resCnt=0;


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select HB_ACCESS_IP,HB_ACCESS_MAC,HB_EXTERNAL,HB_DEL from HW_BASE_TB "
                    "where HB_UNQ='%s'", p_userKey);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) |%s",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg,
                       __func__);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0) // only one value
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(strcmp(p_userIp, g_stMyCon->row[0])) //�ٸ���
                {
                    resCnt += 1;
                    WRITE_INFO(CATEGORY_DB, "User IP changed, cur(%s)db(%s)key(%s) |%s",
                            p_userIp,
                            g_stMyCon->row[0],
                            p_userKey,
                            __func__);
                }
            }
            if(g_stMyCon->row[1] != NULL)
            {
                if(strcmp(p_userMac, g_stMyCon->row[1])) //�ٸ���
                {
                    resCnt += 1;
                    WRITE_INFO(CATEGORY_DB, "User MAC changed, cur(%s)db(%s)key(%s) |%s",
                             p_userMac,
                             g_stMyCon->row[1],
                             p_userKey,
                             __func__);
                }
            }
            if(g_stMyCon->row[2] != NULL)
            {
                *resExt = atoi(g_stMyCon->row[2]);
            }
            if(g_stMyCon->row[3] != NULL)
            {
                resCnt += atoi(g_stMyCon->row[3]);
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"Proc resCnt(%d)resExt(%d) |%s",
            resCnt,*resExt,__func__);

    return resCnt;
}*/


int fdbif_CheckChangeBaseinfo(char* p_userKey,
                              char *p_userIp,
                              char *p_userMac,
                              int  del,
                              int  hb_external,
                              int *resExt)
{
    int resCnt = 0;


    /* HW_USER_IP */
    if(p_userIp[0] != 0x00)
    {
        resCnt += 1;
        WRITE_DEBUG(CATEGORY_DEBUG, "User IP changed, cur(%s)key(%s) ",
                    p_userIp,
                    p_userKey);

    }
    /* HW_BASE_MAC */
    if(p_userMac[0] != 0x00)
    {
        resCnt += 1;

        WRITE_DEBUG(CATEGORY_DEBUG, "User MAC changed, cur(%s)key(%s) ",
                    p_userIp,
                    p_userKey);

    }
    if(hb_external != 0)
    {
        *resExt = hb_external;
    }

    /* HW_BASE DEL */
    if(del != 0)
    {
        WRITE_DEBUG(CATEGORY_DEBUG, "User Deleted, cur(%s)key(%s) ",
                    p_userIp,
                    p_userKey);

        resCnt += 1;
    }

    WRITE_INFO(CATEGORY_DEBUG,	"HW_BASE Duplicate Check resCnt(%d)resExt(%d) ",
               resCnt,*resExt);

    return resCnt;
}