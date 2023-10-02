//
// Created by KimByoungGook on 2020-06-24.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 500
#endif
#include <string.h>

#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"

#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"

#include "report.h"
#include "dap_version.h"

static int fstMainTask();
static int fstReportInit();
static void fstHandleSignal(int sid);
static int fstCustSendMail(int bType, char *p_file, char *p_from, char *p_to);
static int fstSendMail(char *repFullName, int bType);
static int fstReloadConfig(void);
static int fstCheckNotiFile(void);
static void fstSigHandler();
static void fstReloadCfgFile( );


int main(int argc, char** argv)
{
    int nRet;

    if(argc < 2)
    {
        DAP_VERSION_MACRO
        exit(0);
    }

    fcom_ArgParse(argv);

    fstReportInit();

    fcom_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    nRet = fdb_ConnectDB();
    if(nRet < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connection " );
        exit(0);
    }
    else
    {
        WRITE_INFO(CATEGORY_INFO,"Succeed in db connection " );
        g_stProcReportInfo.nFlagDbConnected = 1;
    }

    fdb_LsystemInit();

    fstSigHandler();

    fstMainTask();

    return RET_SUCC;
}
static void fstSigHandler()
{
    signal( SIGHUP, fstHandleSignal);    /* 1 : hangup */
    signal( SIGINT, fstHandleSignal);    /* 2 : interrupt (rubout) */
    signal( SIGQUIT, fstHandleSignal);   /* 3 : quit (ASCII FS) */
    signal( SIGILL, fstHandleSignal);    /* 4 : illegal instruction(not reset when caught) */
    signal( SIGTRAP, fstHandleSignal);   /* 5 : trace trap (not reset when caught) */
    signal( SIGIOT, fstHandleSignal);    /* 6 : IOT instruction */
    signal( SIGABRT, fstHandleSignal);   /* 6 : used by abort,replace SIGIOT in the future */
//    signal( SIGEMT, fstHandleSignal);    /* 7 : EMT instruction */
    signal( SIGFPE, fstHandleSignal);    /* 8 : floating point exception */
    signal( SIGKILL , fstHandleSignal);  /* 9 : kill (cannot be caught or ignored) */
    signal( SIGBUS , fstHandleSignal);   /* 10: bus error */
    signal( SIGSEGV, fstHandleSignal);   /* 11: segmentation violation */
    signal( SIGSYS , fstHandleSignal);   /* 12: bad argument to system call */
//    signal( SIGPIPE, fstHandleSignal);   /* 13: write on a pipe with no one to read it */
//    sigignore( SIGPIPE );              /* 13: write on a pipe with no one to read it */
    signal( SIGPIPE, SIG_IGN );              /* 13: write on a pipe with no one to read it */

    signal( SIGALRM, fstHandleSignal);   /* 14: alarm clock */
    signal( SIGTERM, fstHandleSignal);   /* 15: software termination signal from kill */
    signal( SIGUSR1, fstHandleSignal);   /* 16: user defined signal 1 */
    signal( SIGUSR2, fstHandleSignal);   /* 17: user defined signal 2 */
//    sigignore( SIGCLD );              /* 18: child status change */
    signal( SIGCLD, SIG_IGN );              /* 18: child status change */
//    sigignore( SIGCHLD );             /* 18: child status change alias (POSIX) */
    signal(SIGCHLD, SIG_IGN  );             /* 18: child status change alias (POSIX) */
    signal( SIGPWR , fstHandleSignal);   /* 19: power-fail restart */
//    sigignore( SIGWINCH );            /* 20: window size change */
    signal( SIGWINCH, SIG_IGN );            /* 20: window size change */
    signal( SIGURG  , fstHandleSignal);  /* 21: urgent socket condition */
    signal( SIGPOLL , fstHandleSignal);  /* 22: pollable event occured */
    signal( SIGIO   , fstHandleSignal);  /* 22: socket I/O possible (SIGPOLL alias) */
    signal( SIGSTOP , fstHandleSignal);  /* 23: stop (cannot be caught or ignored) */
    signal( SIGTSTP , fstHandleSignal);  /* 24: user stop requested from tty */
    signal( SIGCONT , fstHandleSignal);  /* 25: stopped process has been continued */
    signal( SIGTTIN , fstHandleSignal);  /* 26: background tty read attempted */
    signal( SIGTTOU , fstHandleSignal);  /* 27: background tty write attempted */
    signal( SIGVTALRM , fstHandleSignal);/* 28: virtual timer expired */
    signal( SIGPROF , fstHandleSignal);  /* 29: profiling timer expired */
    signal( SIGXCPU , fstHandleSignal);  /* 30: exceeded cpu limit */
    signal( SIGXFSZ , fstHandleSignal);  /* 31: exceeded file size limit */

}


