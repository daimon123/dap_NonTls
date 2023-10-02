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

int fw_ReloadCfgFile(void)
{
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel = fcom_GetProfileInt("DEBUG","LEVEL",1);

    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

    return 0;


}

int fw_MainTask(void)
{
    pid_t local_nPid = 0;
    struct sigaction saDeadChild;
    char    local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_POLICY_FW);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    saDeadChild.sa_handler = fw_SigchldHandler; // reap all dead processes
    sigemptyset(&saDeadChild.sa_mask);
    saDeadChild.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saDeadChild, NULL) == -1 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit(1);
    }

    WRITE_INFO(CATEGORY_INFO,"Start Program ... ");

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");


    g_stProcFwInfo.parentPid = getpid();

    /** 정책 데이터 Fork() 프로세스 **/
    g_nForkIdx = FORK_POLICY;
    if( (local_nPid = fork()) < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
        return (-1);
    }

    else if(local_nPid > 0)
    {
        g_stProcFwInfo.Pid[FORK_POLICY]      = local_nPid;
        g_stProcFwInfo.nForkIdx[FORK_POLICY] = g_nForkIdx;
    }
    // child process
    else if(local_nPid == 0)
    {
        fw_ForkWork();
        exit(0);
    }

    /**  데이터 Fork() 프로세스 **/
    g_nForkIdx = FORK_SERVICE;
    if( (local_nPid = fork()) < 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
        return (-1);
    }

    else if(local_nPid > 0)
    {
        g_stProcFwInfo.Pid[FORK_SERVICE]       = local_nPid;
        g_stProcFwInfo.nForkIdx[FORK_SERVICE]  = g_nForkIdx;
    }
    // child process
    else if(local_nPid == 0)
    {
        fw_ForkWork();
        exit(0);
    }

    while(1)
    {
        /** Main Nothing **/
        sleep(1);

    }

    fw_HandleSignal(15);

    return RET_SUCC;

}

