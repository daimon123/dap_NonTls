//
// Created by KimByoungGook on 2020-06-12.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

#include "ipc/dap_Queue.h"
#include "com/dap_com.h"

/***************************/
/****   Receive Part    ****/
/***************************/
int fipc_FQGetInit(int param_Idx, char *param_Qname)
{
    strcpy(g_QName[param_Idx], param_Qname);

    g_Qsockfd[param_Idx] = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (g_Qsockfd[param_Idx] < 0)
        return -1;

    unlink(g_QName[param_Idx]);

    bzero(&g_stServerAddr[param_Idx], sizeof(g_stServerAddr[param_Idx]));
    g_stServerAddr[param_Idx].sun_family = AF_UNIX;
    strcpy(g_stServerAddr[param_Idx].sun_path, g_QName[param_Idx]);    

    if (bind(g_Qsockfd[param_Idx], (struct sockaddr *)&g_stServerAddr[param_Idx], sizeof(g_stServerAddr[param_Idx])) < 0)
    {
        perror("bind error : ");
        return -1;
    }

    g_nClilen[param_Idx] = sizeof(g_stClientAddr[param_Idx]);

    return 0;
}

int fipc_FQGetData(int param_Idx, char* param_buff, int param_nBufflen)
{
    int nRet;
    struct pollfd fds;

    fds.fd = g_Qsockfd[param_Idx];
    fds.events = POLLIN;

    nRet = poll(&fds, 1, 2000);
    if (nRet == 0)
        return 0;
    else if (nRet < 0)
        return nRet;

    nRet = recvfrom(g_Qsockfd[param_Idx], (void *)param_buff, param_nBufflen,
            0, (struct sockaddr *)&g_stClientAddr[param_Idx], (socklen_t *)&g_nClilen[param_Idx]);

    return nRet;
}

/***************************/
/****   Receive Part    ****/
/***************************/



/***************************/
/****     Send Part     ****/
/***************************/
int fipc_FQPutInit(int param_Idx, char* param_Qname)
{
    strcpy(g_QName[param_Idx], param_Qname);

    g_Qsockfd[param_Idx] = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (g_Qsockfd[param_Idx] < 0)
    {
        WRITE_CRITICAL(CATEGORY_IPC,"FQPUT (%s) Init Failed", param_Qname);
        return -1;
    }


    bzero(&g_stServerAddr[param_Idx], sizeof(g_stServerAddr[param_Idx]));
    g_stServerAddr[param_Idx].sun_family = AF_UNIX;

    strcpy(g_stServerAddr[param_Idx].sun_path, g_QName[param_Idx]);
    g_nClilen[param_Idx] = sizeof(g_stClientAddr[param_Idx]);

    return 0;
}

void fipc_FQPutReSet(int param_Idx, char* param_Qname)
{
    strcpy(g_QName[param_Idx], param_Qname);
    strcpy(g_stServerAddr[param_Idx].sun_path, g_QName[param_Idx]);
}

int fipc_FQPutData(int param_Idx, void* param_sbuff, int nBufflen)
{
    int rxt;

    rxt = sendto(g_Qsockfd[param_Idx],
            (void *)param_sbuff,
            nBufflen,
            0,
            (struct sockaddr *)&g_stServerAddr[param_Idx],
            g_nClilen[param_Idx]);

    return rxt;
}

void fipc_FQClose(int param_nIdx)
{
    if (g_Qsockfd[param_nIdx] > 0)
    {
        close(g_Qsockfd[param_nIdx]);
        g_Qsockfd[param_nIdx] = 0;
    }

}