static int fstMainTask()
{
    char    sessionTimeStr[14+1];
    char    prmonTimeStr[14+1];
    char    currTimeStr[14+1];
    char    currHourMinStr[6+1];
    char    currDateStr[10+1];
    char    currDayStr[2+1];
    char    prefix[6+1];
    char    postfix[4+1];
    char	histTableName[50];
    char	repFullPath[256];

    int     local_loop_count = 0;
    int     bFirstLoad = 1;
    int     rxt;
    int     nThrVal;
    int     nCnt;
    int     weekNum;
    int     nCurrMin;

    pthread_t pThreadRecv_t;
    time_t  currTime;
    time_t	sessionTime;
    time_t	prmonTime;
    char    local_szPidPath[127 +1]    = {0x00,};
    char*   local_ptrProcessName = NULL;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_REPORT);

    /** Check Master Pid File **/
    if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }

    /** Make Master Pid File **/
    fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    WRITE_INFO(CATEGORY_INFO,"Start Program " );

    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_INFO, "Start Program");

    sessionTime = time((time_t) 0) - g_stProcReportInfo.nCfgKeepSession;
    fcom_time2str(sessionTime, sessionTimeStr, "YYYYMMDDhhmmss\0");
    WRITE_INFO(CATEGORY_INFO,"cfgKeepSession(%d) sessionTimeStr(%s) ",
               g_stProcReportInfo.nCfgKeepSession,
               sessionTimeStr);

    prmonTime = time((time_t) 0) - g_stProcReportInfo.nCfgKeepSession;
    fcom_time2str(prmonTime, prmonTimeStr, "YYYYMMDDhhmmss\0");
    WRITE_INFO(CATEGORY_INFO,"cfgKeepSession(%d) prmonTimeStr(%s) ",
               g_stProcReportInfo.nCfgKeepSession,
               sessionTimeStr);

    pthread_mutex_init(&db_mutex, NULL);

    for(nCnt = 0; nCnt < MAX_THREAD; nCnt++)
    {
        nThrVal = nCnt ;
        WRITE_INFO(CATEGORY_DEBUG,"idx : %d main start",nThrVal);
        if(fcom_ThreadCreate( (void *)&pThreadRecv_t, fstThreadRecv, (void *)&nThrVal, 4 * 1024 * 1024) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in create thread" );
            return -1;
        }
    }

    while(1)
    {
        g_stProcReportInfo.nIngJobTime =  time(NULL);
        nCurrMin = fcom_GetSysMinute();

        if(bFirstLoad || (g_stProcReportInfo.nIngJobTime % g_stProcReportInfo.nCfgReportCheckNotiInterval) == 0)
        {
            bFirstLoad = 0;
            //check noti
            fstReloadConfig();
            while(1)
            {
                if(pthread_mutex_trylock(&db_mutex) == 0)
                {
                    fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);
                    pthread_mutex_unlock(&db_mutex);
                    break;
                }
                else
                {
                    fcom_SleepWait(5); //0.1
                    local_loop_count++;
                }
                if( local_loop_count >=  300 ) { WRITE_DEBUG(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid() );  break; } //30sec
            }
            fstCheckNotiFile();
        }

        if((nCurrMin % 3) == 0)
        {
            /*  Is Config File Changed */
            if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile, &g_stProcReportInfo.nConfigLastModify) == 1)
                fstReloadCfgFile();
        }

        currTime = time((time_t) 0);

        fcom_time2str(currTime, currTimeStr, "YYYYMMDDhhmmss\0");
        fcom_time2str(currTime, currHourMinStr, "hhmmss\0");
        fcom_time2str(currTime, currDateStr, "YYYY-MM-DD\0");
        fcom_time2str(currTime, currDayStr, "DD\0");

        if(strcmp(currTimeStr, sessionTimeStr) > 0)
        {
            sessionTime = time((time_t) 0) + g_stProcReportInfo.nCfgKeepSession;
            fcom_time2str(sessionTime, sessionTimeStr, "YYYYMMDDhhmmss\0");
            fipc_KeepSession(g_stProcReportInfo.nCfgKeepSession);
        }

        if(strcmp(currTimeStr, prmonTimeStr) > 0)
        {
            prmonTime = time((time_t) 0) + g_stProcReportInfo.nCfgPrmonInterval;
            fcom_time2str(prmonTime, prmonTimeStr, "YYYYMMDDhhmmss\0");
            fipc_PrmonLinker(g_stServerInfo.stDapComnInfo.szDebugName, 0, &g_stProcReportInfo.nLastSendTime);
        }

        if( (g_stProcReportInfo.nCfgPeriodicReport == 1) &&
        ( !strncmp(currHourMinStr, g_stProcReportInfo.szConfReportRunTime, 6)) )
        {
            memset(prefix, 0x00, sizeof(prefix));
            memset(postfix, 0x00, sizeof(postfix));
            fdb_GetHistoryDbTime(prefix, -1); //어제자 통계를 내므로, 어제날짜를가져옴
            fdb_GetHistoryTableTime(postfix, NULL,-1);

            WRITE_INFO(CATEGORY_INFO,"prefix(%s) postfix(%s) ",prefix, postfix);

            WRITE_INFO(CATEGORY_INFO, "Run collect day-report");
            memset(histTableName, 0x00, sizeof(histTableName) );

            // Get EVENT_HISTORY_MM Table Name
            fdb_GetHistoryTableName("EVENT_TB", histTableName, prefix, postfix);

// AS-IS , OLD
//            freport_TaskReport_Old(prefix, postfix);

//            freport_MergeReportDayBysq(histTableName, g_stProcReportInfo.nCfgReportStatsLimit, prefix, postfix);

            // 이력조회

            // TEST
//            snprintf(prefix, sizeof(prefix), "%s","2021"); //YEAR
//            snprintf(postfix, sizeof(postfix), "%s", "07"); //MONTH

            freport_TaskReport(prefix, postfix);

            WRITE_INFO(CATEGORY_DB,"Run collect day-report, histTableName(%s) ",histTableName );
            WRITE_INFO(CATEGORY_DB,"Run make day-report template, path(%s) ",
                    g_stProcReportInfo.szCfgReportPath);

            memset(repFullPath, 0x00, sizeof(repFullPath));

            rxt = freport_MakeReportTemplate(	g_stProcReportInfo.szCfgReportPath,
                                           1,
                                           0,
                                           "",
                                           "",
                                           0,
                                           "",
                                           repFullPath, 0x00);
            WRITE_INFO(CATEGORY_INFO, "Run make day-report template, path(%s)",
                    g_stProcReportInfo.szCfgReportPath);

            if(rxt == 0 && strlen(g_stProcReportInfo.szConfMailToDay) > 5)
            {
                fstSendMail(repFullPath, 1);
            }
            else
            {

                WRITE_INFO(CATEGORY_INFO, "Cancel sending mail for day, rxt(%d)mailto(%s) ",
                        rxt,
                        g_stProcReportInfo.szConfMailToDay);
            }

            //매주 월요일이면 -> 매일 돌리는것으로 변경
            WRITE_INFO(CATEGORY_INFO, "Run collect week-report " );

            freport_MergeReportWeekBysq(g_stProcReportInfo.nCfgReportStatsLimit, prefix, postfix);

            WRITE_INFO(CATEGORY_INFO, "Run collect week-report " );
            weekNum = fcom_GetDayOfWeek(currDateStr); //0:SUN,1:MON,2:TUE..
            if(weekNum == 1) //월요일
            {
                WRITE_INFO(CATEGORY_INFO, "Run make week-report template, path(%s)",
                        g_stProcReportInfo.szCfgReportPath);
                memset(repFullPath, 0x00, sizeof(repFullPath));

                rxt = freport_MakeReportTemplate(g_stProcReportInfo.szCfgReportPath,
                                               2,
                                               0,
                                               "",
                                               "",
                                               0,
                                               "",
                                               repFullPath, 0x00);
                WRITE_INFO(CATEGORY_INFO,"Run make week-report template, path(%s)",
                        g_stProcReportInfo.szCfgReportPath);
                if(rxt == 0 && strlen(g_stProcReportInfo.szConfMailToWeek) > 5)
                {
                    fstSendMail(repFullPath, 2);
                }
                else
                {

                    WRITE_INFO(CATEGORY_INFO, "Cancel sending mail for week, rxt(%d)mailto(%s)",
                            rxt,
                            g_stProcReportInfo.szConfMailToWeek);
                }
            }

            //매월 1일이면 -> 매일 돌리는것으로 변경
            WRITE_INFO(CATEGORY_INFO, "Run collect month-report " );

            freport_MergeReportMonthBysq(g_stProcReportInfo.nCfgReportStatsLimit, prefix, postfix);

            WRITE_INFO(CATEGORY_INFO, "Run collect month-report " );
            if(!strncmp(currDayStr, "01", 2))
            {
                WRITE_INFO(CATEGORY_INFO, "Run make month-report template, path(%s) ",
                        g_stProcReportInfo.szCfgReportPath);
                memset(repFullPath, 0x00, sizeof(repFullPath));

                rxt = freport_MakeReportTemplate(g_stProcReportInfo.szCfgReportPath,
                                               3,
                                               0,
                                               "",
                                               "",
                                               0,
                                               "",
                                               repFullPath, 0x00);
                WRITE_INFO(CATEGORY_INFO, "Run make month-report template, path(%s)",
                        g_stProcReportInfo.szCfgReportPath);

                if(rxt == 0 && strlen(g_stProcReportInfo.szConfMailToMonth) > 5)
                {
                    fstSendMail(repFullPath, 3);
                }
                else
                {

                    WRITE_INFO(CATEGORY_INFO, "Cancel sending mail for month, rxt(%d)mailto(%s)",
                            rxt,
                            g_stProcReportInfo.szConfMailToMonth);
                }
            }
        }


        fcom_Msleep(g_stProcReportInfo.nCfgReportInterval * 1000);

    }

    if(g_pstNotiMaster != NULL)
        free(g_pstNotiMaster);

    fFile_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_report");


    return RET_SUCC;

}

