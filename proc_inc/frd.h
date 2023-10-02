//
// Created by KimByoungGook on 2021-03-11.
//

#ifndef FRD_H
#define FRD_H

#include <sys/un.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <strings.h>



#include "ipc/dap_qdef.h"
#include "db/dap_mysql.h"
#include "com/dap_req.h"

#define FORK_POLICY     0
#define FORK_SERVICE    1
#define FORK_DETECT     2

#define FRD_MAX_THREAD      3

#define MAX_DBFILE_INDEX      99999
#define MAX_DBFILE_SIZE       41943040 //40MB
#define SQL_FAIL_NAME_POLICY    "POLICY_FAIL.sql"
#define SQL_FAIL_NAME_DT        "DT_FAIL.sql"

int     g_SyslogUdsSockFd;
char    g_szSysLogUnixSockPath[128];  // Syslog Unix Socket Path
struct sockaddr_un g_stSyslogUdsSockAddr; // Syslog Unic Socket struct

/** 프로세스 설정 Struct **/
typedef struct
{
    char nDumpFlag; // 0x01 : Dump Active, 0x00 : Dump Inactive
    int  nForkCnt;
    int  nEorActive;
    int  nEorFormat;
    int  nCfgPrmonInterval;
    int	 nCfgKeepSession;
    int  nCfgAlarmActivation;
    unsigned int nCfgMaxFileSize;
    int  nCfgMaxFileIndex;
    int  nFileIdx;
    int  nNextIdx;
    int  nLastReadPosition;
    pid_t parentPid;
    pid_t* Pid;

    time_t nCfgLastModify;

    /* for dbms linker */
    long nCurTime;
    long nLastJobTime;
    long nLastSendTime;
    long nIngJobTime;

    char szEorHomeDir[127 +1];
    char szCpEorFile [127 +1];
    char szFileName  [31  +1];
    char szDtFilePath    [255 +1];
    char szPolicyFilePath[255 +1];

    char szNextFileName[31  +1];
    char szFileFullPath[255 +1];
    char szFileNextPath[255 +1];

    FILE* Fp;
    FILE* SqlFailFp;

}_DAP_PROC_FRD_INFO;




int                 g_ChildExitFlag;
int                 g_nForkIdx;
_DAP_PROC_FRD_INFO  g_stProcFrdInfo;
//_DAP_FRD_DBINFO     g_stFrdDbInfo;


//pthread_rwlock_t    g_RwLockDbInfo;

int frd_GetForkIdx(pid_t param_FindPid);
int frd_ParentExit(int sid);
int frd_ChildExit(void);

void frd_FQEventToPcif(_DAP_EventParam* p_EP);
void frd_FQEventToAlarm(_DAP_QUEUE_BUF *p_AQ, _DAP_EventParam * p_EP);
int frd_FromQToDBMS();
int	frd_MergeEventTb(_DAP_EventParam *p_EP, int p_stLen);
int frd_MergeHwBaseTb(_DAP_AGENT_INFO *p_AgentInfo,
                      char *p_detectTime,
                      _DAP_MAINBOARD *p_MainBoard,
                      char *p_prefix,
                      char *p_postfix);
int frd_MergeSystemTb(char				*p_userKey,
                      unsigned long long 	p_userSeq,
                      char 				*p_detectTime,
                      _DAP_SYSTEM 			*p_ST,
                      char 				*p_prefix,
                      char 				*p_postfix);
int frd_MergeConnectExtTb(
        char 				*p_userKey,
        unsigned long long 	p_userSeq,
        char 				*p_detectTime,
        _DAP_CONNECT_EXT 	*p_CE,
        char 				*p_prefix,
        char 				*p_postfix);
int frd_MergeOsTb(
        char 				 *p_userKey,
        unsigned long long 	 p_userSeq,
        char 				 *p_detectTime,
        _DAP_OPERATING_SYSTEM *p_OS,
        char 				  *p_prefix,
        char 			      *p_postfix);
int frd_MergeCpuTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char* 				p_detectTime,
        _DAP_CPU * 				p_CU,
        char* 				p_prefix,
        char* 				p_postfix);
int frd_MergeNetAdapterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETADAPTER*	p_NA,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeWifiTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WIFI*				p_WF,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeBluetoothTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_BLUETOOTH*	p_BT,
        char*				p_prefix,
        char*				p_postfix );
