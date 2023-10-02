//
// Created by KimByoungGook on 2021-03-10.
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
#include "fw.h"
#include "dap_version.h"

int fw_FwInit(void)
{
    char szTemp[255 +1]             = {0x00,};
    char local_szDefaultIP[15 +1]   = {0x00,};
    _DAP_COMN_INFO*  pstComnInfo    = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")      );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                         ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        printf("Init Config Fail |%s\n",__func__ );
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    fsock_GetNic(szTemp);
    fsock_GetIpAddress(szTemp, local_szDefaultIP);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIP );

    g_stProcFwInfo.nCfgMaxFileSize = fcom_GetProfileInt("DBFILE", "MAX_DBFILE_SIZE", 10);
    g_stProcFwInfo.nCfgMaxFileSize *= (1024 * 1024);
    if ( g_stProcFwInfo.nCfgMaxFileSize > MAX_DBFILE_SIZE )
    {
        g_stProcFwInfo.nCfgMaxFileSize = MAX_DBFILE_SIZE;
    }

    g_stProcFwInfo.nCfgMaxFileIndex = fcom_GetProfileInt("DBFILE", "MAX_DBFILE_INDEX", 9999);
    if ( g_stProcFwInfo.nCfgMaxFileIndex > MAX_DBFILE_INDEX )
    {
        g_stProcFwInfo.nCfgMaxFileIndex = MAX_DBFILE_INDEX;
    }

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

    /** 신규 dap_fw 프로세스용 Unix Socket Init **/
    /** 정책 데이터 **/
    memset( szTemp, 0x00 ,sizeof(szTemp) );
    sprintf(szTemp,"%s/FW_POLICY", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQGetInit(FW_POLICY, szTemp) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue init, path(%s)", szTemp);
        return RET_FAIL;
    }

    /** Agent 서비스 상태 데이터 **/
    memset( szTemp, 0x00 ,sizeof(szTemp) );
    sprintf(szTemp,"%s/FW_SERVICE", pstQueueInfo->szDAPQueueHome);
    if (fipc_FQGetInit(FW_SERVICE, szTemp) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fqueue init, path(%s)", szTemp);
        return RET_FAIL;
    }

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

    snprintf(g_stProcFwInfo.szPolicyFilePath, sizeof(g_stProcFwInfo.szPolicyFilePath),
             "%s/data/policy/", pstComnInfo->szDapHome);

    if( fcom_MkPath(g_stProcFwInfo.szPolicyFilePath, 0755) != 0)
    {
        printf("Init Make Dir (%s) Failed \n", g_stProcFwInfo.szPolicyFilePath);
        return (-1);
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

    printf("Success Init .. |%s\n",__func__ );

    return TRUE;

}

