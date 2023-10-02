//
// Created by KimByoungGook on 2020-06-26.
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
#include "dbif.h"
#include "dap_version.h"


static int fstMainTask();

int main(int argc, char** argv)
{
    int nRet = 0;

    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fdbif_DbifInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    nRet = fdb_ConnectDB();

    if(nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in db connection ");
        exit(0);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Succeed in db connection ");
    }

    fdb_LsystemInit();

    fdbif_SigHandler();

    fstMainTask();

    return RET_SUCC;


}

static int fstMainTask()
{
    int nRet = 0;
    int thrval = 0;
    pthread_t pThreadrecv_t;

    WRITE_INFO(CATEGORY_INFO,"Start Program ... ");
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    fipc_SQInit(g_stProcDbifInfo.nRsltQsize, g_stProcDbifInfo.szSaveFilePath);

    nRet = fipc_SQLoadQ();

    if(nRet == 0)
    {
        //SQShowElement();
        WRITE_INFO(CATEGORY_INFO, "Load rslt queue, rxt(%d) ", nRet);
        nRet = fipc_SQRemove();
    }
    else if(nRet > 0)
    {
        nRet = fipc_SQRemove();
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in Delete Queue ");
    }
    if(fcom_ThreadCreate((void *)&pThreadrecv_t, fdbif_DbifThread, (void *)&thrval, 4 * 1024 * 1024) != 0)

    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
        return -1;
    }

    fdbif_FromQToDBMS();
    fdbif_HandleSignal(15);

    return RET_SUCC;

}