static int fstReloadConfig(void)
{
    struct stat     statBuf;
    _DAP_PROC_REPORT_INFO* pstReportInfo;

    pstReportInfo = &g_stProcReportInfo;

    if(g_pstNotiMaster[REPORT_CHANGE].reload)
    {
        //정규보고서 처리시간 가져오기
        freport_GetReportRuntime(g_stProcReportInfo.szConfReportRunTime);
        if(strlen(g_stProcReportInfo.szConfReportRunTime) < 4)
            strcpy(g_stProcReportInfo.szConfReportRunTime, "010000");
        else
            strcat(g_stProcReportInfo.szConfReportRunTime, "00");

        //정규보고서 수발신설정 가져오기
        freport_GetReportMailInfo(pstReportInfo->szConfMailFrom,
                              pstReportInfo->szConfMailToDay,
                              pstReportInfo->szConfMailToWeek,
                              pstReportInfo->szConfMailToMonth);


        stat(g_pstNotiMaster[REPORT_CHANGE].szNotiFileName, &statBuf);
        g_pstNotiMaster[REPORT_CHANGE].lastModify   =   statBuf.st_mtime;
        g_pstNotiMaster[REPORT_CHANGE].reload       =   FALSE;
    }

    return TRUE;
}

void* fstThreadRecv(void* thrid)
{
    int         local_nStartYear  = 0, local_nEndYear = 0;
    int         local_nStartMonth = 0, local_nEndMonth = 0;
    int         local_nStartDay = 0, local_nEndDay = 0;
    int         rxt;
    int         thrIdx;

    int         local_loop_count = 0;
    int			isCustom = 0;
    char		repFullPath[256];
    char        local_szStartYYYY[4 +1] = {0x00,};
    char        local_szStartMM[2 +1]   = {0x00,};
    char        local_szStartDD[2 +1]   = {0x00,};
    char        local_szEndYYYY[4 +1] = {0x00,};
    char        local_szEndMM[2 +1]   = {0x00,};
    char        local_szEndDD[2 +1]   = {0x00,};
    char        local_szYYYY[4  +1] = {0x00,};
    char        local_szMM[2 +1] = {0x00,};
    char        local_szDD[2 +1] = {0x00,};
    char        local_szTimeBuff[32] = {0x00,};
    _DAP_REPORT_INFO RI;

    thrIdx = *(int*)thrid;

    /* 스레드 자원 해제 */
    /* 0x00 : 프로세스 초기값 */
    /* 0x10 : 프로세스 사용중 */
    /* 0x20 : 프로세스 종료상태 */

    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Start",thrIdx);

    while (1)
    {
        if(g_pthreadStopFlag != 0x00)
            break;

        memset((void *)&RI,0x00,sizeof(RI));
        rxt = fipc_FQGetData(REPORT_QUEUE, (char *)&RI, sizeof(RI));

        if(rxt == 0)
        {
            sleep(1);
            continue;
        }

        if (rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in receive, rxt(%d)",rxt );
            sleep(1);
            continue;
        }
        else
        {
            WRITE_INFO(CATEGORY_INFO,"Succeed in receive, rxt(%d) ", rxt );
        }
        WRITE_INFO(CATEGORY_INFO, "manager_id: %s", RI.manager_id);
        WRITE_INFO(CATEGORY_INFO, "begin_date: %s", RI.begin_date); //2022-02-01
        WRITE_INFO(CATEGORY_INFO, "end_date: %s",   RI.end_date);   //2022-02-04
        WRITE_INFO(CATEGORY_INFO, "group_sq: %lu",  RI.group_sq);
        WRITE_INFO(CATEGORY_INFO, "view_type: %d",  RI.view_type);
        WRITE_INFO(CATEGORY_INFO, "mail_type: %s",  RI.mail_type);
        WRITE_INFO(CATEGORY_INFO, "mail_lang: %s",  RI.mail_lang);
        WRITE_INFO(CATEGORY_INFO, "mail_from: %s",  RI.mail_from);
        WRITE_INFO(CATEGORY_INFO, "mail_to: %s",    RI.mail_to);

        //cusom lang 설정
        fcom_SetCustLang(RI.mail_lang);

        if(!strcmp(RI.mail_type, "today") && strlen(RI.mail_to) > 5)
        {
            if (RI.view_type == 0 || RI.view_type == 2)
            {
                WRITE_INFO(CATEGORY_INFO,"Run today send mail " );
                fstCustSendMail( 0, "", RI.mail_from, RI.mail_to);
            }
        }
        else
        {
            WRITE_INFO(CATEGORY_INFO,"Run make report template, path(%s) ",g_stProcReportInfo.szCfgReportPath );

            if(!strncmp(RI.mail_type, "day", 3)) //24time or daily
                isCustom = 1;
            else if(!strncmp(RI.mail_type, "week", 4)) //range + week
                isCustom = 2;
            else //range + month
                isCustom = 3;

            memcpy(local_szStartYYYY, &(RI.begin_date[0]), 4);
            memcpy(local_szStartMM, &(RI.begin_date[5]),  2);
            memcpy(local_szStartDD, &(RI.begin_date[8]),  2);
            if ( local_szStartYYYY[0] != 0x00 )
            {
                local_nStartYear = atoi(local_szStartYYYY);
            }
            if ( local_szStartMM[0] != 0x00 )
            {
                local_nStartMonth = atoi(local_szStartMM);
            }
            if ( local_szStartDD[0] != 0x00 )
            {
                local_nStartDay = atoi(local_szStartDD);
            }

            memcpy(local_szEndYYYY, &(RI.end_date[0]), 4);
            memcpy(local_szEndMM, &(RI.end_date[5]),   2);
            memcpy(local_szEndDD, &(RI.end_date[8]),   2);
            if ( local_szEndYYYY[0] != 0x00 )
            {
                local_nEndYear = atoi(local_szEndYYYY);
            }
            if ( local_szEndMM[0] != 0x00 )
            {
                local_nEndMonth = atoi(local_szEndMM);
            }
            if ( local_szEndDD[0] != 0x00 )
            {
                local_nEndDay = atoi(local_szEndDD);
            }

            int local_nDiffDay = 0;
            local_nDiffDay = fcom_DiffDay(local_nStartYear, local_nStartMonth, local_nStartDay,
                                          local_nEndYear, local_nEndMonth, local_nEndDay);

            int i = 0;
            for ( i = 1; i <= local_nDiffDay+1; i++ )
            {
                memset( local_szTimeBuff, 0x00, sizeof(local_szTimeBuff) );
                fcom_GetAgoTime(i, local_szTimeBuff);
                memset( local_szYYYY, 0x00, sizeof(local_szYYYY) );
                memset( local_szMM, 0x00, sizeof(local_szMM) );
                memset( local_szDD, 0x00, sizeof(local_szDD) );
                memcpy ( local_szYYYY, &local_szTimeBuff[0], 4);
                memcpy ( local_szMM, &local_szTimeBuff[4], 2);
                memcpy ( local_szDD, &local_szTimeBuff[6], 2);

                WRITE_DEBUG(CATEGORY_DEBUG,"RealTime Report YYYY : [%s] MM : [%s] DD : [%s]",
                                    local_szYYYY, local_szMM, local_szDD );
                        //실시간 보고서 데이터 통계 처리

                while(1)
                {
                    if(pthread_mutex_trylock(&db_mutex) == 0)
                    {
                        freport_TaskReportRealTime(local_szYYYY, local_szMM, local_szDD);
                        pthread_mutex_unlock(&db_mutex);
                        break;
                    }
                    else
                    {
                        fcom_SleepWait(5); //0.1
                        local_loop_count++;
                    }
                    if( local_loop_count >=  300 ) { WRITE_DEBUG(CATEGORY_DEBUG,"PID[%d] Mutex loop break ",getpid() );  break; } //30sec
                }


            }

            memset(repFullPath, 0x00, sizeof(repFullPath));
            rxt = freport_MakeReportTemplate(g_stProcReportInfo.szCfgReportPath,
                                           0,
                                           isCustom,
                                           RI.begin_date,
                                           RI.end_date,
                                           RI.group_sq,
                                           RI.manager_id,
                                           repFullPath, 0x01);
            WRITE_INFO(CATEGORY_INFO, "Run make report template, path(%s) ", g_stProcReportInfo.szCfgReportPath);
            if(rxt == 0 && strlen(RI.mail_to) > 5)
            {
                if (RI.view_type == 0 || RI.view_type == 2)
                {
                    WRITE_INFO(CATEGORY_INFO,"Run today send mail " );
                    fstCustSendMail(isCustom+3, repFullPath, RI.mail_from, RI.mail_to);

                }
            }
            else
            {
                WRITE_INFO(CATEGORY_INFO, "Cancel sending mail, rxt(%d)mailto(%s)",
                        rxt,
                        RI.mail_to );
            }
        }
    }


    WRITE_INFO(CATEGORY_INFO,"Thread Idx %d Stop ",thrIdx);

    pthread_exit(NULL);



}


