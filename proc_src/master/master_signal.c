//
// Created by KimByoungGook on 2021-06-14.
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

void fmaster_GuardSignal(void)
{
    signal(SIGHUP,    fmaster_HandleSignal);   /* 1 : hangup */
    signal(SIGINT,    fmaster_HandleSignal);   /* 2 : interrupt (rubout) */
    signal(SIGQUIT,   fmaster_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL,    fmaster_HandleSignal);   /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP,   fmaster_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT,    fmaster_HandleSignal);   /* 6 : IOT instruction */

    signal(SIGABRT,   fmaster_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGBUS,    fmaster_HandleSignal);   /* 7 : BUS error (4.2 BSD). */
    signal(SIGFPE,    fmaster_HandleSignal);   /* 8 : floating point exception */
    signal(SIGKILL,   fmaster_HandleSignal);   /* 9 : kill (cannot be caught or ignored) */
    signal(SIGUSR1,   fmaster_HandleSignal);   /* 10: User-defined signal 1 */

    signal(SIGSEGV,   fmaster_HandleSignal);   /* 11: segmentation violation */
    signal(SIGUSR2,   fmaster_HandleSignal);   /* 12: User-defined signal 2 (POSIX).*/
    signal(SIGPIPE,   SIG_IGN);   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM,   fmaster_HandleSignal);   /* 14: alarm clock */
    signal(SIGTERM,   fmaster_HandleSignal);   /* 15: software termination signal from kill */
    signal(SIGSTKFLT, fmaster_HandleSignal);   /* 16: Stack fault. */
    signal(SIGCHLD, SIG_IGN);                /* 17: Child status has changed */
    signal(SIGCONT, SIG_IGN);                /* 18: Continue (POSIX). */
    signal(SIGSTOP,   fmaster_HandleSignal);   /* 19: Stop, unblockable */
    signal(SIGTSTP ,  fmaster_HandleSignal);   /* 20: Keyboard stop */
    signal(SIGTTIN ,  fmaster_HandleSignal);   /* 21: background tty read attempted */
    signal(SIGTTOU  , fmaster_HandleSignal);   /* 22: Background write to tty  */
    signal(SIGURG  ,  fmaster_HandleSignal);   /* 23: Urgent condition on socket  */
    signal(SIGXCPU ,  fmaster_HandleSignal);   /* 24: exceeded cpu limit */
    signal(SIGXFSZ ,  fmaster_HandleSignal);   /* 25: exceeded file size limit */
    signal(SIGVTALRM ,fmaster_HandleSignal);   /* 26: Virtual alarm clock */
    signal(SIGPROF ,  fmaster_HandleSignal);   /* 27: profiling timer expired */
    signal(SIGWINCH,  fmaster_HandleSignal);   /* 28: Window size change */
    signal(SIGPOLL ,  fmaster_HandleSignal);   /* 29: I/O now possible  */
    signal(SIGPWR ,   fmaster_HandleSignal);   /* 30: power-fail restart */
    signal(SIGSYS ,   fmaster_HandleSignal);   /* 31: Bad system call */
    //    sigignore( SIGWINCH );            /* 20: window size change */
}

void fmaster_HandleSignal(int sigNo)
{
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    WRITE_CRITICAL(CATEGORY_DEBUG,"Get Signal(%d) ",sigNo);
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sigNo);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_MASTER);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    fmaster_CleanPinfo(g_stProcessInfo, gProcess);

    fmaster_KillAllProcess(g_stProcessInfo);

    fmaster_DeleteMngQueue();

    fdb_CleanAlarm();

    fdb_SqlClose(g_stMyCon);

    WRITE_DEBUG(CATEGORY_DEBUG,"Process Deinit End ");

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    fmaster_ExitServer();
}

int fmaster_CleanPinfo(_PROCESS_INFO *pinfo, int nProcess)
{
    register int loop;

    unsigned int	sqlMgwId = 0;
    int				updateCnt = 0;
    char			sqlPName[21];
    char			sqlArgs[151];
    char			sqlBuf[256];

    WRITE_INFO(CATEGORY_DB, "nProcess(%d)", nProcess );

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    for(loop = 0; loop < nProcess; loop++)
    {
        memset(sqlArgs, 0x00, sizeof(sqlArgs));
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        memset(sqlPName, 0x00, sizeof(sqlPName));

        sqlMgwId		=		pinfo[loop].mgwid;


        strcpy(sqlPName,pinfo[loop].pname);

        if(!strcmp(pinfo[loop].args, ""))
        {
            strcpy(sqlArgs, "NULL");
        }
        else
        {
            strcpy(sqlArgs, pinfo[loop].args);
        }

        WRITE_INFO(CATEGORY_INFO,"mgwid(%d)pname(%s)args(%s)",
                   sqlMgwId,
                   sqlPName,
                   sqlArgs);

        sprintf(sqlBuf, "update SYS_PINFO_TB set "
                        "STATUS='', "
                        "PID=0, "
                        "PPID=0, "
                        "STIME=0, "
                        "KTIME=0, "
                        "PCPU=0, "
                        "PSIZE=0, "
                        "PPRIORITY=0, "
                        "UPDATETIME=sysdate() "
                        "where MGWID=%d "
                        "and PNAME='%s' "
                        "and ifnull(ARGS, 'NULL')='%s'",
                sqlMgwId,
                sqlPName,
                sqlArgs
        );


        updateCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query(%s)",
                           sqlBuf);

            return -1;
        }
    }

    WRITE_INFO(CATEGORY_DB,"clean_pinfo updateCnt(%d)", updateCnt);

    return loop;
}

void fmaster_ExitServer(void)
{
    fipc_FQClose(DBLOG_QUEUE);

    fcom_MallocFree((void**)&g_pstNotiMaster);

    sleep(2);

    exit(0);
}