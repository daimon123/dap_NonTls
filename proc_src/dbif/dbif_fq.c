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
#include "dbif.h"
#include "dap_version.h"

void fdbif_FQEventToPcif(_DAP_EventParam* p_EP)
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
            snprintf(mnqCpPath, sizeof(mnqCpPath),
                     "%s/%s", mnqPath,fileName);
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
void fdbif_FQEventToAlarm(_DAP_QUEUE_BUF *p_AQ, _DAP_EventParam * p_EP)
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

int fdbif_FromQToDBMS()
{
    int		i = 0;
    int		rxt = 0;
    int		retryCnt;
    int		buffLen = 0;
    int		retryBuffLen = 0;
    int		eventCount = 0;
    int		compareRetrySize = 0;
    int		pre_type = 0;
    int		stLen = 0;
    int		bBtDanger = 0;
    int 	bEmptyBase = 0;
    int		bNotiFlag = 0;
    int		hbExt = 0;
    int		retryMergeFailCount = 0;
    int		retryEventFailCount = 0;
    int     CurrMin = 0;
    char	*arrTokenKey = NULL;
    char	*tokenKey = NULL;
    char    *ptrTemp = NULL;
    _DAP_COMN_INFO* pstComnInfo = NULL;
    char	sMsgLen[5 +1] = {0x00,};
    char	prefix[6 +1] = {0x00,};
    char	postfix[4 +1] = {0x00,};
    char	cmdEventChange[128 +1] = {0x00,};
    char	cmdBaseChange[128 +1] = {0x00,};
    char	cmdCpChange[128 +1] = {0x00,};
    char	currDate[19 +1] = {0x00,};
    time_t	currTime = 0;
    time_t      chkMysqlCheckTime = 0;

    _DAP_QUEUE_BUF 	RsltQData;
    _DAP_QUEUE_BUF	RetryQData;
    _DAP_QUEUE_BUF	EventQData;
    _DAP_QUEUE_BUF	AlarmQData;
    _DAP_AGENT_INFO  AgentInfo;
    _DAP_DETECT 		Detect;
    _DAP_EventParam  EventParam;
    DETECT_ITEM DetectItem; //ENUM

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    memset(cmdEventChange, 0x00, sizeof(cmdEventChange));
    sprintf(cmdEventChange, "touch %s%s", getenv("DAP_HOME"),"/config/event_change");
    memset(cmdBaseChange, 0x00, sizeof(cmdBaseChange));
    sprintf(cmdBaseChange, "touch %s%s", getenv("DAP_HOME"),"/config/base_change");
    memset(cmdCpChange, 0x00, sizeof(cmdCpChange));
    sprintf(cmdCpChange, "touch %s%s", getenv("DAP_HOME"),"/config/cp_change");

    while(1)
    {
        g_stProcDbifInfo.nIngJobTime = time(NULL);
        CurrMin = fcom_GetSysMinute();

        memset((void *)&RsltQData,0x00,sizeof(RsltQData)); //정상데이터
        memset((void *)&RetryQData,0x00,sizeof(RetryQData)); //Retry할 All 데이터
        memset((void *)&EventQData,0x00,sizeof(EventQData)); //Retry할 Event 데이터

        if((g_stProcDbifInfo.nIngJobTime % g_stProcDbifInfo.nCfgPrmonInterval) == 0) //foreach 1min
        {
            fipc_PrmonLinker(pstComnInfo->szDebugName, 0, &g_stProcDbifInfo.nLastSendTime);
        }

        if((g_stProcDbifInfo.nIngJobTime % 5) == 0) //foreach 5sec
        {
            rxt = fdbif_UpdateSysQueueTb(g_stProcDbifInfo.nRsltQsize, fipc_SQGetElCnt());
            if(rxt < 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in updateCnt(%d) ", rxt);
            }
        }

        if((CurrMin % 3) == 0)
        {
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile,&g_stProcDbifInfo.nCfgLastModify) == 1)
                fdbif_ReloadCfgFile();
        }

        if(fipc_SQGet((_DAP_QUEUE_BUF *)&RsltQData) == NULL) // 큐가 비어있으면
        {
            if (g_stProcDbifInfo.nLastJobTime - g_stProcDbifInfo.nIngJobTime == 0 && pre_type == DATACODE_DETECT_DATA)
            {
                WRITE_INFO(CATEGORY_DB, "Try in commit db because queue is null " );
                rxt = fdbif_UpdateSysQueueTb(g_stProcDbifInfo.nRsltQsize, fipc_SQGetElCnt());
                if(rxt < 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Fail in updateCnt(%d) ", rxt);
                }
                pre_type = 0;
            }

            if(difftime(g_stProcDbifInfo.nIngJobTime, chkMysqlCheckTime) >= 60)
            {
                // MYSQL_OPT_TEST_CODE
                if (mysql_ping(g_stMyCon->connect) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Mysql ReConnect(mysql_ping) Is Fail code(%d) msg(%s) ",
                                   mysql_errno(g_stMyCon->connect),
                                   mysql_error(g_stMyCon->connect));
                }
                else
                {
                    WRITE_DEBUG(CATEGORY_DB,"Mysql ReConnect(mysql_ping) Is Success " );
                }
                chkMysqlCheckTime = time(NULL);
            }

            fcom_SleepWait(5);
            continue;
        }
        else
        {
            g_stProcDbifInfo.nLastJobTime = time(NULL);
            pre_type = RsltQData.packtype;
            WRITE_INFO(CATEGORY_INFO, "Get queue current(%d) ",fipc_SQGetElCnt());
        }

        memset(prefix, 0x00, sizeof(prefix));
        memset(postfix, 0x00, sizeof(postfix));

        fdb_GetHistoryDbTime(prefix, 0);
        fdb_GetHistoryTableTime(postfix, NULL, 0);

        WRITE_INFO(CATEGORY_INFO, "prefix(%s)postfix(%s)", prefix, postfix);

        if(	RsltQData.packtype == DATACODE_REQUEST_CFG ||
               RsltQData.packtype == EXTERNAL_REQUEST_CFG)
        {
            memset(&AgentInfo, 0x00, sizeof(AgentInfo));
            memcpy((void *)&AgentInfo, (void *)&RsltQData.buf, sizeof(AgentInfo));

            if(RsltQData.packtype == DATACODE_REQUEST_CFG)
            {
                WRITE_INFO(CATEGORY_INFO,
                           "[DQUE](%d) DATACODE_REQUEST_CFG ", fipc_SQGetElCnt());
                /* HW_BASE_TB UPDATE */
                fdbif_UpdateAccessByHwBaseTb(&AgentInfo, prefix, postfix, &hbExt);
                /* HW_BASE_TB.HB_EXTERNAL이 1이면(외부 접속유무) */
                if(hbExt == 1) // 1이면 이벤트 삭제
                {
                    currTime = time((time_t) 0);
                    fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");

                    memset(&EventParam, 0x00, sizeof(EventParam));

                    strcpy(	EventParam.user_key,	AgentInfo.user_key);
                    strcpy(	EventParam.user_ip,		AgentInfo.user_ip);
                    EventParam.user_seq		= AgentInfo.user_seq;
                    strcpy(	EventParam.detect_time,	currDate);
                    EventParam.ev_type		= EXTERNAL_CONN;
                    EventParam.ev_level		= CRITICAL;
                    strcpy(	EventParam.prefix,		prefix);
                    strcpy(	EventParam.postfix,		postfix);
                    strcpy(	EventParam.ev_context, "");
                    EventParam.ru_sq		= 0;
                    rxt = fdbif_MergeEventTb(&EventParam, 0);
                    if(rxt > 0)
                        bNotiFlag = 1;
                }

                // pcif에서 구조체 직접 핸들링 실패했을 경우
                // 200717 자식에서 access_time 변경된 내역을 부모로
                // 보내 다른 자식들에게도 update 하게 개선해야함
//                if(strlen(AgentInfo.access_time) == 0)
//               {
                system(cmdBaseChange);
//               }

            }
            else if(RsltQData.packtype == EXTERNAL_REQUEST_CFG)//isChange를 보지않는다.
            {
                WRITE_INFO(CATEGORY_INFO,  "[DQUE](%d) EXTERNAL_REQUEST_CFG ", fipc_SQGetElCnt());
                rxt = fdbif_UpdateExternalByHwBaseTb(&AgentInfo, &hbExt);
                if(hbExt == 0) // 0이면(처음이면) 알람발생
                {
                    currTime = time((time_t) 0);
                    fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");
                    memset(&EventParam, 0x02, sizeof(EventParam));
                    strcpy(	EventParam.user_key,	AgentInfo.user_key);
                    strcpy(	EventParam.user_ip,		AgentInfo.user_ip);
                    EventParam.user_seq 	= AgentInfo.user_seq;
                    strcpy(	EventParam.detect_time,	currDate);
                    EventParam.ev_type		= EXTERNAL_CONN;
                    EventParam.ev_level		= CRITICAL;
                    strcpy(	EventParam.prefix,		prefix);
                    strcpy(	EventParam.postfix,		postfix);
                    strcpy(	EventParam.ev_context, "ext");
                    EventParam.ru_sq		= 0;
                    rxt = fdbif_MergeEventTb(&EventParam, 1);
                    if(rxt > 0)
                    {
                        bNotiFlag = 1;
                        fdbif_FQEventToPcif(&EventParam);
                        if(g_stProcDbifInfo.nCfgAlarmActivation == 1)
                        {
                            memset(&AlarmQData, 0x00, sizeof(AlarmQData));
                            fdbif_FQEventToAlarm(&AlarmQData, &EventParam);
                        }
                        if(g_stProcDbifInfo.nEorActive == 1)
                        {
                            fdbif_WriteEor(&EventParam);
                        }
                    }
                }
            }
            if(bNotiFlag)
            {
                WRITE_INFO(CATEGORY_INFO,	"[NOTI](%s) userKey(%s)type(%d)level(%d)",
                           EventParam.user_ip,Detect.AgentInfo.user_key,
                           EventParam.ev_type,EventParam.ev_level);
                system(cmdEventChange);
                bNotiFlag = 0;
            }

            // update user_tb
            rxt = fdbif_UpdateUserTb(&AgentInfo, prefix, postfix);
            /* User IP 바꼇을때 */
            if (rxt > 0)
            {
                system(cmdCpChange);
                WRITE_DEBUG(CATEGORY_DEBUG,"User (%s) Ip is Changed cmd (%s)",EventParam.user_ip,cmdCpChange);

            }

        }
        else if(	RsltQData.packtype == DATACODE_DETECT_DATA ||
                    RsltQData.packtype == EXTERNAL_DETECT_DATA ||
                    RsltQData.packtype == DATACODE_ADDRESS_LIST)
        {
            if(RsltQData.packtype == DATACODE_DETECT_DATA)
            {
                WRITE_INFO(CATEGORY_IPC, "[DQUE](%d) DATACODE_DETECT_DATA ", fipc_SQGetElCnt());
                RetryQData.packtype = DATACODE_DETECT_DATA;
            }
            else if(RsltQData.packtype == EXTERNAL_DETECT_DATA)
            {
                WRITE_INFO(CATEGORY_IPC, "[DQUE](%d) EXTERNAL_DETECT_DATA ", fipc_SQGetElCnt() );
                RetryQData.packtype = EXTERNAL_DETECT_DATA;
            }
            else if(RsltQData.packtype == DATACODE_ADDRESS_LIST)
            {
                RetryQData.packtype = DATACODE_ADDRESS_LIST;
            }

            bEmptyBase = 0; //초기화 중요함

            EventQData.packtype = RETRY_EVENT_ALARM;

            memset(&Detect, 0x00, sizeof(Detect));

            //1. msgLen
            memset(sMsgLen, 0x00, sizeof(sMsgLen));
            memcpy(sMsgLen, (void *)&RsltQData.buf, sizeof(sMsgLen));
            buffLen = sizeof(sMsgLen);
            retryBuffLen = sizeof(sMsgLen);

            //2. Detect
            memcpy((void *)&Detect, (void *)&RsltQData.buf[buffLen], sizeof(Detect));
            memcpy((void *)&RetryQData.buf[retryBuffLen], (void *)&RsltQData.buf[buffLen], sizeof(Detect));

            buffLen += sizeof(Detect);
            retryBuffLen += sizeof(Detect);
            compareRetrySize = retryBuffLen;

            fjson_PrintAllDetect(&Detect);

            if(fcom_malloc((void**)&arrTokenKey, sizeof(char)*(strlen(Detect.change_item)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
                return (-1);
            }

            snprintf(arrTokenKey,strlen(Detect.change_item)+1,"%s",Detect.change_item );
            WRITE_INFO(CATEGORY_INFO, "arrTokenKey(%s)",
                       arrTokenKey);

            tokenKey = strtok_r(arrTokenKey, ",",&ptrTemp);

            while(tokenKey != NULL)
            {
                WRITE_INFO(CATEGORY_INFO, "tokenKey(%s) ", tokenKey);
                DetectItem = fjson_GetDetectItem(tokenKey);

                switch(DetectItem)
                {
                    case MAIN_BOARD:

                        fdbif_TaskMainboard(&RsltQData,
                                            &RetryQData,
                                            &Detect,
                                            prefix,
                                            postfix,
                                            cmdBaseChange,
                                            &retryBuffLen,
                                            &buffLen);
                        break;
                    case SYSTEM:
                        fdbif_TaskSystem(   &RsltQData,
                                            &EventQData,
                                            &AlarmQData,
                                            &RetryQData,
                                            &EventParam,
                                            &Detect,
                                            prefix,
                                            postfix,
                                            &bEmptyBase,
                                            &eventCount,
                                            &bNotiFlag,
                                            &retryBuffLen,
                                            &buffLen
                        );

                        break;

                    case CONNECT_EXT_SVR:
                        fdbif_TaskConnExt(&RsltQData,
                                          &EventQData,
                                          &AlarmQData,
                                          &RetryQData,
                                          &EventParam,
                                          &Detect,
                                          prefix,
                                          postfix,
                                          &bEmptyBase,
                                          &eventCount,
                                          &bNotiFlag,
                                          &retryBuffLen,
                                          &buffLen
                        );
                        break;
                    case OPERATING_SYSTEM:

                        fdbif_TaskOperation(&RsltQData,
                                            &RetryQData,
                                            &Detect,
                                            prefix,
                                            postfix,
                                            &bEmptyBase,
                                            &retryBuffLen,
                                            &buffLen
                        );
                        break;
                    case CPU:
                        fdbif_TaskCpu( &RsltQData,
                                       &RetryQData,
                                       &Detect,
                                       prefix,
                                       postfix,
                                       &bEmptyBase,
                                       &retryBuffLen,
                                       &buffLen
                        );
                        break;
                    case NET_ADAPTER:
                        fdbif_TaskNetAdapter(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen
                        );
                        break;
                    case WIFI:
                        fdbif_TaskWifi(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen
                        );
                        break;
                    case BLUETOOTH:
                        fdbif_TaskBlueTooth(&RsltQData,
                                            &EventQData,
                                            &AlarmQData,
                                            &RetryQData,
                                            &EventParam,
                                            &Detect,
                                            prefix,
                                            postfix,
                                            &bEmptyBase,
                                            &eventCount,
                                            &bNotiFlag,
                                            &retryBuffLen,
                                            &bBtDanger,
                                            &buffLen
                        );

                        break;
                    case NET_CONNECTION:
                        fdbif_TaskNetConnection(&RsltQData,
                                                &EventQData,
                                                &AlarmQData,
                                                &RetryQData,
                                                &EventParam,
                                                &Detect,
                                                prefix,
                                                postfix,
                                                &bEmptyBase,
                                                &eventCount,
                                                &bNotiFlag,
                                                &retryBuffLen,
                                                &buffLen);
                        break;
                    case DISK:
                        fdbif_TaskDisk(&RsltQData,
                                       &EventQData,
                                       &AlarmQData,
                                       &RetryQData,
                                       &EventParam,
                                       &Detect,
                                       prefix,
                                       postfix,
                                       &bEmptyBase,
                                       &eventCount,
                                       &bNotiFlag,
                                       &retryBuffLen,
                                       &buffLen
                        );
                        break;
                    case NET_DRIVE:
                        fdbif_TaskNetDrive(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case OS_ACCOUNT:
                        fdbif_TaskOsAccount(
                                &RsltQData,
                                &RetryQData,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &retryBuffLen,
                                &buffLen);

                        break;
                    case SHARE_FOLDER:
                        fdbif_TaskShareFolder(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case INFRARED_DEVICE:
                        fdbif_TaskInfrared(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen
                        );
                        break;
                    case PROCESS:
                        fdbif_TaskProcess(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case ROUTER:
                        fdbif_TaskRouter(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case NET_PRINTER:
                        fdbif_TaskNetPrinter(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen
                        );
                        break;
/*
				case ARP:

					memcpy(	(void *)&DetectData.Arp,
							(void *)&RsltQData.buf[buffLen],
							sizeof(DetectData.Arp) );
					buffLen += sizeof(DetectData.Arp);
					print_all_arp(&DetectData.Arp);
					rxt = merge_arp_tb(Detect.AgentInfo.user_key, &DetectData.Arp, prefix, postfix);
					if(rxt < 0) {
						if(rxt == -2) bEmptyBase = 1;
						memcpy(	(void *)&RetryQData.buf[retryBuffLen],
								(void *)&DetectData.Arp,
								sizeof(DetectData.Arp) );
						retryBuffLen += sizeof(DetectData.Arp);
					}
					break;
*/
                    case NET_SCAN:
                        fdbif_TaskNetScan(
                                &RsltQData,
                                &RetryQData,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case SSO_CERT:
                        fdbif_TaskSsoCert(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &eventCount,
                                &bNotiFlag,
                                &buffLen);
                        break;
                    case WIN_DRV:
                        fdbif_TaskWinDrv(
                                &RsltQData,
                                &RetryQData,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &retryBuffLen,
                                &buffLen);
                        break;
                    case RDP_SESSION:
                        fdbif_TaskRdpSession(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen
                        );
                        break;
                    case CPU_USAGE:
                        fdbif_TaskCpuUsage(
                                &RsltQData,
                                &EventQData,
                                &AlarmQData,
                                &RetryQData,
                                &EventParam,
                                &Detect,
                                prefix,
                                postfix,
                                &bEmptyBase,
                                &eventCount,
                                &bNotiFlag,
                                &retryBuffLen,
                                &buffLen );
                        break;
                    default:
                    WRITE_INFO(CATEGORY_DB, "Unknown detect item(%d) ", DetectItem);
                        break;
                }//switch

                if( bNotiFlag )
                {
                    WRITE_INFO(CATEGORY_DB,	"[NOTI](%s) userKey(%s)type(%d)level(%d)",
                               EventParam.user_ip,Detect.AgentInfo.user_key,
                               EventParam.ev_type,EventParam.ev_level);

                    system(cmdEventChange);
                    bNotiFlag = 0;
                }
                tokenKey = strtok_r(NULL, ",",&ptrTemp);
            }//while

            fcom_MallocFree((void**)&arrTokenKey);
            tokenKey = NULL;

            if(bEmptyBase)
            {
                //merge_hw_base_tb와는 다르게 hb_first_time을 기록하지 않음
                rxt = fdbif_InsertEmptyBase(&Detect.AgentInfo);

                if(rxt < 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Fail in insert_empty_base ");
                }
                else
                {
                    WRITE_INFO(CATEGORY_DB, "Succeed in insert_empty_base ");
                    system(cmdBaseChange);
                }
            }

            // retry in case of merge failure
            if(	retryBuffLen > compareRetrySize )
            {
                if( retryMergeFailCount < g_stProcDbifInfo.nRetryQLimitFailCount )
                {
                    memset(sMsgLen, 0x00, sizeof(sMsgLen));
                    snprintf(sMsgLen, sizeof(sMsgLen), "%d", retryBuffLen);
                    memcpy((void *)&RetryQData.buf, &sMsgLen, sizeof(sMsgLen));
                    rxt = fipc_SQPut(&RetryQData);
                    if (rxt < 0)
                    {
                        WRITE_WARNING(CATEGORY_IPC,	"Retry queue is full, current(%d) ", fipc_SQGetElCnt());
                        for(retryCnt=1; retryCnt <= g_stProcDbifInfo.nRetryQLimitFailCount; retryCnt++)
                        {
                            sleep(1);
                            rxt = fipc_SQPut(&RetryQData);
                            if (rxt < 0)
                            {
                                WRITE_WARNING(CATEGORY_IPC,	"Retry queue is full, after retry(%d/%d) ",
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
                        WRITE_INFO(CATEGORY_INFO, "Retry queue current(%d)",
                                   fipc_SQGetElCnt());
                        sleep(1);
                    }
                    retryMergeFailCount++;
                }
                else
                {
                    retryMergeFailCount = 0;
                }
            }

            // retry in case of event failure
            if( eventCount > 0 )
            {
                if( retryEventFailCount < g_stProcDbifInfo.nRetryQLimitFailCount )
                {
                    memset(sMsgLen, 0x00, sizeof(sMsgLen));
                    snprintf(sMsgLen, sizeof(sMsgLen),"%d", eventCount);
                    memcpy((void *)&EventQData.buf, &sMsgLen, sizeof(sMsgLen));
                    rxt = fipc_SQPut(&EventQData);
                    if (rxt < 0)
                    {
                        WRITE_WARNING(CATEGORY_DB,	"Event queue is full, current(%d)", fipc_SQGetElCnt());
                        for(retryCnt=1; retryCnt <= g_stProcDbifInfo.nRetryQLimitFailCount; retryCnt++)
                        {
                            sleep(1);
                            rxt = fipc_SQPut(&EventQData);
                            if (rxt < 0)
                            {
                                WRITE_WARNING(CATEGORY_IPC,	"Event queue is full, after retry(%d/%d)",
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
                        WRITE_INFO(CATEGORY_DEBUG, "Event queue current(%d)", fipc_SQGetElCnt() );
                        sleep(1);
                    }
                    retryEventFailCount++;
                }
                else
                {
                    retryEventFailCount = 0;
                }
            }
        }
        else if(RsltQData.packtype == RETRY_EVENT_ALARM)
        {
            WRITE_INFO(CATEGORY_IPC, "[DQUE](%d) RETRY_EVENT_ALARM ", fipc_SQGetElCnt() );
            memset(sMsgLen, 0x00, sizeof(sMsgLen));
            memcpy(sMsgLen, (void *)&EventQData.buf, sizeof(sMsgLen));
            WRITE_INFO(CATEGORY_IPC, "sMsgLen(%d)", atoi(sMsgLen));
            eventCount = atoi(sMsgLen);

            for(i=0; i<eventCount; i++)
            {
                memset(&EventParam, 0x00, sizeof(EventParam));
                memcpy(	(void *)&EventParam,
                           (void *)&EventQData.buf[eventCount*sizeof(EventParam)],
                           sizeof(EventParam) );
                rxt = fdbif_MergeEventTb(&EventParam,stLen);
                if(rxt < 0)
                {
                    rxt = fipc_SQPut(&EventQData);
                    if (rxt < 0)
                    {
                        WRITE_WARNING(CATEGORY_IPC,	"Event queue is full, current(%d)",
                                      fipc_SQGetElCnt());
                        for(retryCnt=1; retryCnt <= g_stProcDbifInfo.nRetryQLimitFailCount; retryCnt++)
                        {
                            sleep(1);
                            rxt = fipc_SQPut(&EventQData);
                            if (rxt < 0)
                            {
                                WRITE_WARNING(CATEGORY_IPC,	"Event queue is full, after retry(%d/%d) ",
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
                        WRITE_INFO(CATEGORY_IPC, "Event queue is current(%d) ",
                                   fipc_SQGetElCnt());
                        sleep(1);
                    }
                }
                else
                {
                    if(rxt > 0)
                    {
                        WRITE_INFO(CATEGORY_INFO, "[NOTI](%s) userKey(%s)type(%d)level(%d)",
                                   EventParam.user_ip,Detect.AgentInfo.user_key,
                                   EventParam.ev_type,EventParam.ev_level);

                        system(cmdEventChange);
                    }
                    if(g_stProcDbifInfo.nEorActive == 1)
                    {
                        fdbif_WriteEor(&EventParam);
                    }
                }
            }
        }
        else
        {
            WRITE_CRITICAL(CATEGORY_IPC, "Unknown result queue type(%d)",
                           RsltQData.packtype);
        }
    }
    return 0;
}
