#include    <stdio.h>
#include    <stdarg.h>
#include    <sys/types.h>
#include    <time.h>
#include    <sys/fcntl.h>
#include    <sys/stat.h>
#include    <errno.h>
#include    <unistd.h>
#include    <sys/timeb.h>
#include    <string.h>
#include    <stdlib.h>

#include    <pthread.h>



#include    "com/dap_def.h"
#include    "com/dap_com.h"
#include    "db/dap_defield.h"
#include    "ipc/dap_Queue.h"

static int fstGetLogTime(char *timebuf);
static FILE	* fstOpenEorFile(int type, char* cpLFile);
int fstSendDBLog(char* param_LogIP, int param_LogLevel, char *contents); // 0x00 : server , 0x01 : agent;

/* Debug level is 1:critical,2:major,3:warning,4,info,5:debug */
static _LOGGER_LEVEL g_stLogerLevel[5] =
{
        {DAP_CRITICAL, "CRITICAL"},
        {DAP_MAJOR   , "MAJOR"   },
        {DAP_WARN , "WARNING" },
        {DAP_INFO    , "INFO"    },
        {DAP_DEBUG   , "DEBUG"}
};

/* Category Level Is DB 1 , IPC 2, INFO 3, DEBUG 4 */
static _LOGGER_CATEGORY g_stLoggerCategory[CATEGORY_DEBUG] =
{
        {CATEGORY_DB   , "CAT____DB"},
        {CATEGORY_IPC  , "CAT___IPC"},
        {CATEGORY_INFO , "CAT__INFO"},
        {CATEGORY_DEBUG, "CAT_DEBUG"}
};

char	g_szCpEorFile[128];

int fcom_InitStdLog(const char *log_file)
{
    int local_fd;

    if((local_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND , 0664 )) == -1) { return 0; }


    /** 표준출력 및 에러 리다이렉션 **/
    dup2(local_fd, 1);
    dup2(local_fd, 2);

    if ( local_fd != 0)
    {
        close(local_fd);
    }


    return 0;
}

void fcom_LogTime(FILE* Logfp)
{
    struct timeb itb;
//    struct tm *lt;
    struct tm lt;

    char    cpDate[30] = {0x00,};

    memset(&lt, 0x00, sizeof(struct tm));

    ftime(&itb);
//    lt = localtime(&itb.time);
    localtime_r(&itb.time,&lt);

    memset(cpDate, 0x00, sizeof(cpDate));
    sprintf(cpDate, "%02d:%02d:%02d.%03d", lt.tm_hour, lt.tm_min, lt.tm_sec, itb.millitm);

    fprintf(Logfp, "%s ", cpDate);
}


void fcom_SetEorFile(char* param_EorFile)
{
    snprintf(g_szCpEorFile, sizeof(g_szCpEorFile),
            "%s",
            param_EorFile);
}

static int fstGetLogTime(char *timebuf)
{
    struct timeb t;
    struct tm timeinfo;
    int len;

    ftime(&t);

    localtime_r(&t.time, &timeinfo);

    len = sprintf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d.%03u",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec,
                  t.millitm);

    return len;
}

int fcom_LogInit(char* param_ProcName)
{

    _DAP_LOG_INFO* pstLogInfo;
    pstLogInfo  = &g_stServerInfo.stDapLogInfo;

    memset(pstLogInfo->szCfgLogPath    , 0x00, sizeof(pstLogInfo->szCfgLogPath    ));
    memset(pstLogInfo->szcfgLogFileName, 0x00, sizeof(pstLogInfo->szcfgLogFileName));

    fcom_SetLogInfo(pstLogInfo, param_ProcName);

    if( fcom_LogDirCheck() != RET_SUCC )
    {
        printf("Log Dir Check Fail \n");
        return RET_FAIL;
    }

    return RET_SUCC;
}



int fcom_GetLogSize(FILE* param_fp)
{
    struct stat stFst;

    fstat(fileno(param_fp), &stFst);

    return stFst.st_size;
}

int fcom_MkPath(char* file_path, mode_t mode)
{
    //assert(file_path && *file_path);
    char* p;

    for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/'))
    {
        *p='\0';

        if (mkdir(file_path, mode)==-1)
        {
            if (errno != EEXIST)
            {
                *p='/';
                return -1;
            }
        }
//        chmod(file_path,0755);
        *p='/';
    }
    p = NULL;

    return 0;
}