static int fstReportInit()
{
    int i = 0;
    char szCfgTmp[255 +1] = {0x00,};
    char local_szDefaultIp[15 +1] = {0x00,};
    _DAP_COMN_INFO* pstComnInfo;
    _DAP_QUEUE_INFO* pstQueueInfo;

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    memset(pstComnInfo->szComConfigFile, 0x00, sizeof(pstComnInfo->szComConfigFile));

    snprintf(pstComnInfo->szDapHome                ,
             sizeof(pstComnInfo->szDapHome        ),
             getenv("DAP_HOME")             );

    snprintf(pstComnInfo->szComConfigFile          ,
             sizeof(pstComnInfo->szComConfigFile)  ,
             "%s%s"                                ,
             pstComnInfo->szDapHome                ,
             "/config/dap.cfg"                    );


    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        printf("Init Config Fail |%s\n",__func__ );
        return -1;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    g_stProcReportInfo.nCurTime =
    g_stProcReportInfo.nLastJobTime =
    time(NULL);


    /* Load Config */
    pstComnInfo->nCfgMgwId = fcom_GetProfileInt("COMMON","SERVER_ID", 1);

    fsock_GetNic(szCfgTmp);
    fsock_GetIpAddress(szCfgTmp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );

    g_stProcReportInfo.nCfgPeriodicReport = fcom_GetProfileInt("REPORT","PERIODIC_REPORT", 0);
    g_stProcReportInfo.nCfgReportStatsLimit = fcom_GetProfileInt("REPORT","STATS_ARCHIVE_PERIOD", 12);
    g_stProcReportInfo.nCfgReportCheckNotiInterval = fcom_GetProfileInt("REPORT","CHECK_NOTI_INTERVAL", 10);
    g_stProcReportInfo.nCfgReportInterval = fcom_GetProfileInt("REPORT","CYCLE", 1);
    g_stProcReportInfo.nCfgKeepSession = fcom_GetProfileInt("MYSQL","KEEP_SESSION", 5);
    g_stProcReportInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 600);

    fcom_GetProfile("REPORT","WORK_PATH",g_stProcReportInfo.szCfgReportPath,"/home/intent/dap/script/report");

    if(fcom_malloc((void**)&g_pstNotiMaster,MAX_REPORT_NOTI * sizeof(_CONFIG_NOTI) ) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    snprintf(szCfgTmp, sizeof(szCfgTmp), "%s/config/report_change",pstComnInfo->szDapHome);
    fcom_GetProfile("COMMON","REPORT_CHANGE",g_pstNotiMaster->szNotiFileName,szCfgTmp);

    for(i = 0; i < MAX_REPORT_NOTI; i++)
    {
        g_pstNotiMaster[i].lastModify    =   0;
        g_pstNotiMaster[i].reload        =   TRUE;
    }

    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome),
             "%s/.DAPQ",
             pstComnInfo->szDapHome);

    if (access(pstQueueInfo->szDAPQueueHome,R_OK) != 0)
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            printf("Fail in make queue directory(%s) | %s\n",pstQueueInfo->szDAPQueueHome,__func__ );
            exit(0);
        }
    }

    snprintf(pstQueueInfo->szPrmonQueueHome, sizeof(pstQueueInfo->szPrmonQueueHome),
            "%s/%s",
            pstQueueInfo->szDAPQueueHome,
            "PRMONQ");


    if (fipc_FQPutInit(PRMON_QUEUE, pstQueueInfo->szPrmonQueueHome) < 0)
    {
        printf("Fail in init, path(%s) |%s \n",
                pstQueueInfo->szPrmonQueueHome,
                __func__);
        exit(0);
    }
    snprintf(pstQueueInfo->szDblogQueueHome, sizeof(pstQueueInfo->szDblogQueueHome),
             "%s/DBLOGQ",
            pstQueueInfo->szDAPQueueHome);
    if (fipc_FQPutInit(DBLOG_QUEUE, pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in init, path(%s)|%s\n", pstQueueInfo->szDblogQueueHome,__func__);
        exit(0);
    }

    snprintf(pstQueueInfo->szReportQueueHome,
            sizeof(pstQueueInfo->szReportQueueHome),
            "%s/REPORTQ",
            pstQueueInfo->szDAPQueueHome);

    if (fipc_FQGetInit(REPORT_QUEUE, pstQueueInfo->szReportQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)|%s\n",pstQueueInfo->szReportQueueHome, __func__ );
        exit(0);
    }

//    /* 프로세스.pid 디렉토리 검사 */
//    sprintf(g_stServerInfo.stDapComnInfo.szPidPath,"%s%s",g_stServerInfo.stDapComnInfo.szDapHome,"/bin/var");
//    if(access(g_stServerInfo.stDapComnInfo.szPidPath, W_OK) != 0 )
//    {
//        if (mkdir(g_stServerInfo.stDapComnInfo.szPidPath,S_IRWXU|S_IRWXG|S_IRWXO) != 0)
//        {
//            printf("Fail in Pid File directory(%s)|%s\n",
//                   g_stServerInfo.stDapComnInfo.szPidPath,
//                   __func__ );
//            return 0;
//        }
//    }

//    /* 프로세스.pid 파일검사 */
//    fcom_FileCheckAndDelete(g_stServerInfo.stDapComnInfo.szPidPath,"dap_report");
//    fFile_MakeMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_report");

    char local_szTmp[256] = {0x00,};

    memset(local_szTmp, 0x00, sizeof(local_szTmp));
    snprintf(local_szTmp, sizeof(local_szTmp), "%s/%s/%s",pstComnInfo->szDapHome,"log","stdout.log");

    /* 표준출력(1) 표준오류(2) 리다이렉션 처리 */
    fcom_InitStdLog(local_szTmp);

    printf("Success in Init, pid(%d) | %s\n",getpid(),__func__ );


    return TRUE;

}



