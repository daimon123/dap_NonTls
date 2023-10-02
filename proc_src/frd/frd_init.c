//
// Created by KimByoungGook on 2021-05-07.
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
#include "frd.h"
#include "dap_version.h"

static int fst_UnixSockInit(void)
{
    char local_UdsFileName[31 +1] = {0x00,};

    snprintf(local_UdsFileName, sizeof(local_UdsFileName), "%s", "SYSLOG");

    snprintf(g_szSysLogUnixSockPath, sizeof(g_szSysLogUnixSockPath), "%s/%s/%s",
             g_stServerInfo.stDapComnInfo.szDapHome, ".DAPQ", local_UdsFileName );

    if( (g_SyslogUdsSockFd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"UDP UDS Socket Create Fail (%d:%s)",errno, strerror(errno));
        return (-1);
    }

    if(access(g_szSysLogUnixSockPath, F_OK) != 0)
    {
        fcom_MkPath(g_szSysLogUnixSockPath, 0755);
    }

    bzero(&g_stSyslogUdsSockAddr, sizeof(struct sockaddr_un));
    g_stSyslogUdsSockAddr.sun_family = AF_UNIX;
    strcpy(g_stSyslogUdsSockAddr.sun_path, g_szSysLogUnixSockPath);


    WRITE_INFO(CATEGORY_DEBUG, "Unix Doamin Socket UDP Init Path : [%s] ", g_szSysLogUnixSockPath);


    return 0;
}

int frd_ReloadCfgFile(void)
{
    g_stProcFrdInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 120);

    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel = fcom_GetProfileInt("DEBUG","LEVEL",1);

    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n PRMON_INTERVAL : [%d] \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcFrdInfo.nCfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

    return 0;


}


