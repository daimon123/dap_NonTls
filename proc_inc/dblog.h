//
// Created by KimByoungGook on 2020-06-23.
//

#ifndef DBLOG_H
#define DBLOG_H


#define MAX_THREAD 1

typedef struct
{
    int  nCfgRsltQsize;
    int  nCfgRetryQCount;
    int  nCfgPrmonInterval;
    int  nCfgKeepSession;

    long nCurTime;
    long nLastJobTime;
    long nLastSendTime;
    long nIngJobTime;

    time_t nCfgLastModify;

}_DAP_PROC_DBLOG_INFO;

_DAP_PROC_DBLOG_INFO  g_stProcDblogInfo;


char g_pthreadStopFlag;

void* fstDblogThread(void* thrid);
void fstKeepSession();


#endif //DAP_NEW_DBLOG_H
