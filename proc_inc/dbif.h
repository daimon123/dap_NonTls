//
// Created by KimByoungGook on 2020-06-26.
//

#ifndef _DBIF_H
#define _DBIF_H

#include "com/dap_req.h"
#include "ipc/dap_qdef.h"

#define MAX_THREAD  1

typedef struct
{
    int  nEorActive;
    int  nEorFormat;
    int  nRsltQsize;
    int  nRetryQLimitFailCount;
    int  nCfgPrmonInterval;
    int	 nCfgKeepSession;
    int  nCfgAlarmActivation;

    time_t nCfgLastModify;

    /* for dbms linker */
    long nCurTime;
    long nLastJobTime;
    long nLastSendTime;
    long nIngJobTime;

    char szEorHomeDir[128];
    char szCpEorFile[128];
    char szSaveFilePath[256];

}_DAP_PROC_DBIF_INFO;

_DAP_PROC_DBIF_INFO g_stProcDbifInfo;

char g_pthread_stop_sign[MAX_THREAD];


void* fdbif_DbifThread(void* thrid);
int fdbif_FromQToDBMS();
int fdbif_BufferToDBMS(int param_ThreadIdx);
void fdbif_FQEventToPcif(_DAP_EventParam * p_EP);
void fdbif_FQEventToAlarm(_DAP_QUEUE_BUF *p_AQ, _DAP_EventParam * p_EP);

void fdbif_HandleSignal(int sid);
int fdbif_ReloadCfgFile(void);
void fdbif_SigHandler(void);
int fdbif_DbifInit(void);
void fdbif_WriteEor(_DAP_EventParam * p_EP);
/* ------------------------------------------------------------------- */

int fdbif_UpdateSysQueueTb(int tot, int value                       );
int fdbif_UpdateExternalByHwBaseTb(_DAP_AGENT_INFO *p_AI, int *hbExt);
int fdbif_CheckChangeBaseinfo(char* p_userKey,
                              char *p_userIp,
                              char *p_userMac,
                              int  del,
                              int  hb_external,
                              int *resExt);
int fdbif_CheckUnregDisk(_DAP_DISK *p_DK, char *p_userKey);
int fdbif_CheckChangeUserinfo(unsigned long long p_seq, char *p_ip);
int fdbif_UpdateAccessByHwBaseTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix, int* hbExt);
int fdbif_UpdateUserTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix);
int fdbif_InsertEmptyBase(_DAP_AGENT_INFO* p_AgentInfo                          );
int fdbif_selectHbsqByHwBaseTb(char*				p_userKey, // pc unique key
                               unsigned long long*	p_hbSq);   // result variable;
char* fdbif_GetPrevHistSq(char *p_tableName, char *p_colName, unsigned long long p_Sq);
int fdbif_GetLengthDiskMobileRead(_DAP_DISK *p_Disk);
int fdbif_GetLengthDiskMobileWrite(_DAP_DISK *p_Disk);
char* fdbif_GetPrevHistSqByBase(char *p_tableName, char *p_colName, char *p_Sq);
char* fdbif_GetPrevHistSqWinDrv(	char *p_tableName,
                                    char *p_colName,
                                    unsigned long long p_Sq );
int	fdbif_MergeEventTb(_DAP_EventParam *p_EP, int p_stLen);
int fdbif_MergeHwBaseTb(_DAP_AGENT_INFO *p_AgentInfo,
                        char *p_detectTime,
                        _DAP_MAINBOARD *p_MainBoard,
                        char *p_prefix,
                        char *p_postfix);
int fdbif_MergeSystemTb(char				*p_userKey,
                        unsigned long long 	p_userSeq,
                        char 				*p_detectTime,
                        _DAP_SYSTEM 			*p_ST,
                        char 				*p_prefix,
                        char 				*p_postfix);
int fdbif_MergeConnectExtTb(
        char 				*p_userKey,
        unsigned long long 	p_userSeq,
        char 				*p_detectTime,
        _DAP_CONNECT_EXT 	*p_CE,
        char 				*p_prefix,
        char 				*p_postfix
);
int fdbif_MergeOsTb(
        char 				 *p_userKey,
        unsigned long long 	 p_userSeq,
        char 				 *p_detectTime,
        _DAP_OPERATING_SYSTEM *p_OS,
        char 				  *p_prefix,
        char 			      *p_postfix);
int fdbif_MergeCpuTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char* 				p_detectTime,
        _DAP_CPU * 				p_CU,
        char* 				p_prefix,
        char* 				p_postfix
);
int	fdbif_SelectEvsqEventTb(unsigned long long	p_hbSq, // hb_sq of hw_base_tb
                               int p_evType, // event type
                               unsigned long long *p_evSq); // ev_sq of event_tb;
