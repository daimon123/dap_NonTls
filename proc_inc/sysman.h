//
// Created by KimByoungGook on 2020-06-19.
//

#ifndef SYSMAN_H
#define SYSMAN_H

#include "db/dap_defield.h"
#include "com/dap_def.h"
#include "linuxke/machine.h"

#define     MAX_SYSMAN_NOTI             2

typedef struct
{
    int	    cfgSysmanInterval;
    int		cfgPrmonInterval;
    int     cfgKeepSession;
    long	cur_time;
    long	last_job_time;
    long	last_send_time;
    time_t  nConfigLastModify;

    int      nFileSystem;
    int      nThreshold;
    int      nQueue    ;
}_DAP_PROC_SYSMAN_INFO;

_DAP_PROC_SYSMAN_INFO      g_stProcSysmanInfo;


struct	system_info   g_stSystemInfo;
_DAP_DB_SYSTEM_LOAD_INFO g_stSystemLoadInfo;


_DAP_DB_FILSYSTEM_STAT  fs[MAX_FILESYSTEM];
_DAP_DB_FILSYSTEM_STAT  *pFs;

_DAP_DB_THRESHOLD_INFO  g_stThreshold[MAX_THRESHOLD_CATEGORY];
_DAP_DB_THRESHOLD_INFO* g_pstThreshold;

_DAP_DB_QUEUE_STAT 	qs[MAX_QUEUE];
_DAP_DB_QUEUE_STAT 	*pQs;

static void fstHandleSignal(int sigNo);
void fstGuardSignal();
int fstSysmanInit();
void fstEndProc();
int fstLogInit();
int fstMainTask(void);
static int fstReloadConfig(void);
static int fstSaveDiskInfo();
static int fstSaveSystemInfo();
static int fstSaveQueueInfo();
static int fstGetPerCpuStaus(register int *states, _DAP_DB_SYSTEM_LOAD_INFO *data);
int fstCheckNotiFile(void);
int fstCheckAlaram(_DAP_DB_SYSTEM_LOAD_INFO    *pSysLoad,
                   _DAP_DB_FILSYSTEM_STAT      *pFs,
                   _DAP_DB_QUEUE_STAT 			*pQs,
                   int                      nMount,
                   int                      nQueue,
                   _DAP_DB_THRESHOLD_INFO      *pThreshold);
static void fstReloadCfgFile( );

/* sysman_db.c */
int fsysdb_GetFileStat(_DAP_DB_FILSYSTEM_STAT *pFileStat);
int fsysdb_LoadThreshold(_DAP_DB_THRESHOLD_INFO *pThreshold);
int fsysdb_GetSysDiskInfo(_DAP_DB_FILSYSTEM_STAT *pSysFile, int nProcess);
int fsysdb_UpdateFileStat(_DAP_DB_FILSYSTEM_STAT *pFileStat, int nMount);
int fsysdb_UpdateSysInfo(_DAP_DB_SYSTEM_LOAD_INFO *pSysLoad);
int fsysdb_UpdateSysAlarm(_DAP_DB_THRESHOLD_INFO *pThreshold, int value, char *pReason);
int fsysdb_GetQueueStat(_DAP_DB_QUEUE_STAT *pQueueStat);
#endif //SYSMAN_H
