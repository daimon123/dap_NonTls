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

void frd_FQEventToPcif(_DAP_EventParam* p_EP)
{
    int		rxt = 0;
    int		fqIdx = 0;
    char	mnqPath[128 +1] = {0x00,};
    char	fileName[50 +1] = {0x00,};
    char	strFQNum[10 +1] = {0x00,};
    char	mnqCpPath[256 +1] = {0x00,};
    DIR*	pMngDir = NULL;
    struct	dirent *pDirent = NULL;

    memset(mnqPath, 0x00, sizeof(mnqPath));
    sprintf(mnqPath, "%s/MNGQ", g_stServerInfo.stDapQueueInfo.szDAPQueueHome);
    if(!(pMngDir = opendir(mnqPath)))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in manager queue dir ");
    }
    else
    {
        while((pDirent = readdir(pMngDir)) != NULL)
        {
            if(strlen(pDirent->d_name) < 10)
                continue;
            if(fcom_TokenCnt(pDirent->d_name, "_") == 0)
                continue;

            memset(fileName, 0x00, sizeof(fileName));
            memset(strFQNum, 0x00, sizeof(strFQNum));

            snprintf(fileName, sizeof(fileName), "%s", pDirent->d_name);
            fcom_GetFQNum(fileName, "_", strFQNum);
            WRITE_INFO(CATEGORY_IPC, "strFQNum(%s) ", strFQNum);
            if(!isdigit(strFQNum[0]))
            {
                continue;
            }
            else
            {
                fqIdx = atoi(strFQNum);
            }
            memset(mnqCpPath, 0x00, sizeof(mnqCpPath));
            snprintf(mnqCpPath, sizeof(mnqCpPath), "%s/%s", mnqPath,fileName);
            WRITE_INFO(CATEGORY_IPC, "mnqCpPath(%s) ", mnqCpPath);
            if (fipc_FQPutInit(fqIdx, mnqCpPath) < 0)
            {
                WRITE_CRITICAL(CATEGORY_IPC, "Fail in fqueue init, path(%s) ",  mnqPath);
                rxt = -1;
            }
            else
            {
                rxt = fipc_FQPutData(fqIdx, (_DAP_EventParam *)p_EP, sizeof(_DAP_EventParam));
            }

            if (rxt < 0)
            {
                WRITE_CRITICAL(CATEGORY_IPC,"Fail in send event data, fq(%d) ", fqIdx);
            }
            fipc_FQClose(fqIdx);
        }
        closedir(pMngDir);
    }
}

void frd_FQEventToAlarm(_DAP_QUEUE_BUF *p_AQ, _DAP_EventParam * p_EP)
{
    int rxt = 0;

    p_AQ->packtype = EVENT_ALARM;
    memcpy((void *)p_AQ->buf, p_EP, sizeof(_DAP_EventParam));
    rxt = fipc_FQPutData(ALARM_QUEUE,(_DAP_QUEUE_BUF *) p_AQ, sizeof(_DAP_QUEUE_BUF));
    if (rxt < 0)
    {
        WRITE_CRITICAL(CATEGORY_IPC, "Fail in send event alarm ");
    }
}