int frd_MergeNetConnectionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETCONNECTION *		p_NC,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeDiskTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_DISK *				p_DK,
        char*				p_prefix,
        char*				p_postfix,
        int	*				p_dkNewCnt,
        char*				p_resVal);
int frd_MergeNetDriveTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETDRIVE*			p_ND,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeNetPrinterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETPRINTER*		p_NP,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeNetScanTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_NETSCAN*			p_NS,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeWinDrvTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_WINDRV *			p_WD,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeRdpSessionTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_RDP_SESSION *		p_RDP,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeOsAccountTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_OSACCOUNT *	p_OA,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeShareFolderTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_SHARE_FOLDER*		p_SF,
        char*				p_prefix,
        char*				p_postfix);
int frd_MergeInfraredDeviceTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_INFRARED_DEVICE*	p_ID,
        char*				p_prefix,
        char*				p_postfix );
int frd_MergeProcessTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_PROCESS *			p_PS,
        char*				p_prefix,
        char*				p_postfix );
int frd_MergeRouterTb(
        char*				p_userKey,
        unsigned long long 	p_userSeq,
        char*				p_detectTime,
        _DAP_ROUTER *			p_RT,
        char*				p_prefix,
        char*				p_postfix );
int frd_MergeCpuUsageTb(char*			    	p_userKey,
                        unsigned long long 	p_userSeq,
                        char*				    p_detectTime,
                        _DAP_CPU_USAGE*		p_CpuUsage,
                        char*				    p_prefix,
                        char*				    p_postfix);
int frd_InsertEmptyBase(_DAP_AGENT_INFO* p_AgentInfo );
int frd_InsertAgentStatusHist(_DAP_AGENT_INFO* p_AgentInfo, char* param_AgentStatus, char* param_HbSq);
char* frd_GetPrevHistSq(char *p_tableName, char *p_colName, unsigned long long p_Sq);
char* frd_GetPrevHistSqByBase(char *p_tableName, char *p_colName, char *p_Sq);
char* frd_GetPrevHistSqWinDrv(	char *p_tableName,
                                  char *p_colName,
                                  unsigned long long p_Sq );
int frd_selectHbsqByHwBaseTb(char*				p_userKey,
                             unsigned long long*	p_hbSq);
int frd_SelectCountNewByDisk(
        unsigned long long 	p_hbSq,
        _DAP_DISK *			p_DK,
        char*				res) ;
int frd_SelectCountDuplByBase(
        char*			p_type,
        char*			p_userKey,
        _DAP_NETADAPTER *	p_NA,
        char*			res		);
int frd_CheckUnregDisk(_DAP_DISK *p_DK, char *p_userKey);
int frd_CheckChangeUserinfo(unsigned long long p_seq, char *p_ip);

int frd_TaskMainboard(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        char*               param_cmd,
        int*                retryBuffLen,
        int*                buffLen );

int frd_TaskSystem(
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
        int*                buffLen);

int frd_TaskConnExt  (
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
        int*		        buffLen);

int frd_TaskOperation(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                retryBuffLen,
        int*                buffLen);

int frd_TaskCpu(
        _DAP_QUEUE_BUF* 	RsltQData,
        _DAP_QUEUE_BUF*	    RetryQData,
        _DAP_DETECT* 		Detect,
        char*               param_Prefix,
        char*               param_Postfix,
        int*                param_bEmptyBase,
        int*                retryBuffLen,
        int*                buffLen);

int frd_TaskNetAdapter(
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
        int*    		    buffLen);

int frd_TaskWifi( _DAP_QUEUE_BUF *RsltQData,
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
                  int* buffLen);

int frd_TaskBlueTooth( _DAP_QUEUE_BUF *RsltQData,
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
                       int* buffLen);

int frd_TaskNetConnection( _DAP_QUEUE_BUF *RsltQData,
                           _DAP_QUEUE_BUF *EventQData,
                           _DAP_QUEUE_BUF *AlarmQData,
                           _DAP_QUEUE_BUF *RetryQData,
                           _DAP_EventParam *EventParam,
                           _DAP_DETECT* 		Detect,
                           char* param_Prefix,
                           char* param_Postfix,
                           int*  param_bEmptyBase,
                           int*  EventCount,
                           int*  bNotiFlag,
                           int*  retryBuffLen,
                           int*  buffLen );

int frd_TaskDisk( _DAP_QUEUE_BUF *RsltQData,
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
                  int* buffLen );