int fcom_LogDirCheck(void)
{
    int nRet;

    if(access(g_stServerInfo.stDapLogInfo.szCfgLogPath, F_OK) != 0)
    {
        if((nRet = fcom_MkPath(g_stServerInfo.stDapLogInfo.szCfgLogPath, 0755)) != RET_SUCC)
        {
            return RET_FAIL;
        }
    }
    return RET_SUCC;
}

int fcom_SetLogInfo(_DAP_LOG_INFO* param_stDapLogInfo, char* param_ProcName)
{
    struct tm stTm;
    time_t tTime;
    tTime = time(NULL);

    memset(&stTm, 0x00, sizeof(struct tm));
    localtime_r(&tTime,&stTm);

    param_stDapLogInfo->nCfgMaxLogFileSize  = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    param_stDapLogInfo->nCfgDebugLevel      = fcom_GetProfileInt("DEBUG","LEVEL",1);

    /* Debug level is 1:critical,2:major,3:warning,4,info,5:debug */
    param_stDapLogInfo->nCfgDbWriteLevel    = fcom_GetProfileInt("DEBUG","DBWRITE",1);
     fcom_GetProfile("DEBUG","DBWRITE_YN",param_stDapLogInfo->szCfgDbWriteFlag,"N");
    param_stDapLogInfo->nCfgKeepDay         = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);

    /* Set LogFile Name */
    snprintf(param_stDapLogInfo->szcfgLogFileName, sizeof(param_stDapLogInfo->szcfgLogFileName),
             "%s.%04d%02d%02d.log",
             param_ProcName,
             stTm.tm_year+1900,
             stTm.tm_mon+1,
             stTm.tm_mday);

    /* Set Log Home */
    snprintf(param_stDapLogInfo->szCfgLogHome             ,
             sizeof(param_stDapLogInfo->szCfgLogHome)     ,
             "%s/log"                              ,
             g_stServerInfo.stDapComnInfo.szDapHome       );

    /* Set Log Path */
    snprintf(param_stDapLogInfo->szCfgLogPath             ,
             sizeof(param_stDapLogInfo->szCfgLogPath)     ,
             "%s/%s/%04d/%02d/%02d"                ,
             param_stDapLogInfo->szCfgLogHome             ,
             "service"                                    ,
             stTm.tm_year+1900                            ,
             stTm.tm_mon+1                                ,
             stTm.tm_mday                                );

    /* Set Pcif Log Path */
    snprintf(param_stDapLogInfo->szCfgPcifLogPAth         ,
             sizeof(param_stDapLogInfo->szCfgPcifLogPAth) ,
             "%s/%s/%04d/%02d/%02d"                ,
             param_stDapLogInfo->szCfgLogHome             ,
             "pcif"                                       ,
             stTm.tm_year+1900                            ,
             stTm.tm_mon+1                                ,
             stTm.tm_mday                                );

    g_stServerInfo.stDapLogInfo.nCurrDay = stTm.tm_mday;

    return RET_SUCC;
}