static void fstHandleSignal(int sid)
{
    char    local_szPidPath[127 +1] = {0x00,};
    char*   local_ptrProcessName = NULL;

    WRITE_CRITICAL(CATEGORY_DEBUG,"Got signal(%d)",sid );
    WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Recv Signal (%d) Process Stop Status", sid);

    /** Get Pid File Process Name **/
    fFile_SetPidFileSt();
    local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_REPORT);

    snprintf(local_szPidPath, sizeof(local_szPidPath), "%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_DEBUG(CATEGORY_DEBUG,"Thread Is Stop Status");
    if ( g_pthreadStopFlag == 0x00 )
    {
        g_pthreadStopFlag = 0x01;
    }

    sleep(2);

    if(g_pstNotiMaster != NULL)
        free(g_pstNotiMaster);

    fipc_FQClose(PRMON_QUEUE);
    fipc_FQClose(DBLOG_QUEUE);
    fipc_FQClose(REPORT_QUEUE);

    fdb_SqlClose(g_stMyCon);

    fFile_cleanupMasterPid( local_szPidPath, local_ptrProcessName);

    exit(0);
}


static int fstCustSendMail(int bType, char *p_file, char *p_from, char *p_to)
{
    char command[1024];

    memset(command, 0x00, sizeof(command));
    if(!strcmp(p_file, ""))
    {
        sprintf(command, "%s/bin/sendreport.sh %d \"\" %s %s %s",
                g_stServerInfo.stDapComnInfo.szDapHome,
                bType,
                p_from,
                p_to,
                fcom_GetCustLang());
    }
    else
    {
        sprintf(command, "%s/bin/sendreport.sh %d %s %s %s %s",
                g_stServerInfo.stDapComnInfo.szDapHome,
                bType,
                p_file,
                p_from,
                p_to,
                fcom_GetCustLang());
    }
    system(command);

    WRITE_INFO(CATEGORY_INFO,"Send mail info, file(%s)from(%s)to(%s)lang(%s) ",
            p_file,
            p_from,
            p_to,
            fcom_GetCustLang());

    return 0;
}

