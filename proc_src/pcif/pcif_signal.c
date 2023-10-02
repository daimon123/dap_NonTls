//
// Created by KimByoungGook on 2020-10-13.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <string.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>



#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "json/dap_json.h"

#include "pcif.h"

void fpcif_SigHandler(void)
{
    signal( SIGHUP, fpcif_HandleSignal);    /* 1 : hangup */
    signal( SIGINT, fpcif_HandleSignal);    /* 2 : interrupt (rubout) */
    signal( SIGQUIT, fpcif_HandleSignal);   /* 3 : quit (ASCII FS) */
    signal( SIGILL, fpcif_HandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP, fpcif_HandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT, fpcif_HandleSignal);    /* 6 : IOT instruction */
    signal( SIGABRT, fpcif_HandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
    signal( SIGFPE, fpcif_HandleSignal);    /* 8 : floating point exception */
    signal( SIGKILL , fpcif_HandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS , fpcif_HandleSignal);   /* 10: bus error */
    signal( SIGSEGV, fpcif_HandleSignal);   /* 11: segmentation violation */
    signal( SIGSYS , fpcif_HandleSignal);   /* 12: bad argument to system call */
    signal( SIGPIPE, SIG_IGN );              /* 13: write on a pipe with no one to read it */

    signal( SIGALRM, fpcif_HandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fpcif_HandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fpcif_HandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fpcif_HandleSignal);   /* 17: user defined signal 2 */
    signal( SIGCLD,SIG_IGN );              /* 18: child status change */
    signal( SIGCHLD,SIG_IGN );             /* 18: child status change alias (POSIX) */
    signal( SIGPWR , fpcif_HandleSignal);   /* 19: power-fail restart */
    signal( SIGWINCH, SIG_IGN );            /* 20: window size change */
    signal( SIGURG  , fpcif_HandleSignal);  /* 21: urgent socket condition */
    signal( SIGPOLL , fpcif_HandleSignal);  /* 22: pollable event occured */
    signal( SIGIO   , fpcif_HandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP , fpcif_HandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal( SIGTSTP , fpcif_HandleSignal);  /* 24: user stop requested from tty */
    signal( SIGCONT , fpcif_HandleSignal);  /* 25: stopped process has been continued */
    signal( SIGTTIN , fpcif_HandleSignal);  /* 26: background tty read attempted */
    signal( SIGTTOU , fpcif_HandleSignal);  /* 27: background tty write attempted */
    signal( SIGVTALRM , fpcif_HandleSignal);/* 28: virtual timer expired */
    signal( SIGPROF , fpcif_HandleSignal);  /* 29: profiling timer expired */
    signal( SIGXCPU , fpcif_HandleSignal);  /* 30: exceeded cpu limit */
    signal( SIGXFSZ , fpcif_HandleSignal);  /* 31: exceeded file size limit */

}

void fpcif_HandleSignal(int sid)
{
    if(sid == SIGALRM)
    {
        return ;
    }

    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit signal(%d)pid(%d)ppid(%d)", sid,getpid(),getppid());
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    if(g_stProcPcifInfo.parentPid == getpid())
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Try kill child process, ppid(%d) ", g_stProcPcifInfo.parentPid);
        fpcif_ParentExit(sid); // 부모
    }
    else // 자식
    {
        // SEGV 발생하므로 자원정리를 커널에 맡기자
//        if ( sid == SIGABRT || sid == SIGSEGV) //비정상적인 상황인경우 강제 종료한다.
//            exit(0);
//        fpcif_ChildExit(sid);
        exit(0);
    }

    exit(0);

}

/**SIGCHLD HANDLER**/
void fpcif_SigchldHandler(int s)
{
    int 	pid = 0, cpId = 0, rxt = 0, loop = 0;

    WRITE_DEBUG(CATEGORY_DEBUG,"Signal Catch : (%d)",s);

    while((pid = waitpid(-1,NULL,WNOHANG)) > 0)
    {
        for(loop=0; loop < g_stProcPcifInfo.cfgForkCnt; loop++)
        {
            if ( g_ChildExitFlag != 0x01)
            {
                if( pid == g_stProcPcifInfo.ptrChildPid[loop] )
                {
                    if( pid != g_stProcPcifInfo.parentPid)
                    {
                        g_nPcifForkIdx = loop;
                        if((cpId = fork()) < 0)
                        {
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fork " );
                            continue;
                        }
                        else if(cpId > 0)
                        {
                            g_stProcPcifInfo.ptrChildPid[loop] = cpId;
                            WRITE_CRITICAL(CATEGORY_DEBUG,"Try invoke, pid(%d->%d) ", pid,cpId);
                        }
                        else if(cpId == 0) // child process
                        {
                            WRITE_DEBUG(CATEGORY_DEBUG,"Fork DB Handle : %d ",g_stMyCon );
                            fdb_SqlClose(g_stMyCon);

                            rxt = fdb_ConnectDB();
                            if( rxt < 0 )
                            {
                                WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connection " );
                                exit(0);
                            }

                            fpcif_ForkWork();
                            WRITE_INFO(CATEGORY_DEBUG,"Exit " );
                            exit(0);
                        }
                    }
                }
            }
            sleep(0);
        } // for
        sleep(0);
    }
}

void fpcif_ParentExit(int sid)
{
    int local_nLoop = 0;
    int local_status = 0;
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** SIGCHLD wait() function으로 신규 자식생성 중지 FLAG **/
    if ( g_ChildExitFlag == 0x00 )
    {
        g_ChildExitFlag = 0x01;
    }

    /* Child Signal Send */
    for(local_nLoop = 0; local_nLoop < g_stProcPcifInfo.cfgForkCnt; local_nLoop++)
    {
        WRITE_DEBUG(CATEGORY_DEBUG," pid [%d] ", g_stProcPcifInfo.ptrChildPid[local_nLoop]);
        kill(g_stProcPcifInfo.ptrChildPid[local_nLoop], SIGTERM);
    }

    /** 모든 자식(Child Process) 죽을떄 까지 대기 **/
    wait(&local_status);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PCIF);

    if(gFQNum > START_MANAGER_FQ)
    {
        fipc_FQClose(gFQNum);
    }

    fpcif_DelAllMnq();

    for(local_nLoop=0; local_nLoop <= g_stProcPcifInfo.cfgQCount; local_nLoop++)
    {
        fipc_FQClose(local_nLoop);
    }
    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);
    fipc_FQClose(REPORT_QUEUE);
    fipc_FQClose(TAIL_QUEUE);
    fipc_FQClose(FW_POLICY);
    fipc_FQClose(FW_SERVICE);
    fcom_MallocFree((void**)&(pRuleInfo->dp_print_port));

    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        for(local_nLoop=0; local_nLoop < MAX_RULE_COUNT; local_nLoop++)
        {
            fcom_BufferFree(pDetectInfo->rd_value[local_nLoop]);
        }
    }
    else
    {
        fcom_MallocFree((void**)&(pDetectSelInfo->dp_process_white));
        fcom_MallocFree((void**)&(pDetectSelInfo->dp_process_black));
        fcom_MallocFree((void**)&(pDetectSelInfo->ws_ipaddr));
        fcom_MallocFree((void**)&(pDetectSelInfo->ws_url));
    }

    fcom_MallocFree((void**)&g_pstNotiMaster);
    fcom_MallocFree((void**)&g_pstNotiEvent);
    fcom_MallocFree((void**)&pUserInfo);
    fcom_MallocFree((void**)&pGroupLink);
    fcom_MallocFree((void**)&pBaseInfo);

    // sleep을 줘서 sigchild에서 g_stProcPcifInfo.ptrChildPid 변수 참조로인한 SEGV를 막아준다.
    sleep(2);

    fcom_MallocFree((void**)&g_stProcPcifInfo.ptrChildPid);

    if ( g_stProcPcifInfo.listenSock > 0)
    {
        close(g_stProcPcifInfo.listenSock);
        g_stProcPcifInfo.listenSock = 0;
    }

    fdb_SqlClose(g_stMyCon);

    /** Cleanup Master PID File **/
    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);
}