int fstSendDBLog(char* param_LogIP, int param_LogLevel, char *contents)
{
    int			local_nRet = 0;
    int			local_nRetryCnt = 0;
    char		strLevel[10] = {0x00,};
    char		currDate[19 +1] = {0x00,};
    time_t		currTime = 0;
    _DAP_DB_SERVERLOG_INFO  local_stServerLog;
    _DAP_QUEUE_BUF          local_stDbLogQBuf;

    memset(&local_stServerLog, 0x00, sizeof(_DAP_DB_SERVERLOG_INFO));
    memset(&local_stDbLogQBuf, 0x00, sizeof(_DAP_QUEUE_BUF));
    memset(currDate, 0x00, sizeof(currDate));
    memset(strLevel, 0x00, sizeof(strLevel));

    switch(param_LogLevel)
    {
        case DAP_CRITICAL:
            strcpy(strLevel, "CRITICAL");
            break;
        case DAP_MAJOR:
            strcpy(strLevel, "MAJOR");
            break;
        case DAP_WARN:
            strcpy(strLevel, "WARNNING");
            break;
        case DAP_INFO:
            strcpy(strLevel, "INFO");
            break;
        case DAP_DEBUG:
            strcpy(strLevel, "DEBUG");
            break;
        default:
            strcpy(strLevel, "UNKNOWN");
            break;
    }

    /** 로그타임 **/
    currTime = time((time_t) 0);
    fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss");
    strcpy(local_stServerLog.logdate, currDate);

    /** PID **/
    local_stServerLog.pid = getpid();

    /** Process **/
    snprintf(local_stServerLog.process, sizeof(local_stServerLog.process),
             "%s", g_stServerInfo.stDapComnInfo.szDebugName);

    /** Log IP **/
    snprintf(local_stServerLog.logip, sizeof(local_stServerLog.logip), "%s", param_LogIP);

    /** Log Level **/
    strcpy(local_stServerLog.loglevel, strLevel);

    /** Log Msg **/
    strcpy(local_stServerLog.logmsg, contents);

    local_stDbLogQBuf.packtype = PROCESS_SERVER_LOG;
    memcpy((void *)&local_stDbLogQBuf.buf, (void *)&local_stServerLog, sizeof(local_stServerLog));

    local_nRet = fipc_FQPutData(DBLOG_QUEUE, (char *)&local_stDbLogQBuf, sizeof(local_stDbLogQBuf));
    if(local_nRet < 0)
    {
        fcom_SleepWait(5);
        local_nRetryCnt = 0;
        while(local_nRetryCnt < 3)
        {
            local_nRet = fipc_FQPutData(DBLOG_QUEUE, (char *)&local_stDbLogQBuf, sizeof(local_stDbLogQBuf));
            if(local_nRet < 0)
            {
                local_nRetryCnt++;
                fcom_SleepWait(5);
            }
            else
            {
                break;
            }
        }
        if(local_nRetryCnt == 3)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in put dblog, retryCnt(%d/3) [%d][%s] ",
                           local_nRetryCnt, errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

void fcom_JsonWrite(int param_LogLevel, char* cpip, const char *fmt, ...)
{
    struct tm stTm;
    time_t tTime;

    char szLogPath[ 256 +1] = {0x00,};
    char szTimeBuf[  24 +1] = {0x00,};
    char* szVarLog = NULL;
    char* szLogBuf = NULL;
    FILE* fp = NULL;
    va_list     args;

    if(fcom_malloc((void**)&szVarLog,sizeof(char) * 8192) != 0)
    {
        return;
    }
    if(fcom_malloc((void**)&szLogBuf,sizeof(char) * 8192) != 0)
    {
        fcom_MallocFree((void**)&szVarLog);
        return;
    }

    memset(&stTm,     0x00, sizeof(struct tm));
    memset(szLogPath, 0x00, sizeof(szLogPath)  );
    memset(szTimeBuf, 0x00, sizeof(szTimeBuf)  );
    memset(szVarLog , 0x00, sizeof(char) * 8192);
    memset(szLogBuf , 0x00, sizeof(char) * 8192);

    tTime = time(NULL);
    localtime_r(&tTime, &stTm);

    if(g_stServerInfo.stDapLogInfo.nCurrDay != stTm.tm_mday)
    {
        fcom_SetLogInfo(&g_stServerInfo.stDapLogInfo, g_stServerInfo.stDapComnInfo.szDebugName);
    }

    fp = fcom_OpenPcifLogFile(g_stServerInfo.stDapLogInfo.szCfgPcifLogPAth,cpip);
    if(fp == NULL)
    {
        perror("fcom_JsonWrite fp Is Null! :");
        fcom_MallocFree((void**)&szVarLog);
        fcom_MallocFree((void**)&szLogBuf);
        return;
    }

    if(access(szLogPath,F_OK) != 0)
    {
        fcom_MkPath(szLogPath,0755);
    }

    if(param_LogLevel != 6)
    {
        if(param_LogLevel > g_stServerInfo.stDapLogInfo.nCfgDebugLevel)
        {
            if(fp != NULL)
            {
                fclose(fp);
            }
            fcom_MallocFree((void**)&szVarLog);
            fcom_MallocFree((void**)&szLogBuf);
            return;
        }

    }

    va_start(args,fmt);
    /* Log Formatting */
    /* [Time][ProcessID:ThreadID][LogLevel][IP][Message]  */
    fstGetLogTime(szTimeBuf);
    vsnprintf(szVarLog, 8192 -1, fmt, args);
    va_end(args);

    snprintf(szLogBuf,
             8192 -1,
             "[%-24.24s][%d:%llu][%8.8s][%s][%.*s]\n",
             szTimeBuf,
             getpid() ,
             (unsigned long long)pthread_self(),
             g_stLogerLevel[--param_LogLevel].LevelMsg,
             cpip,
             (int)strlen(szVarLog),
             szVarLog       );

    if(fp != NULL)
        fprintf(fp,szLogBuf);

    fcom_MallocFree((void**)&szVarLog);
    fcom_MallocFree((void**)&szLogBuf);

    if(fp != NULL)
        fclose(fp);
}




FILE* fcom_OpenPcifLogFile(char* pcifLogPath, char* cpLName)
{
    int		i, rxt = 0;
    int		tokenCnt = 0;
    char    cpFile[128 +1] = {0x00,};
    char	token[3 +1] = {0x00,};


    memset	(cpFile, 0x00, sizeof(cpFile));

    /* log/pcif/yyyy/mm/dd */
    sprintf (cpFile, pcifLogPath);

    tokenCnt = fcom_TokenCnt(cpLName, ".");
    if(tokenCnt > 0) // log/pcif/yyyy/mm/dd/10/20/20/158
    {
        for(i=0; i<=tokenCnt; i++)
        {
            memset(token, 0x00, sizeof(token));
            fcom_ParseStringByToken(cpLName, ".", token, i);
            sprintf(cpFile+strlen(cpFile), "/%s", token);
        }
    }
    else // log/pcif/yyyy/mm/dd/pcif
    {
        sprintf(cpFile+strlen(cpFile), "/%s", cpLName);
    }

    if (access(cpFile, W_OK) != 0) // If not exist
    {
        rxt = fcom_MkPath(cpFile, 0755);
        if (rxt < 0){
        }
        else
        {
            chmod(cpFile, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
        }
    }

    return(fopen(cpFile, "a+, ccs=UTF-8"));
}


void fcom_IpLogWrite(int param_LogLevel, char* cpip, const char *fmt, ...)
{
    struct tm stTm;
    time_t tTime;

    char szLogPath[ 256 +1] = {0x00,};
    char szVarLog [1024 +1] = {0x00,};
    char szLogBuf [2048 +1] = {0x00,};
    char szTimeBuf[  24 +1] = {0x00,};

    FILE* fp = NULL;
    va_list     args;

    memset(szVarLog,  0x00, sizeof(szVarLog));
    memset(szLogPath, 0x00, sizeof(szLogPath));
    memset(szLogBuf,  0x00, sizeof(szLogBuf));
    memset(szTimeBuf, 0x00, sizeof(szTimeBuf));
    memset(&stTm,     0x00, sizeof(struct tm));

    tTime = time(NULL);
    localtime_r(&tTime,&stTm);


    if(g_stServerInfo.stDapLogInfo.nCurrDay != stTm.tm_mday)
    {
        fcom_SetLogInfo(&g_stServerInfo.stDapLogInfo, g_stServerInfo.stDapComnInfo.szDebugName);
    }

    fp = fcom_OpenPcifLogFile(g_stServerInfo.stDapLogInfo.szCfgPcifLogPAth,cpip);
    if(fp == NULL)
    {
        perror("fcom_IpLogWrite Is NULL! :");
        return;
    }

    if(access(szLogPath,F_OK) != 0)
    {
        fcom_MkPath(szLogPath,0755);
    }

    if(param_LogLevel != 6)
    {
        if(param_LogLevel > g_stServerInfo.stDapLogInfo.nCfgDebugLevel)
        {
            if(fp != NULL)
            {
                fclose(fp);
            }
            return;
        }

    }

    va_start(args,fmt);
    /* Log Formatting */
    /* [Time][ProcessID:ThreadID][LogLevel][IP][Message]  */
    fstGetLogTime(szTimeBuf);
    vsnprintf(szVarLog, sizeof(szVarLog) -1, fmt, args);
    va_end(args);

    snprintf(szLogBuf, sizeof(szLogBuf) -1,
             "[%-24.24s][%d:%llu][%8.8s][%s][%.*s]",
             szTimeBuf,
             getpid() ,
             (unsigned long long)pthread_self(),
             g_stLogerLevel[--param_LogLevel].LevelMsg,
             cpip,
             (int)strlen(szVarLog),
             szVarLog       );

    if(fp != NULL)
    {
        fprintf(fp,"%s\n",szLogBuf);
        fclose(fp);
    }

}

int fcom_AgentDbLogWrite( char* param_HbSq,
                          char* param_UserKey,
                          char* param_UserIp,
                          char* param_Process,
                          char* param_LogDate,
                          char* param_LogLevel,
                          char* param_LogMsg )
{
    int			local_nRet = 0;
    int			local_nRetryCnt = 0;
    _DAP_DB_AGENTLOG_INFO   local_stAgentLog;
    _DAP_QUEUE_BUF          local_stDbLogQBuf;

    /** dap_dblog 프로세스에 전송 **/
    memset(&local_stAgentLog, 0x00, sizeof(local_stAgentLog)  );
    memset(&local_stDbLogQBuf, 0x00, sizeof(local_stDbLogQBuf));

    /** HB_SQ **/
    snprintf(local_stAgentLog.hbsq, sizeof(local_stAgentLog.hbsq), "%s", param_HbSq);

    /** User Key **/
    snprintf(local_stAgentLog.hbunq, sizeof(local_stAgentLog.hbunq), "%s", param_UserKey);

    /** User IP **/
    snprintf(local_stAgentLog.ip, sizeof(local_stAgentLog.ip), "%s", param_UserIp);

    /** Process **/
    snprintf(local_stAgentLog.process, sizeof(local_stAgentLog.process), "%s", param_Process);

    /** Log Date **/
    snprintf(local_stAgentLog.logdate, sizeof(local_stAgentLog.logdate), "%s", param_LogDate);

    /** Log Level **/
    snprintf(local_stAgentLog.loglevel, sizeof(local_stAgentLog.loglevel), "%s", param_LogLevel);

    /** Log Msg **/
    snprintf(local_stAgentLog.logmsg, sizeof(local_stAgentLog.logmsg), "%s", param_LogMsg);

    local_stDbLogQBuf.packtype = PROCESS_AGENT_LOG;
    memcpy((void *)&local_stDbLogQBuf.buf, (void *)&local_stAgentLog, sizeof(local_stAgentLog));

    local_nRet = fipc_FQPutData(DBLOG_QUEUE, (char *)&local_stDbLogQBuf, sizeof(local_stDbLogQBuf));
    if(local_nRet < 0)
    {
        fcom_SleepWait(5);
        local_nRetryCnt = 0;
        while(local_nRetryCnt < 3)
        {
            local_nRet = fipc_FQPutData(DBLOG_QUEUE, (char *)&local_stDbLogQBuf, sizeof(local_stDbLogQBuf));
            if(local_nRet < 0)
            {
                local_nRetryCnt++;
                fcom_SleepWait(5);
            }
            else
            {
                break;
            }
        }
        if(local_nRetryCnt == 3)
        {
            WRITE_CRITICAL(CATEGORY_IPC,"Fail in put dblog, retryCnt(%d/3) [%d][%s] ",
                           local_nRetryCnt, errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

void fcom_DbLogWrite(int param_LogLevel, char* param_LogIp, int param_CatLevel, const char *fmt, ...)
{
    char szVarLog [1023+1] = {0x00,};
    char szLogBuf [4095+1] = {0x00,};

    va_list     args;

    memset(szVarLog , 0x00, sizeof(szVarLog ));
    memset(szLogBuf , 0x00, sizeof(szLogBuf ));

    if ( g_stServerInfo.stDapLogInfo.szCfgDbWriteFlag[0] != 'y' &&
         g_stServerInfo.stDapLogInfo.szCfgDbWriteFlag[0] != 'Y')
    {
        return;
    }

    if(param_LogLevel > g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel)
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"DBLOG Cfg Level, Log Level Return");
        return;
    }

    va_start(args,fmt);

    /* Log Formatting */
    vsnprintf(szVarLog, sizeof(szVarLog) -1, fmt, args);

    va_end(args);

    snprintf(szLogBuf, sizeof(szLogBuf) - 1,
             "[%.*s]",
             (int)strlen(szVarLog),
             szVarLog       );

    WRITE_DEBUG(CATEGORY_DEBUG,"DBLOG Send : [%s]", szLogBuf);

    /** dap_dblog 프로세스에 전송 **/
    fstSendDBLog(param_LogIp, param_LogLevel, szLogBuf);

}

void fcom_LogWrite(int param_LogLevel, int param_CatLevel, const char *fmt, ...)
{
    char szLogPath[ 256 +1] = {0x00,};
    char szVarLog [1024 +1] = {0x00,};
    char szLogBuf [2048 +1] = {0x00,};
    char szTimeBuf[  24 +1] = {0x00,};
    va_list     args;
    FILE* fp = NULL;
    struct tm stTm;
    time_t tTime;

    /***************************************
    # Local Init.
    ****************************************/
    memset(szVarLog , 0x00, sizeof(szVarLog ));
    memset(szLogPath, 0x00, sizeof(szLogPath));
    memset(szLogBuf , 0x00, sizeof(szLogBuf ));
    memset(szTimeBuf, 0x00, sizeof(szTimeBuf));
    memset(&stTm, 0x00, sizeof(stTm));

    tTime = time(NULL);
    localtime_r(&tTime, &stTm);

    if(g_stServerInfo.stDapLogInfo.nCurrDay != stTm.tm_mday)
    {
        fcom_SetLogInfo(&g_stServerInfo.stDapLogInfo, g_stServerInfo.stDapComnInfo.szDebugName);
    }

    snprintf(szLogPath, sizeof(szLogPath) -1, "%s/%s", g_stServerInfo.stDapLogInfo.szCfgLogPath,
                                                              g_stServerInfo.stDapLogInfo.szcfgLogFileName);

    if(access(szLogPath, F_OK) != 0)
    {
        fcom_MkPath(szLogPath,0755);
    }

    /* Debug level is 1:critical,2:major,3:warning,4,info,5:debug 6:all */
    if(param_LogLevel != 6)
    {
        if(param_LogLevel > g_stServerInfo.stDapLogInfo.nCfgDebugLevel)
        {
            return;
        }

    }

    fp = fopen(szLogPath,"a, ccs=UTF-8");
    if(fp == NULL)
    {
        perror("fcom_LogWrite Is NULL! :");
        return;
    }

    va_start(args,fmt);
    /* Log Formatting */
    /* [Time][ProcessID:ThreadID][LogLevel][Category][Message]  */
    fstGetLogTime(szTimeBuf);
    vsnprintf(szVarLog, sizeof(szVarLog) -1, fmt, args);
    va_end(args);

    snprintf(szLogBuf, sizeof(szLogBuf) -1,
            "[%-24.24s][%d:%lu][%-8.8s][%s][%.*s]",
            szTimeBuf,
            (int)getpid() ,
            (unsigned long)pthread_self(),
            g_stLogerLevel[--param_LogLevel].LevelMsg,
            g_stLoggerCategory[--param_CatLevel].CategoryMsg,
            (int)strlen(szVarLog),
            szVarLog       );

    if(fp != NULL)
    {
        fprintf(fp,"%s\n",szLogBuf);
        fclose(fp);
    }


}

static FILE	* fstOpenEorFile(int type, char* cpLFile)
{
    char    cpFile[64] = {0x00,};
    char    cpDate[20] = {0x00,};
    time_t  tTime;
    struct 	tm  stTm;

    memset(cpFile,'\0',sizeof(cpFile));
    memset(&stTm, 0x00, sizeof(struct tm));

    tTime = time((time_t *)NULL);
    localtime_r(&tTime,&stTm);

    if(type == 0)
    {
    }
    else if(type == 1) //MONTH
    {
        sprintf(cpDate,"%04d%02d",stTm.tm_year+1900,stTm.tm_mon+1);
    }
    else //day
    {
        sprintf(cpDate,"%04d%02d%02d",stTm.tm_year+1900,stTm.tm_mon+1,stTm.tm_mday);
    }

    if(type == 0) //Fixed
    {
        sprintf(cpFile, "%s", cpLFile);
    }
    else
    {
        sprintf(cpFile, "%s.%s", cpLFile, cpDate);
    }

    return(fopen(cpFile, "a+, ccs=UTF-8"));
}

void fcom_EorRet(int etype, const char *fmt, ...)
{
    va_list		args;
    FILE		*Logfp;
    //int			lock, release;

    va_start(args,fmt);
    if((Logfp = fstOpenEorFile(etype, g_szCpEorFile)) == (FILE *)NULL)
    {
        fcom_LogTime(stderr);
        vfprintf(stderr, fmt, args);
        fputc('\n', stderr);
        fflush(stderr);
    }
    else
    {
        fcom_LogTime(Logfp);
        vfprintf(Logfp, fmt, args);
        fclose(Logfp);
    }

    va_end(args);
}
