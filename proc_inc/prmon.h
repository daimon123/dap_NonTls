//
// Created by KimByoungGook on 2020-06-16.
//

#ifndef PRMON_H
#define PRMON_H

enum ENUM_TIME_FORMAT
{
    INIT_TIME  = 0,
    PROXY_TIME = 1,
//    LINKER_TIME ,
    DBIF01_TIME ,
    DBIF02_TIME ,
    DBIF03_TIME ,
    DBIF04_TIME ,
    DBIF05_TIME ,
    PCIF_TIME   ,
    SYSMAN_TIME ,
    SCHD_TIME   ,
    DBLOG_TIME  ,
    REPORT_TIME ,
    FWDER_TIME  ,
    ALARM_TIME  ,
    REPLMON_TIME ,
    VWLOG_TIME
};

typedef struct
{

    int cfgCdrHeng;
    int	cfgProcCount;
//    int	cfgLinkerActive;
    int	cfgProxyActive;
    int	cfgSysmanActive;
    int	cfgFwderActive;
    int	cfgSchdActive;
    int	cfgReportActive;
    int	cfgAlarmActive;
    int	cfgReplmonActive;
    int cfgVwlogActive;
    char cfgAccount[64+1];
    char cfgDapTmpFile[128+1];

    long		nLastProxyTime;
//    long		nLastLinkerTime;
    long		nLastDbif01Time;
    long		nLastDbif02Time;
    long		nLastDbif03Time;
    long		nLastDbif04Time;
    long		nLastDbif05Time;
    long		nLastPcifTime;
    long		nLastSysmanTime;
    long		nLastSchdTime;
    long		nLastDblogTime;
    long		nLastReportTime;
    long		nLastFwderTime;
    long		nLastAlarmTime;
    long		nLastReplmonTime;
    long		nLastVwlogTime;

    time_t nConfigLastModify;

}_DAP_PROC_PRMON_INFO;

_DAP_PROC_PRMON_INFO g_stProcPrmonInfo;

#define    MAX_THREAD 1

char g_pthreadStopFlag;

static int fstPrmonInit(void);

void fstPrmonSetTime(enum ENUM_TIME_FORMAT nPrmonType);
int fstCheckDaemon(enum ENUM_TIME_FORMAT nPrmonType);
void* fstThreadMon(void *thrid);
static int fstKillProc( char *p_name );
int fstRecvFromProc(void);

#endif //PRMON_H