void fpcif_ChildExit(int sid)
{
    int idx = 0;

    if ( g_ChildExitFlag == 0x00)
    {
        g_ChildExitFlag = 0x01;
    }

    /* 잠시대기 */
    sleep(2);

    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        for(idx = 0; idx < pDetectInfo->tot_cnt; idx++)
        {
            fcom_BufferFree(pDetectInfo->rd_value[idx]);
        }
    }
    else
    {
        fcom_MallocFree((void**)&(pDetectSelInfo->dp_process_white));
        fcom_MallocFree((void**)&(pDetectSelInfo->dp_process_black));
        fcom_MallocFree((void**)&(pDetectSelInfo->ws_ipaddr));
        fcom_MallocFree((void**)&(pDetectSelInfo->ws_url));
    }

    fipc_SQClear();
    if ( g_stProcPcifInfo.Fp != NULL )
    {
        fclose(g_stProcPcifInfo.Fp);
        g_stProcPcifInfo.Fp = NULL;
    }

    fsock_CleanupOpenssl();
    sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
    CRYPTO_cleanup_all_ex_data();

    fdb_SqlClose(g_stMyCon);
    WRITE_INFO(CATEGORY_DEBUG,"Function Close DB Handle : %d ",g_stMyCon );

    fcom_MallocFree((void**)&g_pstNotiEvent);
    fcom_MallocFree((void**)&g_pstNotiMaster);
    fcom_MallocFree((void**)&g_stThread_arg);

    fcom_MallocFree((void**)&pUserInfo);
    fcom_MallocFree((void**)&pGroupLink);
    fcom_MallocFree((void**)&pBaseInfo);
    fcom_MallocFree((void**)&g_stProcPcifInfo.ptrChildPid);

}

void fpcif_KillZombieCp(void)
{
    char buf[1024];

    memset(buf, 0x00, sizeof(buf));

    sprintf(buf, "%s/bin/kill_proc.sh -15 %s", getenv("DAP_HOME"), g_stServerInfo.stDapComnInfo.szProcName);
    WRITE_CRITICAL(CATEGORY_DEBUG, "Try kill garbage processing(%s) ", buf);
    system(buf);

    return;
}
