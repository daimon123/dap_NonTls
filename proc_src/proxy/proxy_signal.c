//
// Created by KimByoungGook on 2021-04-09.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "com/dap_com.h"
#include "sock/dap_sock.h"
#include "ipc/dap_Queue.h"

#include "proxy.h"
#include "dap_version.h"

/**SIGCHLD HANDLER**/
void fproxy_SigchldHandler(int signo)
{
    int pid, cpId;
    int nLoop = 0;

    while((pid = waitpid(-1,NULL,WNOHANG)) > 0)
    {
        if(g_signalFlag != TRUE)
        {
            //WRITE_INFO(CATEGORY_DEBUG,"-> pid[%d] parent[%d]", pid,parent);
            g_numChildren--;

            // 20190507 added, 죽으면 다시 fork
            if (pid != g_ParentPid)
            {
                if ((cpId = fork()) < 0)
                {
                    continue;
                }
                else if (cpId > 0) // parent process 부모를 죽일필요있을 때만
                {
                    g_numChildren++;
                    for(nLoop = 0; nLoop < g_stProcProxyInfo.cfgForkCnt; nLoop++)
                    {
                        if(g_ChildPid[nLoop] == pid)
                        {
                            g_ChildPid[nLoop] = cpId;
                        }
                    }
                    continue;
                } else if (cpId == 0) // child process
                {
                    WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                         "Try Child Invoke Process  pid(%d->%d)",  pid, cpId);
                    fproxy_ForkWork();
                    WRITE_CRITICAL(CATEGORY_DEBUG, "Exit ");
                    exit(0); //child이므로 걍 종료
                    break;
                }
            }

        }


    }
}


void fproxy_SigHandler(int s)
{
    int idx = 0;
    int local_status=0;
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", s);

    if(getppid() == 1) //부모
    {
        snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

        g_signalFlag = TRUE;

        for(idx = 0; idx < g_stProcProxyInfo.cfgForkCnt; idx++)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Child Kill %d ",g_ChildPid[idx]);
            kill(g_ChildPid[idx],SIGTERM);
        }

        /** 모든 자식(Child Process) 죽을떄 까지 대기 **/
        wait(&local_status);

        fipc_FQClose(DBLOG_QUEUE);

        SSL_CTX_free(g_stProcProxyInfo.ctx);
        fsock_CleanupOpenssl();
        sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
        ERR_remove_thread_state(NULL);
        CRYPTO_cleanup_all_ex_data();

        if(g_ChildPid != NULL)
        {
            free(g_ChildPid);
        }

        /** Get Pid File Process Name **/
        fFile_SetPidFileSt();
        local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_PROXY);

        /** Cleanup Master PID File **/
        fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

        sleep(2);

        exit(0);
    }
    else //자식
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"Get Signal :%d,%d",s,g_stProcProxyInfo.cfgThreadCnt);

        fsock_CleanupOpenssl();
        sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
        CRYPTO_cleanup_all_ex_data();

        g_ForkExit = FAILURE;

        sleep(2);

    }

    WRITE_DEBUG(CATEGORY_DEBUG,"SigFunc exit");

    exit(0);

}