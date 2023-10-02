//
// Created by KimByoungGook on 2020-10-30.
//

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


int fmaster_MasterCommandCheck(void)
{
    char buffer[256 +1] = {0x00,};
    FILE* fp = NULL;

    fp = popen("which zip","r");

    WRITE_INFO(CATEGORY_DEBUG,"Master Process Init Cmd Check : %s ","which zip");

    if(fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fp is NULL ");
        exit(0);
    }
    fgets(buffer, 256, fp);

    if(strlen(buffer) <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"ZIP Command Is Not Found ");
    }

    pclose(fp);

    return 0;


}

int fmaster_GetPrintRlim(void)
{
    int nStatus = 0;
    struct rlimit stLimit;

    nStatus = getrlimit(RLIMIT_NOFILE, &stLimit);
    if (nStatus != 0)
    {
        WRITE_CRITICAL(CATEGORY_INFO,"System get open files failed");
        return 0;
    }
    else
    {
        WRITE_INFO(CATEGORY_INFO,"System get open files, cur=%lu max=%lu",
                   stLimit.rlim_cur,
                   stLimit.rlim_max);
    }
    return stLimit.rlim_max;
}

void fmaster_SetRlim(void)
{
    struct rlimit stLimit;

    stLimit.rlim_cur = 65536lu;
    stLimit.rlim_max = 65536lu;

    setrlimit(RLIMIT_NOFILE, &stLimit);
    getrlimit(RLIMIT_NOFILE, &stLimit);

    printf("System set open files, cur=%lu max=%lu\n",
           stLimit.rlim_cur, stLimit.rlim_max);
    WRITE_INFO(CATEGORY_INFO,"System set open files, cur=%lu max=%lu",
               stLimit.rlim_cur, stLimit.rlim_max);


}

//int fmaster_SetMasterPid(void)
//{
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
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_master");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_master");
//
//    return 0;
//}

int fmaster_DbReconnect(void)
{
    char smsmin[32][32];
    int  rxt = 0;
    char text[1024] = {0x00,};
    char stime[20] = {0x00,};

    int err_code = -1;
    g_ptrProcessInfo = (_PROCESS_INFO*)g_stProcessInfo;

    memset((char*)&smsmin, 0x00, sizeof(smsmin));
    memset((char*)text,    0x00, sizeof(text));
    memset((char*)stime,   0x00, sizeof(stime));

    while(err_code < 0)
    {
        fcom_Msleep(g_stServerInfo.stDapComnInfo.nArgSecInterval*1000);

        fmaster_GetProcessInfo(g_ptrProcessInfo);

        fmaster_InvokeProcess(g_ptrProcessInfo);
        fdb_SqlClose(g_stMyCon);

        rxt = fdb_ConnectDB();
        if(rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in db connection");
            err_code = -1;
        }
        else
        {
            WRITE_INFO(CATEGORY_DB,"Succeed in db connection");
            err_code = 1;
        }
    }/* while loop */

//    fmaster_GetProcessInfo(g_ptrProcessInfo, g_stSysProcInfo);
    fmaster_GetProcessInfo(g_ptrProcessInfo);

    return RET_SUCC;
}


int fmaster_CheckNotiFile(void)
{
    register int	i = 0;
    struct stat		statBuf;

    memset(&statBuf, 0x00, sizeof(struct stat));

    for(i = 0; i < MAX_MASTER_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in stat noti files(%s)(%d)",
                           g_pstNotiMaster[i].szNotiFileName,
                           i);

            printf("%s\n", strerror(errno));
            return	-1;
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

void fmaster_ReloadCfgFile(void)
{
    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);

    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n"
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}