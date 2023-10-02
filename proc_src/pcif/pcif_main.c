
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
#include "dap_version.h"

#include "pcif.h"


int main(int argc, char** argv)
{

    pConfigInfo		= (dbConfig*)&configInfo;
    pRuleInfo		= (dbRule*)&ruleInfo;
    pSchdInfo		= (dbSchd*)&schdInfo;
    pUgdInfo		= (dbUgd*)&ugdInfo;
    pGwInfo         = (dbGw*)&gwInfo;
    pManagerInfo	= (dbManager*)&managerInfo;

    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fpcif_PcifInit();

    if( g_stProcPcifInfo.cfgLoadDetectMem == 1 )
    {
        memset(&detectInfo,0x00, sizeof(detectInfo));
        pDetectInfo = (dbDetect*)&detectInfo;
    }
    else
    {
        pDetectSelInfo = (dbDetectSel*)&detectSelInfo;
    }

    fpcif_ForkTask();

    return RET_SUCC;

}