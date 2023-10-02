//
// Created by KimByoungGook on 2021-03-11.
//


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "com/dap_com.h"

#include "frd.h"
#include "dap_version.h"


int frd_MainTask()
{
    pid_t local_nPid = 0;
    int local_nLoop = 0;
    struct sigaction saDeadChild;
    char    local_szPidPath[127 +1]     = {0x00,};
    char*   local_ptrProcessName        = NULL;

    WRITE_INFO(CATEGORY_INFO,"Start Program ... ");
//    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    if ( g_stProcFrdInfo.nDumpFlag != 0x01 ) {
        /** Get Pid File Process Name **/
        fFile_SetPidFileSt();
        local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_FRD);

        /** Check Master Pid File **/
        if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
            return (-1);
        }

        /** Make Master Pid File **/
        fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);
    }


    saDeadChild.sa_handler = frd_SigchldHandler; // reap all dead processes
    sigemptyset(&saDeadChild.sa_mask);
    saDeadChild.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &saDeadChild, NULL)==-1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Error while sigaction.");
        exit(1);
    }

    if ( g_stProcFrdInfo.nDumpFlag != 0x01 )
    {
        fcom_malloc((void**) &g_stProcFrdInfo.Pid, sizeof(pid_t) * (g_stProcFrdInfo.nForkCnt +2 +1) ); // 정책,서비스 Fork 고정으로 뜨기때문에+2 해준다.

        g_stProcFrdInfo.parentPid = getpid();

        /** 정책 데이터 Fork() 프로세스 **/
        g_nForkIdx      = FORK_POLICY;
        if( (local_nPid = fork()) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
            return (-1);
        }

        else if(local_nPid > 0)
        {
            g_stProcFrdInfo.Pid[FORK_POLICY]  = local_nPid;
        }

        // child process
        else if(local_nPid == 0)
        {
            frd_ForkWork();
            exit(0);
        }

        /** 정책 데이터 Fork() 프로세스 **/
        g_nForkIdx      = FORK_SERVICE;
        if( (local_nPid = fork()) < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
            return (-1);
        }

        else if(local_nPid > 0)
        {
            g_stProcFrdInfo.Pid[FORK_SERVICE]  = local_nPid;
        }

        // child process
        else if(local_nPid == 0)
        {
            frd_ForkWork();
            exit(0);
        }

        /** Fork Process구별용 Index **/
        g_nForkIdx = FORK_DETECT;

        /** 검출 데이터 Fork() 프로세스,  개수 = Unix 소켓 Count만큼. **/
        for( local_nLoop = g_nForkIdx; local_nLoop < g_stProcFrdInfo.nForkCnt +2; local_nLoop++) // 정책,서비스 Fork 고정으로 뜨기때문에+2 해준다.
        {
            if( (local_nPid = fork()) < 0 )
            {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fork ");
                continue;
            }

            else if(local_nPid > 0)
            {
                g_stProcFrdInfo.Pid[local_nLoop] = local_nPid; //1 , 2 , 3

                /** Fork Process구별용 Index **/
                g_nForkIdx++;
            }

            // child process
            else if(local_nPid == 0)
            {
                frd_ForkWork();
                exit(0);
            }
        }

        while(1)
        {
            /** Main Nothing **/
            sleep(1);
        }
    }
    else
    {

        WRITE_DEBUG(CATEGORY_DB,"frd Dump Exec Start ..");
        frd_DumpExec();
        WRITE_DEBUG(CATEGORY_DB,"frd Dump Exec End ..");

        snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    }

    frd_HandleSignal(15);

    return RET_SUCC;

}

