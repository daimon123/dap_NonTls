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
#include "com/dap_def.h"
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


/*void* frd_DbifThread(void* thrid)
{
    int			rxt = 0;
    int			retryCnt = 0;
    int         thrIdx = 0;
    _DAP_COMN_INFO* pstComnInfo = NULL;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    _DAP_QUEUE_BUF 	RsltQData;
    thrIdx      = *(int*)thrid;

    while (1)
    {
        memset((void *)&RsltQData,0x00,sizeof(RsltQData));

        switch(pstComnInfo->szDebugName[5])
        {
            case '1':
                rxt = fipc_FQGetData(DBIF01_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '2':
                rxt = fipc_FQGetData(DBIF02_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '3':
                rxt = fipc_FQGetData(DBIF03_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '4':
                rxt = fipc_FQGetData(DBIF04_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '5':
                rxt = fipc_FQGetData(DBIF05_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '6':
                rxt = fipc_FQGetData(DBIF06_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '7':
                rxt = fipc_FQGetData(DBIF07_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '8':
                rxt = fipc_FQGetData(DBIF08_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '9':
                rxt = fipc_FQGetData(DBIF09_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
            case '0':
                rxt = fipc_FQGetData(DBIF10_QUEUE, (char *)&RsltQData, sizeof(RsltQData));
                break;
        }

        if(g_pthread_stop_sign[thrIdx] == 0x20)
            break;

        if(rxt == 0)
        {
            sleep(1);
            continue;
        }

        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_IPC, "Fail in receive, rxt(%d)  ", rxt);
            sleep(1);
            continue;
        }
        else
        {
            WRITE_INFO(CATEGORY_IPC, "Succeed in receive, rxt(%d) ", rxt);

            rxt = fipc_SQPut(&RsltQData);
            if (rxt == -1)
            {
                WRITE_WARNING(CATEGORY_IPC, "Queue is full, current(%d) ", fipc_SQGetElCnt());
                for(retryCnt=1; retryCnt <= g_stProcDbifInfo.nRetryQLimitFailCount; retryCnt++)
                {
                    sleep(1);
                    rxt = fipc_SQPut(&RsltQData);
                    if (rxt == -1)
                    {
                        WRITE_WARNING(CATEGORY_DEBUG, "Queue is full, after sleep(%d/%d)",
                                      retryCnt,
                                      g_stProcDbifInfo.nRetryQLimitFailCount);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                WRITE_INFO(CATEGORY_IPC, "Put queue current(%d) ", fipc_SQGetElCnt() );
            }
        }
    }

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ",thrIdx);
    g_pthread_stop_sign[thrIdx] = 0x00;

    pthread_exit(NULL);

}*/

//int frd_ThreadDataInit(void)
//{
//    int local_nLoop = 0;
//
//    fcom_malloc((void**)&g_pstThreadDbInfo, sizeof(_DAP_THREAD_DBINFO) * FRD_MAX_THREAD);
//    for( local_nLoop = 0; local_nLoop < FRD_MAX_THREAD; local_nLoop++ )
//    {
//        g_pstThreadDbInfo[local_nLoop].UseFlag = 'N';
//    }
//
//
//}
//int frd_GetThreadUnuseIdx(int param_MaxThread)
//{
//    int local_nLoop = 0;
//
//    for( local_nLoop = 0; local_nLoop < param_MaxThread; local_nLoop++)
//    {
//        if ( g_pstThreadDbInfo[local_nLoop].UseFlag == 'N')
//        {
//            return local_nLoop;
//        }
//    }
//
//    return (-1);
//
//}
//
//void* frd_DbTaskThread(void* param_ThraedIdx)
//{
//    char local_DataFlag = 0;
//    int local_ThreadIdx = 0;
//    _DAP_THREAD_DBINFO* local_pstThreadInfo = NULL;
//
//    local_ThreadIdx = *(int*)param_ThraedIdx;
//    local_pstThreadInfo = &g_pstThreadDbInfo[local_ThreadIdx];
//
//    while(1)
//    {
//        pthread_rwlock_tryrdlock( &g_RwLockDbInfo );
//        local_DataFlag = local_pstThreadInfo->DataFlag;
//        pthread_rwlock_unlock( &g_RwLockDbInfo );
//
//        if( local_DataFlag == 'Y' )
//        {
//            pthread_rwlock_trywrlock( &g_RwLockDbInfo );
//            local_pstThreadInfo->UseFlag = 'Y'; //사용중으로 변경.
//            pthread_rwlock_unlock( &g_RwLockDbInfo );
//
//            frd_BufferToDBMS( local_ThreadIdx , local_pstThreadInfo);
//
//            pthread_rwlock_trywrlock( &g_RwLockDbInfo );
//            local_pstThreadInfo->UseFlag = 'N';
//            local_pstThreadInfo->DataFlag = 'N';
//            pthread_rwlock_unlock( &g_RwLockDbInfo );
//        }
//
//        fcom_SleepWait(3);
//    }
//
//    pthread_exit(NULL);
//
//}