int frd_Init(void)
{
    char local_szSyslogFlag[1 +1] = {0x00,};

    char szCfgTmp[255 +1]               = {0x00,};
    char local_szDefaultIP[15 +1]       = {0x00,};
    char local_szSyslogBinPath[255 +1]  = {0x00,};
    char local_szSyslogComand[255 +1]  = {0x00,};

    _DAP_COMN_INFO* pstComnInfo     = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             "%s"                           ,
             getenv("DAP_HOME")             );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                          ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        printf("Init Config Fail |%s\n", __func__ );
        return -1;
    }
    fcom_SetIniName(pstComnInfo->szComConfigFile);

    pstComnInfo->nCfgMgwId              = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    fsock_GetNic(szCfgTmp);
    fsock_GetIpAddress(szCfgTmp, local_szDefaultIP);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIP );
    pstComnInfo->nCfgKeepSession        = fcom_GetProfileInt("MYSQL","KEEP_SESSION", 5);
    g_stProcFrdInfo.nEorActive          = fcom_GetProfileInt("COMMON","EOR_ACTIVATION",1);
    g_stProcFrdInfo.nCfgAlarmActivation = fcom_GetProfileInt("ALARM","ACTIVATION", 0);
    g_stProcFrdInfo.nCfgPrmonInterval   = fcom_GetProfileInt("PRMON","INTERVAL", 120);
    g_stProcFrdInfo.nForkCnt            = fcom_GetProfileInt("PCIF","FORK_COUNT" ,3);
    g_stProcFrdInfo.nCfgMaxFileSize     = fcom_GetProfileInt("DBFILE", "MAX_DBFILE_SIZE", 10);

    g_stProcFrdInfo.nCfgMaxFileSize *= (1024 * 1024);
    if ( g_stProcFrdInfo.nCfgMaxFileSize > MAX_DBFILE_SIZE )
    {
        g_stProcFrdInfo.nCfgMaxFileSize = MAX_DBFILE_SIZE;
    }

    g_stProcFrdInfo.nCfgMaxFileIndex    = fcom_GetProfileInt("DBFILE", "MAX_DBFILE_INDEX", 9999);
    if ( g_stProcFrdInfo.nCfgMaxFileIndex > MAX_DBFILE_INDEX )
    {
        g_stProcFrdInfo.nCfgMaxFileIndex = MAX_DBFILE_INDEX;
    }

    /** Dump Flag **/
    if(strcasecmp(pstComnInfo->szDebugName, "dump" ) == 0)
    {
        g_stProcFrdInfo.nDumpFlag = 0x01;
    }
    else
    {
        g_stProcFrdInfo.nDumpFlag = 0x00;
    }

    /* Set EOR Path */
    fcom_GetProfile("COMMON","EOR_PATH",g_stProcFrdInfo.szEorHomeDir,"/home/intent/dap/log/eor");
    snprintf(g_stProcFrdInfo.szCpEorFile, sizeof(g_stProcFrdInfo.szCpEorFile),
             "%s/%s",
             g_stProcFrdInfo.szEorHomeDir,"eor");

    fcom_SetEorFile(g_stProcFrdInfo.szCpEorFile);

    g_stProcFrdInfo.nEorFormat = fcom_GetProfileInt("COMMON","EOR_FORMAT",2);

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

    snprintf(pstQueueInfo->szAlaramQueueHome,sizeof(pstQueueInfo->szAlaramQueueHome),
             "%s/ALARMQ",
             pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(ALARM_QUEUE, pstQueueInfo->szAlaramQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s) |%s\n", pstQueueInfo->szAlaramQueueHome, __func__);
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

    snprintf(g_stProcFrdInfo.szDtFilePath, sizeof(g_stProcFrdInfo.szDtFilePath),
             "%s/data/dt/", pstComnInfo->szDapHome );
    snprintf(g_stProcFrdInfo.szPolicyFilePath, sizeof(g_stProcFrdInfo.szPolicyFilePath),
             "%s/data/policy/", pstComnInfo->szDapHome);

    if( fcom_MkPath(g_stProcFrdInfo.szDtFilePath, 0755) != 0)
    {
        printf("Init Make Dir (%s) Failed \n", g_stProcFrdInfo.szDtFilePath);
        return (-1);
    }

    if( fcom_MkPath(g_stProcFrdInfo.szPolicyFilePath, 0755) != 0)
    {
        printf("Init Make Dir (%s) Failed \n", g_stProcFrdInfo.szPolicyFilePath);
        return (-1);
    }


    g_stProcFrdInfo.nCurTime =
    g_stProcFrdInfo.nLastJobTime =
            time(NULL);

    memset(szCfgTmp, 0x00, sizeof(szCfgTmp));
    snprintf(szCfgTmp, sizeof(szCfgTmp), "%s/%s/%s", pstComnInfo->szDapHome, "log", "stdout.log");

    if ( g_stProcFrdInfo.nDumpFlag == 0x00 )
    {
        fcom_InitStdLog(szCfgTmp);

        // syslog 이벤트 cfg check하여 y인경우 우리 syslog 프로세스 실행.
        fcom_GetProfile("EVENT_LOGGING","USE_FLAG", local_szSyslogFlag, "N");
        if ( local_szSyslogFlag[0] == 'y' || local_szSyslogFlag[0] == 'Y' )
        {
            // syslog config use flag 활성화시 syslog_mon 프로세스 기동시킨다.
            snprintf(local_szSyslogBinPath, sizeof(local_szSyslogBinPath), "%s/bin/%s",
                     pstComnInfo->szDapHome,
                     "syslog_mon");

            if ( fcom_fileCheckStatus( local_szSyslogBinPath ) == 0) // 파일있음
            {
                // 프로세스 실행
                snprintf(local_szSyslogComand, sizeof(local_szSyslogComand), "%s dap", local_szSyslogBinPath);
                WRITE_DEBUG(CATEGORY_DEBUG, "Event Syslog Execute (%s) ", local_szSyslogComand);
                system(local_szSyslogComand);

                // Unix Socket Init
                fst_UnixSockInit();
            }
            else
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"(%s) File Is Not Exist", local_szSyslogBinPath);
                exit(0);
            }
        }
    }

    printf("Success Init .. |%s\n",__func__ );

    return TRUE;

}