int fdbif_SelectCountNewByDisk(
        unsigned long long 	p_hbSq, // hb_sq of HW_BASE_TB
        _DAP_DISK *			p_DK,   // structs of Disk
        char*				res);    // result variable
int fdbif_SelectCountDuplByBase(
        char*			p_type,		// distinguished columns
        char*			p_userKey,	// pc unique key
        _DAP_NETADAPTER *	p_NA,		// structs of NetAdapter
        char*			res		// result variables
);
int fdbif_UpdateExternalByHwBaseTb(_DAP_AGENT_INFO *p_AI, int *hbExt);
void fdbif_WriteEor(_DAP_EventParam * p_EP);
int fdb_GetIpByBase(
        char*	p_type, 		// distinguished columns
        char*	p_userKey, 		// pc unique key
        char*	p_hbAccessRes);	// result variable

int fdbif_MergeNetAdapterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETADAPTER*	p_NA,
        char*				p_prefix,
        char*				p_postfix);

int fdbif_MergeInfraredDeviceTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_INFRARED_DEVICE*	p_ID,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeProcessTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_PROCESS *			p_PS,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeRouterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_ROUTER *			p_RT,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeCpuUsageTb(char*			    	p_userKey,
                          unsigned long long 	p_userSeq,
                          char*				    p_detectTime,
                          _DAP_CPU_USAGE*		p_CpuUsage,
                          char*				    p_prefix,
                          char*				    p_postfix);

int fdbif_MergeWifiTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WIFI*				p_WF,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeBluetoothTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_BLUETOOTH*	p_BT,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeNetConnectionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETCONNECTION *		p_NC,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeDiskTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_DISK *				p_DK,
        char*				p_prefix,
        char*				p_postfix,
        int	*				p_dkNewCnt,
        char*				p_resVal);
int fdbif_MergeNetDriveTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETDRIVE*			p_ND,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeNetPrinterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETPRINTER*		p_NP,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeNetScanTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETSCAN*			p_NS,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeWinDrvTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WINDRV *			p_WD,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeRdpSessionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_RDP_SESSION *		p_RDP,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeShareFolderTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_SHARE_FOLDER*		p_SF,
        char*				p_prefix,
        char*				p_postfix);
int fdbif_MergeOsAccountTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_OSACCOUNT *	p_OA,
        char*				p_prefix,
        char*				p_postfix
);
int fdbif_MergeNetDriveTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETDRIVE*			p_ND,
        char*				p_prefix,
        char*				p_postfix);



/* ------------------------------------------------------------------- */

int fdbif_TaskMainboard(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        char*               param_cmd,
        int*                retryBuffLen,
        int*                buffLen
);

/* �ý������� HW_BASE_TB ó�� */
int fdbif_TaskSystem(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    EventQData,
        _DAP_QUEUE_BUF*  	AlarmQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_EventParam*    EventParam,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                EventCount,
        int*		        bNotiFlag,
        int*                retryBuffLen,
        int*                buffLen
);


/* �ܺο��� HW_BASE_TB ó�� */
int fdbif_TaskConnExt  (
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    EventQData,
        _DAP_QUEUE_BUF*  	AlarmQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_EventParam*    EventParam,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                EventCount,
        int*		        bNotiFlag,
        int*                retryBuffLen,
        int*		        buffLen
);

int fdbif_TaskOperation(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                retryBuffLen,
        int*                buffLen
);
int fdbif_TaskCpu(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                retryBuffLen,
        int*                buffLen
);
int fdbif_TaskNetAdapter(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    EventQData,
        _DAP_QUEUE_BUF*  	AlarmQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_EventParam*    EventParam,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                EventCount,
        int*		        bNotiFlag,
        int*                retryBuffLen,
        int *buffLen
);
int fdbif_TaskWifi(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskBlueTooth(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *bBtDanger,
        int *buffLen
);
int fdbif_TaskNetConnection(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskDisk(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskNetDrive(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskOsAccount(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int	*buffLen
);
int fdbif_TaskShareFolder(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskInfrared(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen);
int fdbif_TaskProcess(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskRouter(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen);
int fdbif_TaskNetPrinter(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskNetScan(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen);
int fdbif_TaskSsoCert(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *EventCount,
        int *bNotiFlag,
        int *buffLen
);
int fdbif_TaskWinDrv(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen
);
int fdbif_TaskRdpSession(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *bNotiFlag,
        int *retryBuffLen,
        int *buffLen);
int fdbif_TaskCpuUsage(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *EventCount,
        int *retryBuffLen,
        int *bBtDanger,
        int *buffLen

);
#endif //_DBIF_H