int frd_TaskNetDrive(  _DAP_QUEUE_BUF *RsltQData,
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
                       int* buffLen) ;

int frd_TaskWifi(
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
        int* buffLen );



int frd_TaskOsAccount(   _DAP_QUEUE_BUF *RsltQData,
                         _DAP_QUEUE_BUF *RetryQData,
                         _DAP_DETECT* 		Detect,
                         char* param_Prefix,
                         char* param_Postfix,
                         int*  param_bEmptyBase,
                         int*  retryBuffLen,
                         int*  buffLen );

int frd_TaskShareFolder(
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
        int *buffLen );

int frd_TaskInfrared(
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
        int *buffLen );

int frd_TaskProcess(
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
        int *buffLen );

int frd_TaskRouter(
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
        int *buffLen );

int frd_TaskNetPrinter(
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
        int *buffLen );

int frd_TaskNetScan(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen );

int frd_TaskSsoCert(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *EventQData,
        _DAP_QUEUE_BUF *AlarmQData,
        _DAP_EventParam *EventParam,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *EventCount,
        int *bNotiFlag,
        int *buffLen );

int frd_TaskWinDrv(
        _DAP_QUEUE_BUF *RsltQData,
        _DAP_QUEUE_BUF *RetryQData,
        _DAP_DETECT* 		Detect,
        char *param_Prefix,
        char *param_Postfix,
        int *param_bEmptyBase,
        int *retryBuffLen,
        int *buffLen );

int frd_TaskRdpSession(
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
        int *buffLen );

int frd_TaskCpuUsage(
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
        int *buffLen );

int frd_ReloadCfgFile(void);
int frd_Init(void);
void frd_WriteEor(_DAP_EventParam * p_EP);
int frd_GetLengthDiskMobileRead(_DAP_DISK *p_Disk);
int frd_GetLengthDiskMobileWrite(_DAP_DISK *p_Disk);
void* frd_DbifThread(void* thrid);
int frd_CheckChangeBaseinfo(char* p_userKey,
                            char *p_userIp,
                            char *p_userMac,
                            int  del,
                            int  hb_external,
                            int *resExt);
int	frd_SelectEvsqEventTb(unsigned long long	p_hbSq, // hb_sq of hw_base_tb
                             int p_evType, // event type
                             unsigned long long *p_evSq); // ev_sq of event_tb;
void frd_SigHandler(void);
void frd_SigchldHandler(int param_Sig);
void frd_HandleSignal(int sid);

int frd_ForkWork(void);
int frd_InitClearFile(int param_ForkIdx );
int frd_InitDbExecFail(int param_ForkIdx);
int frd_UpdateAccessByHwBaseTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix, int* hbExt);
int frd_UpdateExternalByHwBaseTb(_DAP_AGENT_INFO *p_AI, int *hbExt);
int frd_UpdateSysQueueTb(int tot, int value);
int frd_UpdateUserTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix);
int frd_MainTask();
int frd_GetLastFileIndex(char* param_FileName);
int frd_GetFirstFileIndex(char* param_FileName);
int frd_GetFirstFile(char* param_FileName, char* param_MMDD, int param_MMDDSize, char* param_TodayMMDD);
int frd_GetFirstFileMMDD(char* param_FileName , char* param_MMDD, int param_MMDDSize);
int frd_GetFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize);
int frd_ClearPolicyFile(char* param_FileName ,int param_FileIndex);
int frd_ClearDtFile(char* param_FileName , int param_FileIndex, char* param_ExceptFileName);

int frd_FileRead(int param_ForkIdx, FILE** param_Fp, _DAP_QUEUE_BUF* param_PackBuffer, int* param_PackType, int* FileNextFlag);
int frd_CheckEofFile(FILE* param_Fp, int param_nForkIdx);
int frd_BufferToDBMS(int param_ForkIdx, _DAP_QUEUE_BUF* param_QueueBuf);
int frd_GetNextFile(int param_ForkIdx );
int frd_GetLastPosition(void);
int frd_SetLastPosition(void);
void frd_WriteSqlFail(char* param_SqlBuffer, size_t param_SqlBufferLen );

int frd_DumpExec(void);
int frd_DumpDtLoop(void);
void frd_DumpDeleteDtLast(void);

void frd_ExecSyslog(char* param_ClientIp, int param_EventLevel, int param_EventType);
#endif //FRD_H
