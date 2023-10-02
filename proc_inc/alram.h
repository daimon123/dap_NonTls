//
// Created by KimByoungGook on 2020-06-23.
//

#ifndef ALRAM_H
#define ALRAM_H

#include "db/dap_defield.h"

#define ALARM_CHANGE   0
#define	MAX_ALARM_NOTI 1

#define MAX_ALARM_COUNT    10000

#define     EVENT_ALARM					101
#define     MAX_THREAD    1
typedef struct
{
    int  nCfgRsltQsize;
    int  nCfgRetryQCount;
    int  nCfgPrmonInterval;
    int	 nCfgKeepSession;
    int	 nCfgAlarmNotiInterval;
    int  nCfgSendMailDelayTime;

    long nCurTime;
    long nLastJobTime;
    long nLastSendTime;
    long nIngJobTime;
    int nAlarm;
    time_t nConfigLastModify;

    /* SMS Info */
    char szConfSmsServerIp[15+1];
    int	 nConfSmsServerPort;
    char szConfSmsFrom[11+1];
    char szConfSmsLang[2+1];
    char szConfMailFrom[64];
    char szConfMailLang[2+1];

    /* Reload Flag */
    int	 bFirstReloadFinish;

    char szCfgSendMailTempPath[256];

}_DAP_PROC_ALARM_INFO;

_DAP_PROC_ALARM_INFO  g_stProcAlarmInfo;

_DAP_DB_ALARM_INFO    alarmInfo[MAX_ALARM_COUNT];
_DAP_DB_ALARM_INFO*   pAlarmInfo;


char g_pthreadStopFlag;

int fstAlarmInit();
int fstReloadConfig(void);
void fstHandleSignal(int sid);
static void fstSigHandler();
static int fstMainTask();
void* fstAlarmThread(void *thrid);
int fstFromQToAlarm();
static int fstCheckNotiFile(void);
static int fstSendDelayMail(char* delayMsg);
static int fstSendMail(char* mailTo, char* msg);
static int fstCheckSmsAlarm(_DAP_EventParam *p_EP);
static int fstCheckMailAlarm(_DAP_EventParam *p_EP);
static void fstReloadCfgFile( );

/* alram_db.c */
int falarm_GetAlarmInfo(char* p_smsIp, int p_smsPort, char* p_smsFrom, char* p_smsLang,char* p_mailFrom,char* p_mailLang);
int falarm_LoadAlarm(_DAP_DB_ALARM_INFO *pAlarmInfo);
#endif //ALRAM_H
