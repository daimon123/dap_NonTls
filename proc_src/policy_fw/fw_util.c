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


//void frd_SigHandler(void)
void fw_SigHandler(void)
{
    signal(SIGHUP, fw_HandleSignal);    /* 1 : hangup */
    signal(SIGINT, fw_HandleSignal);    /* 2 : interrupt (rubout) */
    signal(SIGQUIT, fw_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL, fw_HandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP, fw_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT, fw_HandleSignal);    /* 6 : IOT instruction */
    signal(SIGABRT, fw_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGFPE, fw_HandleSignal);    /* 8 : floating point exception */
    signal(SIGKILL, fw_HandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal(SIGBUS, fw_HandleSignal);   /* 10: bus error */
    signal(SIGSEGV, fw_HandleSignal);   /* 11: segmentation violation */
    signal(SIGSYS, fw_HandleSignal);   /* 12: bad argument to system call */
    signal(SIGPIPE, fw_HandleSignal);   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM, fw_HandleSignal);   /* 14: alarm clock */
    signal(SIGTERM, fw_HandleSignal);   /* 15: software termination signal from kill */
    signal(SIGUSR1, fw_HandleSignal);   /* 16: user defined signal 1 */
    signal(SIGUSR2, fw_HandleSignal);   /* 17: user defined signal 2 */
    signal(SIGCLD, SIG_IGN);              /* 18: child status change */
    signal(SIGCHLD, SIG_IGN);             /* 18: child status change alias (POSIX) */
    signal(SIGPWR, fw_HandleSignal);   /* 19: power-fail restart */
//    sigignore(SIGWINCH);            /* 20: window size change */
    signal(SIGWINCH,SIG_IGN);            /* 20: window size change */
    signal(SIGURG, fw_HandleSignal);  /* 21: urgent socket condition */
    signal(SIGPOLL, fw_HandleSignal);  /* 22: pollable event occured */
    signal(SIGIO, fw_HandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal(SIGSTOP, fw_HandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal(SIGTSTP, fw_HandleSignal);  /* 24: user stop requested from tty */
    signal(SIGCONT, fw_HandleSignal);  /* 25: stopped process has been continued */
    signal(SIGTTIN, fw_HandleSignal);  /* 26: background tty read attempted */
    signal(SIGTTOU, fw_HandleSignal);  /* 27: background tty write attempted */
    signal(SIGVTALRM, fw_HandleSignal);/* 28: virtual timer expired */
    signal(SIGPROF, fw_HandleSignal);  /* 29: profiling timer expired */
    signal(SIGXCPU, fw_HandleSignal);  /* 30: exceeded cpu limit */
    signal(SIGXFSZ, fw_HandleSignal);  /* 31: exceeded file size limit */
}
/**SIGCHLD HANDLER**/
void fw_SigchldHandler(int param_Sig)
{
    int 	pid = 0, cpId = 0,  nRetIdx = 0;
    int     local_nLoop = 0;

    WRITE_DEBUG(CATEGORY_DEBUG,"Signal Catch : (%d)", param_Sig);

    while( (pid = waitpid(-1,NULL,WNOHANG)) > 0 )
    {
        for(local_nLoop = 0; local_nLoop < FORK_COUNT; local_nLoop++) // 정책 Fork 고정으로 하나 뜨기때문에 +1 해준다.
        {
            if( pid == g_stProcFwInfo.Pid[local_nLoop] && g_ChildExitFlag != 1)
            {
                if( pid != g_stProcFwInfo.parentPid)
                {
                    nRetIdx = g_stProcFwInfo.nForkIdx[local_nLoop];

                    if( (cpId = fork() ) < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fork " );
                        continue;
                    }
                    else if(cpId > 0)
                    {
                        g_stProcFwInfo.Pid[local_nLoop] = cpId;
                        g_stProcFwInfo.nForkIdx[local_nLoop]  = nRetIdx;
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Try invoke, pid(%d->%d) ", pid, cpId);
                        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                             "Try Child Invoke Process  pid(%d->%d)",  pid, cpId);
                        continue;
                    }
                    else if(cpId == 0) // child process
                    {
                        fw_ForkWork();
                        exit(3);
                        break;
                    }
                }
            }
        }

    }
}

void fw_HandleSignal(int sid)
{

    if( g_stProcFwInfo.parentPid == getpid() )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Try kill child process, ppid(%d) ", g_stProcFwInfo.parentPid);

        kill(g_stProcFwInfo.Pid[FORK_POLICY], 15);
        kill(g_stProcFwInfo.Pid[FORK_SERVICE], 15);

        fw_ParentExit();
    }
    else
    {
        fw_ChildExit();
    }

    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program, received signal(%d)pid(%d)", sid,getpid());
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

//    fcom_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath, szProcessName);

    sleep(1);
    exit(sid);
}


/*
int fw_GetForkIdx(pid_t param_FindPid)
{
    int local_nLoop = 0;

    for(local_nLoop = 0; local_nLoop < g_stProcFwInfo.cfgQCount+1; local_nLoop++)
    {
        if(g_stProcFwInfo.Pid[local_nLoop] == param_FindPid)
        {
            g_nForkIdx = g_stProcFwInfo.nForkIdx;
            return local_nLoop;
        }
    }
    return (-1);
}
*/


int fw_ParentExit(void)
{
    int     local_status = 0;
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    if (g_ChildExitFlag == 0 )
    {
        g_ChildExitFlag = 1;
    }

    /** 모든 자식(Child Process) 죽을떄 까지 대기 **/
    wait(&local_status);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_POLICY_FW);

    fipc_FQClose(FW_POLICY);
    fipc_FQClose(FW_SERVICE);
    fipc_FQClose(DBLOG_QUEUE);

    /** Cleanup Master PID File **/
    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    return 0;

}

int fw_ChildExit(void)
{

    if ( g_stProcFwInfo.Fp != NULL)
        fclose(g_stProcFwInfo.Fp);

    sleep(1);
    exit(0);

}