static int fstSendMail(char *repFullName, int bType)
{
    char command[1024];
    char mailTo[512+1];
    char strType[10];

    _DAP_COMN_INFO* pstComnInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    memset(command, 0x00, sizeof(command));
    if(bType == 1) //day
    {
        strcpy(mailTo, g_stProcReportInfo.szConfMailToDay);
        strcpy(strType, "day");
    }
    else if(bType == 2) //week
    {
        strcpy(mailTo, g_stProcReportInfo.szConfMailToWeek);
        strcpy(strType, "week");
    }
    else if(bType == 3) //month
    {
        strcpy(mailTo, g_stProcReportInfo.szConfMailToMonth);
        strcpy(strType, "month");
    }

    sprintf(command, "%s/bin/sendreport.sh %d %s %s %s %s",
            pstComnInfo->szDapHome,
            bType,
            repFullName,
            g_stProcReportInfo.szConfMailFrom,
            mailTo,
            g_stProcReportInfo.szConfMailLang);

    WRITE_INFO(CATEGORY_INFO, "Send mail for %s, file(%s)from(%s)to(%s)lang(%s)",
               strType,
               repFullName,
               g_stProcReportInfo.szConfMailFrom,
               mailTo,
               g_stProcReportInfo.szConfMailLang);

    WRITE_DEBUG(CATEGORY_DEBUG,"Send Report Execute Shell Path(%s)", command );

    system(command);

    return 0;
}

