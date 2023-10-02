//
// Created by KimByoungGook on 2020-06-10.
//

#ifndef MASTER_H
#define MASTER_H

#include "linuxke/dap_linux.h"
#include "linuxke/machine.h"


//#define     MAX_PROCESS_COUNT   30
/* Fork 개수 포함하여 30 -> 45개로 늘림. */
/* Fork 개수 포함하여 45 -> 60개로 늘림. */
#define     MAX_PROCESS_COUNT   60
#define     PERIOD_STAT		    15		/*	perodic statistics interval	*/

typedef struct
{
    unsigned int    mgwid;
    int             index;
    unsigned char   pname[20+1];  /* pkey not null */
    unsigned char   args[100+1];
    unsigned long   maxnum;         /* maximun child process num */
    unsigned char   status;
    unsigned long   pid;
    unsigned long   ppid;
    unsigned long   stime;
    unsigned long   ktime;
    unsigned long   p_cpu;
    unsigned long   p_size;
    unsigned short  p_priority;
    unsigned char   autoflag;       /* A: auto start, K : killed,  R:Running N : noting */
} _PROCESS_INFO; /* 프로세스 DB Fork제외한 상태 Update용 */

_PROCESS_INFO     g_stProcessInfo[MAX_PROCESS_COUNT];
_PROCESS_INFO*    g_ptrProcessInfo;

int			      gProcess;

int               g_nStartFlag;// 기동시 Process Info Read하기위한 Flag 변수.

void fmaster_RestartProcess(void);

/* master_process.c */
int fmaster_GetOneprocessInfo(char* pname,char* parg);
int fmaster_CheckConfigFlag(void);
int fmaster_ReadPinfo(_PROCESS_INFO *pinfo);
int fmaster_SaveProcessInfo(void);
int fmaster_GetProcessInfo(_PROCESS_INFO *pInfo);
int fmaster_UpdatePinfo(_PROCESS_INFO *pinfo, int nProcess);
int fmaster_CheckProcAlarm(_PROCESS_INFO *pInfo, int nProc);
int fmaster_InvokeProcess(_PROCESS_INFO* pInfo);
int fmaster_KillProcess(_PROCESS_INFO *pInfo);
//int fmaster_KillAllProcess(_ALL_SYS_PROC_INFO *pSysInfo);
int fmaster_KillAllProcess(_PROCESS_INFO* pSysInfo);
int fmaster_CleanPinfo(_PROCESS_INFO *pinfo, int nProcess);

void fmaster_DeleteServicelogByDay(char* param_LogHome);
void fmaster_DeleteDirPcif(char* FilePath, int CfgDay);
void fmaster_DeletePciflogByDay(char *specify_file_path, int valid_day);
int fmaster_ManageLogSize(char* Logpath) /* $DAP_HOME/log/service */;
int fmaster_DeleteLogProcess(void);
int fmaster_DeleteBackupProcess(void);
int fmaster_RenameLogProcess(void);
int fmaster_DeleteDbProcess(void);
void fmaster_DeleteMngQueue(void);

int fmaster_MasterCommandCheck(void);
int fmaster_GetPrintRlim(void);
void fmaster_SetRlim(void);
int fmaster_SetMasterPid(void);
int fmaster_ReloadConfig(void);
void fmaster_FrdDumpExec(void);

void fmaster_GuardSignal(void);
void fmaster_ReloadCfgFile(void);
void fmaster_HandleSignal(int sigNo);
void fmaster_ExitServer(void);
int fmaster_CheckNotiFile(void);
int fmaster_DbReconnect(void);
int fmaster_SetMasterPid(void);
void fmaster_SetRlim(void);
int fmaster_GetPrintRlim(void);
int fmaster_MasterCommandCheck(void);

#endif //MASTER_H
