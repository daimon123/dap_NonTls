//
// Created by KimByoungGook on 2020-06-22.
//

#ifndef VWLOG_H
#define VWLOG_H

#include <openssl/ssl.h>

#define BUFSIZE 512

typedef struct
{
    /* vwlog */
    /* ---------------- */
    int     nCfgRsltQsize;
    int     nCfgRetryQCount;
    int     nCfgPrmonInterval;
    /* ---------------- */

    /* tail */
    /* ---------------- */
    int     nCfgRetryAckCount;
    int     nCfgRetryAckSleep;
    int     nCfgMaxSendPerSecond;
    int     nCfgSendDelayTime;

    char    szServerIp[15+1];
    char 	szManagerIp[15+1];
    char 	szFilePath[526];
    int		nManagerFq;
    time_t  nConfigLastModify;
    /* ---------------- */

    long    nIngJobTime;
    long    nLastJobTime;
    long    nLastSendTime;
    char    szCfgSaveFilePath[256];
}_DAP_PROC_VWLOG_INFO;

_DAP_PROC_VWLOG_INFO g_stProcVwlogInfo;

#define MAX_THREAD 1
//char g_pthread_stop_sign[MAX_THREAD];
char g_pthreadStopFlag;

void fstCleanTail(char* clientIp, char* clientPath);
void* fstThreadRecv(void *thrid);

/* tail_main.c */
void fstAlarmInterrupt(int signo);
#endif //VWLOG_H
