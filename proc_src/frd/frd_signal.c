//
// Created by KimByoungGook on 2021-05-07.
//


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <string.h>

#include "com/dap_com.h"
#include "frd.h"
#include "dap_version.h"

extern char g_ExecFileName[256];
void frd_SigHandler(void)
{
    signal(SIGHUP, frd_HandleSignal);    /* 1 : hangup */
    signal(SIGINT, frd_HandleSignal);    /* 2 : interrupt (rubout) */
    signal(SIGQUIT, frd_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal(SIGILL, frd_HandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal(SIGTRAP, frd_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal(SIGIOT, frd_HandleSignal);    /* 6 : IOT instruction */
    signal(SIGABRT, frd_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal(SIGFPE, frd_HandleSignal);    /* 8 : floating point exception */
    signal(SIGKILL, frd_HandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal(SIGBUS, frd_HandleSignal);   /* 10: bus error */
    signal(SIGSEGV, frd_HandleSignal);   /* 11: segmentation violation */
    signal(SIGSYS, frd_HandleSignal);   /* 12: bad argument to system call */
    signal(SIGPIPE, frd_HandleSignal);   /* 13: write on a pipe with no one to read it */
    signal(SIGALRM, frd_HandleSignal);   /* 14: alarm clock */
    signal(SIGTERM, frd_HandleSignal);   /* 15: software termination signal from kill */
    signal(SIGUSR1, frd_HandleSignal);   /* 16: user defined signal 1 */
    signal(SIGUSR2, frd_HandleSignal);   /* 17: user defined signal 2 */
    signal(SIGCLD, SIG_IGN);              /* 18: child status change */
    signal(SIGCHLD, SIG_IGN);             /* 18: child status change alias (POSIX) */
    signal(SIGPWR, frd_HandleSignal);   /* 19: power-fail restart */
//    sigignore(SIGWINCH);            /* 20: window size change */
    signal(SIGWINCH,SIG_IGN);            /* 20: window size change */
    signal(SIGURG, frd_HandleSignal);  /* 21: urgent socket condition */
    signal(SIGPOLL, frd_HandleSignal);  /* 22: pollable event occured */
    signal(SIGIO, frd_HandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal(SIGSTOP, frd_HandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal(SIGTSTP, frd_HandleSignal);  /* 24: user stop requested from tty */
    signal(SIGCONT, frd_HandleSignal);  /* 25: stopped process has been continued */
    signal(SIGTTIN, frd_HandleSignal);  /* 26: background tty read attempted */
    signal(SIGTTOU, frd_HandleSignal);  /* 27: background tty write attempted */
    signal(SIGVTALRM, frd_HandleSignal);/* 28: virtual timer expired */
    signal(SIGPROF, frd_HandleSignal);  /* 29: profiling timer expired */
    signal(SIGXCPU, frd_HandleSignal);  /* 30: exceeded cpu limit */
    signal(SIGXFSZ, frd_HandleSignal);  /* 31: exceeded file size limit */
}

/**SIGCHLD HANDLER**/
void frd_SigchldHandler(int param_Sig)
{
    int 	pid = 0, cpId = 0, loop = 0;

    WRITE_DEBUG(CATEGORY_DEBUG,"Signal Catch : (%d)", param_Sig);

    while( (pid = waitpid(-1, NULL, WNOHANG)) > 0 )
    {
        for(loop = 0; loop < g_stProcFrdInfo.nForkCnt +2; loop++) // 정책/서비스 Fork 고정으로 하나 뜨기때문에 +1 해준다.
        {
            if( pid == g_stProcFrdInfo.Pid[loop] && g_ChildExitFlag != 1)
            {
                if( pid != g_stProcFrdInfo.parentPid)
                {
                    g_nForkIdx = loop;
                    if( (cpId = fork() ) < 0)
                    {
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fork " );
                        continue;
                    }
                    else if(cpId > 0)
                    {
                        g_stProcFrdInfo.Pid[loop] = cpId;
                        WRITE_CRITICAL(CATEGORY_DEBUG,"Try invoke, pid(%d->%d) ", pid, cpId);
                        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                             "Try Child Invoke Process pid(%d->%d)",  pid, cpId);
                        continue;
                    }
                    else if(cpId == 0) // child process
                    {
                        frd_ForkWork();
                        exit(3);
                        break;
                    }
                }
            }
        } // for
        fcom_SleepWait(4);
    }
}

void frd_HandleSignal(int sid)
{
    if( g_stProcFrdInfo.parentPid == getpid() ) // Parent
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Try kill child process, ppid(%d) ", g_stProcFrdInfo.parentPid);

        frd_ParentExit(sid);
    }
    else //Child
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"Child Recv Signal [%d] ", sid);

        if ( sid == SIGSEGV ){
            if ( g_stProcFrdInfo.nDumpFlag == 0x01 ) {
                // Create Dump Flag file
                char szFilePath[256] = {0x00,};
                FILE* local_fp = NULL;

                snprintf( szFilePath, sizeof(szFilePath),
                          "%s%s", g_stProcFrdInfo.szDtFilePath, "DumpFlag" );

                local_fp = fopen(szFilePath,"w");
                if ( local_fp != NULL) {
                    fclose(local_fp);
                }

                //TODO . dump 동작 중 SEGV 발생시 현재 dump 작업중이던 파일을 삭제한다.
                // 추후 SEGV자체 원인 분석 필요
                // Delete Dump Flag file
                if ( fcom_fileCheckStatus(g_ExecFileName) == 0){
                    unlink(g_ExecFileName);
                }
            }
        } else {
            if ( g_stProcFrdInfo.nDumpFlag == 0x01 ) {
                char szFilePath[256] = {0x00,};

                snprintf( szFilePath, sizeof(szFilePath),
                          "%s%s", g_stProcFrdInfo.szDtFilePath, "DumpFlag" );
                // Delete Dump Flag file
                if ( fcom_fileCheckStatus(szFilePath) == 0){
                    unlink(szFilePath);
                }
            }
        }

        frd_ChildExit();
    }

    fcom_MallocFree((void**) &g_stProcFrdInfo.Pid);

    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program, received signal(%d) pid(%d)",
                   sid, getpid());



    exit(0);

}
