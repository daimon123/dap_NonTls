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
#include "frd.h"
#include "dap_version.h"


int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DAP_VERSION_MACRO
        printf("\n");
        printf("Usage [ dap_frd frd ] OR [ dap_frd dump ] \n");
        printf("dump Is FW_DT File Execute ");
        printf("\n");
        exit(0);
    }

    fcom_ArgParse(argv);

    frd_Init();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    fdb_LsystemInit();

    frd_SigHandler();

    frd_MainTask();

    return RET_SUCC;


}