static int fstCheckNotiFile(void)
{
    register int i;
    struct stat statBuf;


    for(i = 0; i < MAX_REPORT_NOTI; i++)
    {
        if(stat(g_pstNotiMaster[i].szNotiFileName, &statBuf) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in stat noti files(%s)(%d)",
                    g_pstNotiMaster[i].szNotiFileName,
                    i);

            printf("%s\n", strerror(errno));
            return  -1;
        }

        if(statBuf.st_mtime > g_pstNotiMaster[i].lastModify)
        {
            g_pstNotiMaster[i].reload = TRUE;
        }
        else
        {
            g_pstNotiMaster[i].reload = FALSE;
        }
    }


    return TRUE;
}


static void fstReloadCfgFile( )
{
    g_stProcReportInfo.nCfgReportCheckNotiInterval = fcom_GetProfileInt("REPORT","CHECK_NOTI_INTERVAL", 10);
    g_stProcReportInfo.nCfgReportInterval = fcom_GetProfileInt("REPORT","CYCLE", 1);
    g_stProcReportInfo.nCfgPrmonInterval = fcom_GetProfileInt("PRMON","INTERVAL", 600);

    /* Log Set Info Reload */
    g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize = fcom_GetProfileInt("KEEPLOG","MAX_LOG_FILE_SIZE",50) * 1024000;
    g_stServerInfo.stDapLogInfo.nCfgDebugLevel     = fcom_GetProfileInt("DEBUG","LEVEL",1);
    g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel   = fcom_GetProfileInt("DEBUG","DBWRITE",1);
    g_stServerInfo.stDapLogInfo.nCfgKeepDay        = fcom_GetProfileInt("KEEPLOG","MAX_LOG_KEEP_DAY",30);


    WRITE_ALL(CATEGORY_INFO,"DAP Config IS Changed \n CHECK_NOTI_INTERVAL : [%d] \n REPORT_CYCLE : [%d] \n PRMON_INTERVAL : [%d] \n "
                            "MAX_LOG_FILE_SIZE : [%d] \n LOG_LEVEL : [%d] \n DBWRITE_LEVEL : [%d] \n "
                            "MAX_LOG_KEEP_DAY : [%d] ",
              g_stProcReportInfo.nCfgReportCheckNotiInterval,
              g_stProcReportInfo.nCfgReportInterval,
              g_stProcReportInfo.nCfgPrmonInterval,
              g_stServerInfo.stDapLogInfo.nCfgMaxLogFileSize,
              g_stServerInfo.stDapLogInfo.nCfgDebugLevel,
              g_stServerInfo.stDapLogInfo.nCfgDbWriteLevel,
              g_stServerInfo.stDapLogInfo.nCfgKeepDay);

}
