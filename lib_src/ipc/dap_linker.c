//
// Created by KimByoungGook on 2020-06-19.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "db/dap_checkdb.h"

#include "ipc/dap_Queue.h"
#include "com/dap_def.h"
#include "com/dap_com.h"


void fipc_KeepSession(int nParam_CfgSession)
{
    int nRet;
    g_stLinkerInfo.cur_time = time(NULL);

    if ((g_stLinkerInfo.cur_time - g_stLinkerInfo.last_job_time) > nParam_CfgSession)
    {
        g_stLinkerInfo.last_job_time = time(NULL);

        nRet = fdb_PutSessionQuery();

        if (nRet !=  0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in session query " );
        }
    }
}

void fipc_PrmonLinker(char *pname, int frc, long* param_LastSendTime)
{
    long ptime;
    _DAP_PING stPing;

    memset(&stPing,0x00, sizeof(stPing));

    ptime = time(0) - 60;

    if(frc)
    {
        g_stLinkerInfo.last_send_time = time(0);
        sprintf(stPing.msgtype, "%d", PRMON_LINKER);
        strcpy(stPing.procname, pname);

        if (fipc_FQPutData(PRMON_QUEUE, (char *)&stPing, sizeof(stPing)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in link");
        }
        else
        {
            WRITE_INFO(CATEGORY_INFO,"Prmon Link ");
        }
    }
    else if(ptime > *param_LastSendTime)
    {
        *param_LastSendTime = time(0);
        sprintf(stPing.msgtype, "%d", PRMON_LINKER);
        strcpy(stPing.procname, pname);

        if (fipc_FQPutData(PRMON_QUEUE, (char *)&stPing, sizeof(stPing)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in link");
        }
    }
}

