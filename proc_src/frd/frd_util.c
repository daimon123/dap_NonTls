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
#include "frd.h"
#include "dap_version.h"


int frd_GetForkIdx(pid_t param_FindPid)
{
    int local_nLoop = 0;

    for(local_nLoop = 0; local_nLoop < g_stProcFrdInfo.nForkCnt+2; local_nLoop++)
    {
        if(g_stProcFrdInfo.Pid[local_nLoop] == param_FindPid)
        {
            g_nForkIdx = local_nLoop;
            return local_nLoop;
        }
    }
    return (-1);
}


int frd_ParentExit(int sid)
{
    int     local_nRetPid   = 0;
    int     local_status    = 0;
    int     local_nLoop     = 0;
    char    local_szPidPath[127 +1]     = {0x00,};
    char    local_szSyslogFlag[1 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program, received signal(%d)pid(%d)", sid, getpid());
//    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    /** SIGCHLD wait() function???? ??? ?????? ???? FLAG **/
    if(g_ChildExitFlag == 0)
    {
        g_ChildExitFlag = 1; // Fork Exit Status ????.
    }

    for( local_nLoop = 0; local_nLoop < g_stProcFrdInfo.nForkCnt +2; local_nLoop++) // ??? ???? Fork ?????? +1 ??? ??.
    {
        kill(g_stProcFrdInfo.Pid[local_nLoop], 15);
    }

    /** ??? ???(Child Process) ?????? ???? ??? **/
    wait(&local_status);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();

    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_FRD);

    fipc_FQClose(DBLOG_QUEUE);

    if ( g_SyslogUdsSockFd > 0 )
    {
        close(g_SyslogUdsSockFd);
        g_SyslogUdsSockFd = 0;
    }

    /** Cleanup Master PID File **/
    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    fcom_GetProfile("EVENT_LOGGING","USE_FLAG", local_szSyslogFlag, "N");
    if ( local_szSyslogFlag[0] == 'y' || local_szSyslogFlag[0] == 'Y' )
    {

        local_nRetPid = fFile_GetMasterPid(local_szPidPath, "syslog_mon");
        if(local_nRetPid > 0)
        {
            WRITE_INFO(CATEGORY_INFO,"Try In Kill %d ",local_nRetPid);
            kill(local_nRetPid, 15);
            fFile_cleanupMasterPid( local_szPidPath, "syslog_mon");
        }
    }



    return 0;
}

int frd_ChildExit(void)
{
    WRITE_DEBUG(CATEGORY_DEBUG,"Get Set Position (%d)", g_nForkIdx)

    if ( g_nForkIdx >= FORK_DETECT)
    {
        frd_SetLastPosition();
    }

    if ( g_stProcFrdInfo.Fp != NULL)
    {
        fclose(g_stProcFrdInfo.Fp);
        g_stProcFrdInfo.Fp = NULL;
    }

    if ( g_stProcFrdInfo.SqlFailFp != NULL )
    {
        fclose( g_stProcFrdInfo.SqlFailFp );
        g_stProcFrdInfo.SqlFailFp = NULL;
    }

    fdb_SqlClose(g_stMyCon);

    sleep(1);

    return 0;
}


void frd_WriteEor(_DAP_EventParam * p_EP)
{
    fcom_EorRet(g_stProcFrdInfo.nEorFormat,
                "%s\t%s\t%s\t%d\t%d\n",
                p_EP->user_key,
                p_EP->user_ip,
                p_EP->detect_time,
                p_EP->ev_type,
                p_EP->ev_level);
}



int frd_GetLengthDiskMobileRead(_DAP_DISK *p_Disk)
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

int frd_GetLengthDiskMobileWrite(_DAP_DISK *p_Disk)
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



int frd_CheckChangeBaseinfo(char* p_userKey,
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