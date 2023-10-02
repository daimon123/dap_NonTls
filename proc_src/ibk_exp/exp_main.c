/*
 * DAP Master
 */

/* ------------------------------------------------------------------- */
/* System Header                                                       */
/* ------------------------------------------------------------------- */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/timeb.h>
#include <sys/statfs.h>

/* ------------------------------------------------------------------- */
/* User Header                                                         */
/* ------------------------------------------------------------------- */
#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "linuxke/dap_linux.h"
#include "db/dap_mysql.h"
//#include "db/dap_checkdb.h"
#include "sock/dap_sock.h"
//#include "dap_version.h"
#include "master.h"

/*
enum LEVEL_RATE
{
    LEVEL_PASS = 0,
    LEVEL_DROP,
    LEVEL_INFO,
    LEVEL_CAUTION,
    LEVEL_WARNING,
    LEVEL_CRITICAL,
    LEVEL_BLOCK
};
*/


#include "util_func.c"
#include "mysql_event_data.c"
#include "data_define.h"

typedef unsigned char BOOL;
#define TRUE 1
#define FALSE 0

#define		DF_RUNFILE_NAME					"ibk_exp"

char szIBKLogHome       [128+1];

/* ------------------------------------------------------------------- */
/* Static Function                                                     */
/* ------------------------------------------------------------------- */
static int fstMainTask(void);
static int fstMasterInit(void);
static void fst_FrdDumpExec(void);
//static int fIBK_GetLastEventNo(unsigned int *nLastEV_SQ);
//static int fIBK_Eventinfo(unsigned short sYear, unsigned short sMonth, char* szTime, unsigned int *nLastEVH_SQ);
//static int fIBK_EventInfo_Router(unsigned short sYear, unsigned short sMonth, unsigned int nHB_SQ, unsigned int nUS_SQ, char* szDetectTime, _ROUTER_EVENT_DATA *stData);

//static int fIBK_EventInfo_Disk_New

static int fIBK_LogWrite_Router(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_Disk_New(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_Disk_Removable(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_Wifi(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_Bluetooth(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_Infrared(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_LogWrite_NetAdapter(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime);
static int fIBK_Create_EventLogFile();
static int fIBK_Create_ManagerLogFile();
static int fIBK_LogWrite_Manager(char* ipAddr, char* szID, unsigned short nActionType, char* szDetectTime);

typedef enum _ServiceStatusEnum
{
    SERVICE_UNKNOWN = 0,
    SERVICE_START = 1,
    SERVICE_STOP,

}ServiceStatusEnum;

ServiceStatusEnum ServiceStatus(char *szServiceStatus)
{
    if (strcmp(szServiceStatus, "start") == 0)
    {
        return SERVICE_START;
    }

    if (strcmp(szServiceStatus, "stop") == 0)
    {
        return SERVICE_STOP;
    }

    // 아무일도 없음
    return SERVICE_UNKNOWN;
}

int fIBK_LogDirCheck(void)
{
    int nRet;

    if(access(g_stServerInfo.stDapLogInfo.szCfgLogPath, F_OK) != 0)
    {
        if((nRet = fIBK_MkPath(g_stServerInfo.stDapLogInfo.szCfgLogPath, 0755)) != RET_SUCC)
        {
            return RET_FAIL;
        }
    }
    return RET_SUCC;
}

int fIBK_SetLogInfo(_DAP_LOG_INFO* param_stDapLogInfo, char* param_ProcName)
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

    /* Set IBK Expand Log Home */
    snprintf(szIBKLogHome             ,
             sizeof(szIBKLogHome)     ,
             "%s/ibk"                              ,
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

int fIBK_LogInit(char* param_ProcName)
{
    _DAP_LOG_INFO* pstLogInfo;
    pstLogInfo  = &g_stServerInfo.stDapLogInfo;

    memset(pstLogInfo->szCfgLogPath    , 0x00, sizeof(pstLogInfo->szCfgLogPath    ));
    memset(pstLogInfo->szcfgLogFileName, 0x00, sizeof(pstLogInfo->szcfgLogFileName));

    fIBK_SetLogInfo(pstLogInfo, param_ProcName);

    if( fcom_LogDirCheck() != RET_SUCC )
    {
        printf("Log Dir Check Fail \n");
        return RET_FAIL;
    }

    return RET_SUCC;
}

// 파일이 존재하는지 검사한다.
short FileCheckStatus(char *pre_filepath_filename)
{
    struct 	statfs 	local_file_stat;

    if (statfs((char *)pre_filepath_filename, &local_file_stat) == 0)
    {
        //screen_out(INFO_LOG, "%s", pre_filepath_filename);
        //screen_out(INFO_LOG, "Pid created!!\n");
        return 0;	 // 파일 있음
    }
    else
    {
        // screen_out(INFO_LOG, "%s", pre_filepath_filename);
        // screen_out(INFO_LOG, "Pid %s create Failed!!\n", pre_filepath_filename);
        return -1;	// 파일 없음
    }
}

// 프로세스 이름으로 파일 PID 을 생성한다.
int MakeMasterPID()
{
    FILE *local_fp = NULL;

    char local_filename[30];
    char local_buff[10];

    sprintf(local_filename, "%s.pid",  DF_RUNFILE_NAME);
    //screen_out(INFO_LOG, "[%s][%d]Make Pid File : %s",  __func__, __LINE__, local_filename);

    if ((local_fp = fopen(local_filename, "w")) == NULL)
    {
        //screen_out(INFO_LOG, "Writing pid file failed!!");
        return -1;
    }
    else
    {
        //screen_out(INFO_LOG, "Writing pid file!!");
        sprintf(local_buff, "%d", getpid());
        fputs(local_buff, local_fp);

        fclose(local_fp);

        return 0;
    }
}

BOOL CheckMasterPID()
{
    char local_buff[255] = "";

    sprintf(local_buff, "%s.pid", DF_RUNFILE_NAME);
    //screen_out(INFO_LOG, "[%s][%d]Check Pid File : %s", __func__, __LINE__, local_buff);
    if (FileCheckStatus(local_buff) == 0)
    {
        return TRUE;	// 파일 있음
    }

    return FALSE; // 파일 없음
}

//	프로세스 이름으로 PID 값을 리턴 받는다.
int GetMasterPID()
{
    FILE *local_fp;
    char local_buff[255];

    int local_return_value = 0;

    sprintf(local_buff, "%s.pid", DF_RUNFILE_NAME);

    if ((local_fp = fopen(local_buff, "r")) == NULL)
    {
        local_return_value = -1;
    }
    else
    {
        fread(local_buff, 1, 10, local_fp);
        local_return_value = atoi(local_buff);
        fclose(local_fp);
    }

    return local_return_value;
}

// 프로세스 이름으로 파일 PID 삭제한다.
void CleanUpMasterPID()
{
    char local_filename[30] = "";

    sprintf(local_filename, "%s.pid", DF_RUNFILE_NAME);
    unlink(local_filename);
    //printf("\n'IBK Log Interworking' Service process id deleted. (%s)\n\n",
    //       local_filename);
}

void SvcStart()
{
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        printf("fork failed.\n");
        WRITE_CRITICAL(CATEGORY_DEBUG, "fork failed.");
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        printf("setsid failed.n");
        WRITE_CRITICAL(CATEGORY_DEBUG, "setsid failed.");
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    //if ((chdir("/")) < 0) {
        /* Log the failure */
    //    printf("SvcStart - 5\n");
    //    exit(EXIT_FAILURE);
    //}

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);

    // TEST - WDKIM
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (MakeMasterPID() < 0)
    {
        printf("Failed to write pid file.\n");
        WRITE_CRITICAL(CATEGORY_DEBUG, "Failed to write pid file.");
    }
}

int main(int argc, char **argv)
{
    //int nMaxLimit = 0;
    //_DAP_COMN_INFO* pstComnInfo = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    memset(&g_stServerInfo, 0x00, sizeof(g_stServerInfo));

    //pstComnInfo = &g_stServerInfo.stDapComnInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    if(argc < 2)
    {
        //DAP_VERSION_MACRO
        printf("\nUsage: ibk_exp [start|stop]\n\n");
        exit(0);
    }

    switch (ServiceStatus(argv[1])) {
        case SERVICE_START:    // 서비스 기동

            if (CheckMasterPID())
            {
                printf("\nServices already running.\n\n");
                exit(0);
            }

            printf("\nStart 'IBK Log Interworking' service.\n\n");

            SvcStart();

            if (CheckMasterPID())
            {
                int pid = GetMasterPID();
                printf("service PID = %d\n", pid);
            }
            else
            {
                printf("Failed to check pid file.\n");
            }

            break;
        case SERVICE_STOP:    // 서비스 종료

            if (CheckMasterPID())
            {
                int pid = GetMasterPID();

                kill(pid, SIGTERM);

                printf("\n'IBK Log Interworking' service termination.\n\n");

                CleanUpMasterPID();
            }
            else
            {
                printf("\n'IBK Log Interworking' Service not running.\n\n");
            }

            exit(0);
            break;
        case SERVICE_UNKNOWN:
            printf("\nUsage: ibk_exp [start|stop]\n\n");
            exit(0);
            break;
    }

    /* Argument Parsing */
    //fcom_ArgParse(argv);

    /* DAP Process Init */
    fstMasterInit();

    /* DAP Process Log Init */
    sprintf(g_stServerInfo.stDapComnInfo.szDebugName, "ibk_exp");
    fIBK_LogInit(g_stServerInfo.stDapComnInfo.szDebugName);

    /* Get Max Open Files Limit */
    //nMaxLimit = fmaster_GetPrintRlim();

    //fcom_SetDeamon(nMaxLimit, pstComnInfo->nCfgMaxOpenFileLimit);

/*
    if(nMaxLimit < pstComnInfo->nCfgMaxOpenFileLimit)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Exit program because, currOpenFileLimit(%d) < maxOpenFileLimit(%d)",
                nMaxLimit,
                pstComnInfo->nCfgMaxOpenFileLimit);

        fipc_FQClose(DBLOG_QUEUE);
        fcom_MallocFree((void**)&g_pstNotiMaster);

        exit(0);
    }
*/

    /* Daemonize */
    /* Exception Signal */
    //fmaster_GuardSignal();

    fstMainTask();

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */

    WRITE_INFO(CATEGORY_DEBUG, "Program Exit!" );

    return RET_SUCC;
}

// 2022.09.17 - 기업은행 요청 사항 적용 : 이벤트 건수별 생성되던 데이터 파일을 데이터 분류별로 1일 하나씩 생성하고 발생 이벤트가 없는 경우에도 데이터 파일은 생성하도록 수정.
int fstMainTask(void)
{
    //int nRetval = 0;
    //char szDelFileDay[10]           = {0x00,};
    //char szcurrDay[10]              = {0x00,};
    char local_szPidPath[127 +1]    = {0x00,};

    //time_t	currTime, delFileTime;
    //time_t  nCfgLastModify;

    g_ptrProcessInfo = (_PROCESS_INFO*)g_stProcessInfo;

    snprintf(local_szPidPath, sizeof(local_szPidPath),"%s/bin", g_stServerInfo.stDapComnInfo.szDapHome);

    WRITE_INFO(CATEGORY_DEBUG, "Function Start " );

    /** Get Pid File Process Name **/
    //fFile_SetPidFileSt();

    //char* local_ptrProcessName = NULL;
    //local_ptrProcessName = fFile_GetPidFileName(ENUM_DAP_MASTER);

    /** Check Master Pid File **/
    /*if (fFile_CheckMasterPid(local_szPidPath, local_ptrProcessName) == 0) // 파일있음
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Already Exist Pid File (%s/%s)", local_szPidPath, local_ptrProcessName);
        return (-1);
    }*/

    /** Make Master Pid File **/
    //fFile_MakeMasterPid(local_szPidPath, local_ptrProcessName);

    if(machine_init(&g_stServerInfo.stStatics) == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in init machine " );
        //nRetval = 1;
        goto __goto__end__;
    }

    /*
    if(fmaster_GetOneprocessInfo("dap_master", "") > 1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in restart " );
        //nRetval = 0;
        goto __goto__end__;
    }
    */

    /* Maria DB Connect */
    if(fdb_ConnectDB() != RET_SUCC)
    {
        WRITE_INFO(CATEGORY_DB, "Fail in db connection " );
        fdb_RetryConnectDb();
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Success Db Connection ");
    }

    /*
    if (fdb_CheckTbTables() < 0)
    {
        fcom_ExitServer(0);
    }
    */

    /*
    // 0, 25293, 2022-03-18 14:54:24
    int nDataCount = fIBK_EventInfo_Disk(2022, 3, 25293, 0, "2022-03-18 14:54:24");
    printf("Disk Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        printf("summary = %s\n", g_stDiskList[0].dk_summary);

        char* pRtn = fIBK_GetAddInfo(g_stDiskList[0].dk_summary);

        if (pRtn)
        {
            char szValue2[1024] = {};
            strcpy(szValue2, pRtn);
            printf("add = %s\n", szValue2);

            int nItemCount = fIBK_GetInnerDataCount(szValue2);
            printf("item count = %d\n", nItemCount);

            int nItemIndex = 0;
            for (; nItemIndex < nItemCount; nItemIndex++)
            {
                pRtn = fIBK_GetInnerData(szValue2, nItemIndex);
                printf("sub = %s\n", pRtn);

                int nDiskIndex = 0;
                for (; nDiskIndex < nDataCount; nDiskIndex++)
                {
                    if (strcmp(g_stDiskList[nDiskIndex].dk_model, pRtn) == 0)
                    {
                        printf("new disk = %s\n", pRtn);
                        continue;
                    }

                    if (strcmp(g_stDiskList[nDiskIndex].dk_desc, pRtn) == 0)
                    {
                        printf("new disk2 = %s\n", pRtn);
                    }
                }
            }
        }
    }
    */

    char szCurrTime[20];
    struct timeb ct;
    struct tm timeinfo;
    //int len = 0;

    ftime(&ct);

    localtime_r(&ct.time, &timeinfo);

    unsigned short sYear_Temp = timeinfo.tm_year + 1900;
    unsigned short sMonth_Temp = timeinfo.tm_mon + 1;

    unsigned short sYear = 0;
    unsigned short sMonth = 0;

    sprintf(szCurrTime, "%04d-%02d-%02d %02d:%02d:%02d",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec);

    unsigned int nLastEventNo = 0;
    fIBK_GetLastEventNo(&nLastEventNo);

    unsigned int nLastEventNo_His = 0;
    fIBK_GetLastEventNo_His(sYear_Temp, sMonth_Temp, &nLastEventNo_His);

    if (nLastEventNo_His > nLastEventNo)
    {
        nLastEventNo = nLastEventNo_His;
    }

    unsigned int nLastManagerLogNo = 0;

    fIBK_GetLastManagerLogNo_His(sYear_Temp, sMonth_Temp, &nLastManagerLogNo);

    // TEST
    //printf("EV_SQ : %d\n", nLastEventNo);

    /** FRD Dump 처리 **/
    //fst_FrdDumpExec();

    /** Master 프로세스 초기 기동시 Process 기동시켜준다. **/
    //fst_InitProcessInvoke();

    // TEST
    /*
    char szLogPath[1024];
    snprintf(szLogPath, sizeof(szLogPath) -1, "%s/%s\n", g_stServerInfo.stDapLogInfo.szCfgLogPath,
             g_stServerInfo.stDapLogInfo.szcfgLogFileName);
    printf(szLogPath);

    snprintf(szLogPath, sizeof(szLogPath) -1, "%s\n", szIBKLogHome);
    printf(szLogPath);
    */

    WRITE_INFO(CATEGORY_DEBUG,"Process Start " );
    //WRITE_INFO_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DEBUG, "Process Start ");

    //check 후 linker를 통한 db접속이 제대로 안될때가 있어 sleep 줌.
    sleep(1);

    //delFileTime = time((time_t) 0)- g_stServerInfo.stDapLogInfo.nCfgJobTime;
    //fcom_time2str(delFileTime, szDelFileDay,"YYYYMMDD\0");

    //unsigned char szTemp[1024] = {0x00,};

    int nDay = 0;
    int nDay_before = 0;

    /** Master Process는 여기부터 dblog 기능 사용가능. Master process가 dblog process를 띄우기전이라 이전에는 불가. **/
    while(1)
    {
        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        sprintf(szCurrTime, "%04d-%02d-%02d %02d:%02d:%02d",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);

        // TEST
        //printf("%s\n", szCurrTime);

        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;

        nDay = timeinfo.tm_mday;

        int nLastEventNo_Temp = nLastEventNo;

        int nEventSize = fIBK_Eventinfo(sYear_Temp, sMonth_Temp, szCurrTime, &nLastEventNo_Temp);

        // 날짜 정보가 변경된 경우 이전 검사일 날짜에 기록된 이벤트 누락 방지을 위해 이전 날짜로 한번 더 검사 하고 다음 검사 시점에 변경된 날짜로 검사 하도록 함.
        if (sYear_Temp != sYear || sMonth_Temp != sMonth)
        {
            nEventSize = fIBK_Eventinfo_His(sYear_Temp, sMonth_Temp, szCurrTime, nEventSize, &nLastEventNo_Temp);
        }
        else
        {
            nEventSize = fIBK_Eventinfo_His(sYear, sMonth, szCurrTime, nEventSize, &nLastEventNo_Temp);
        }

        nLastEventNo = nLastEventNo_Temp;


        int nLastManagerLogNo_Temp = nLastManagerLogNo;

        int nLogSize = fIBK_ManagerLogInfo(sYear_Temp, sMonth_Temp, szCurrTime, &nLastManagerLogNo_Temp);

        nLastManagerLogNo = nLastManagerLogNo_Temp;

        // TEST
        //if (nSize > 0) {
        //    printf("Detect Event Count : %d\n", nSize);
        //}

        sYear_Temp = sYear;
        sMonth_Temp = sMonth;

        // 데이터 파일 검사 및 생성
        fIBK_Create_EventLogFile();

        // 여기서 이벤트별 분류하여 로그 파일 생성

        int nIndex = 0;
        for (nIndex = 0; nIndex < nEventSize; nIndex++) {
            switch (g_stIBKExpEvent[nIndex].ev_type) {
                case EVENT_TYPE_ROUTER:
                    fIBK_LogWrite_Router(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                         g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr,g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_DISK_REG:
                case EVENT_TYPE_DISK_HIDDEN:
                case EVENT_TYPE_DISK_NEW:
                    fIBK_LogWrite_Disk_New(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                                 g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_DISK_MOBILE:
                case EVENT_TYPE_DISK_MOBILE_READ:
                case EVENT_TYPE_DISK_MOBILE_WRITE:
                    fIBK_LogWrite_Disk_Removable(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                                 g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_NET_ADAPTER_OVER:
                    fIBK_LogWrite_NetAdapter(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                                 g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_WIFI:
                    fIBK_LogWrite_Wifi(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                             g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_BLUETOOTH:
                    fIBK_LogWrite_Bluetooth(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                       g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
                case EVENT_TYPE_INFRARED_DEVICE:
                    fIBK_LogWrite_Infrared(g_stIBKExpEvent[nIndex].hb_sq, g_stIBKExpEvent[nIndex].us_sq,
                                            g_stIBKExpEvent[nIndex].us_sno, g_stIBKExpEvent[nIndex].ev_ipaddr, g_stIBKExpEvent[nIndex].ev_detect_time);
                    break;
            }

            // 1 밀리초 지연 처리
            fcom_Msleep(1);
        }

        // 저장소 초기화
        memset(g_stIBKExpEvent, 0x00, sizeof(_IBK_EXP_EVENT_INFO)*MAX_EVENT_COUNT);

        // 관리자 로그 데이터 파일 검사 및 생성
        fIBK_Create_ManagerLogFile();

        // 여기서 관리자 로그 기록

        nIndex = 0;
        for (nIndex = 0; nIndex < nLogSize; nIndex++) {

            fIBK_LogWrite_Manager(g_stIBKExpManagerLog[nIndex].mne_ipaddr, g_stIBKExpManagerLog[nIndex].mn_id,
                                  g_stIBKExpManagerLog[nIndex].mne_action_type, g_stIBKExpManagerLog[nIndex].mne_record_time);
        }

        // 저장소 초기화
        memset(g_stIBKExpManagerLog, 0x00, sizeof(_IBK_EXP_MANAGER_LOG_INFO)*MAX_EVENT_COUNT);

       // currTime = time((time_t) 0);
        //delFileTime = fcom_CalcTime(PERIOD_STAT);

        /*
        if(fmaster_ReloadConfig() < 0)
        {
            fmaster_DbReconnect();
        }

        if(fmaster_SaveProcessInfo() < 0)
        {
            fmaster_DbReconnect();
        }

        fdb_CheckNotiDb(g_stServerInfo.stDapComnInfo.szDebugName);

        if(fmaster_CheckNotiFile() < 0)
        {
            fmaster_DbReconnect();
        }

        fmaster_CheckProcAlarm(g_ptrProcessInfo, gProcess);
        fmaster_KillProcess(g_ptrProcessInfo);
        fmaster_InvokeProcess(g_ptrProcessInfo);

        fcom_time2str(currTime, szcurrDay, "YYYYMMDD\0");

        if(strcmp(szcurrDay,szDelFileDay) > 0)
        {
            delFileTime    =   time((time_t) 0);
            fcom_time2str(delFileTime, szDelFileDay, "YYYYMMDD\0");
            fmaster_DeleteLogProcess();
            fmaster_DeleteDbProcess();
            fdb_DeleteSyncTableByDay();
        }

        if(fcom_FileCheckModify(g_stServerInfo.stDapComnInfo.szComConfigFile,&nCfgLastModify) == 1)
        {
            fmaster_ReloadCfgFile();
        }
        */

        // 5초에 한번씩 검사
        fcom_Msleep(5000);

        if (nDay_before != nDay)
        {
            // 일주일 이전에 기록된 로그 디렉토리 삭제
            fIBK_LogFileDelete(szIBKLogHome);
        }

        nDay_before = nDay;
    }

    //return 1;

__goto__end__:
    //fcom_ExitServer(nRetval);

    return RET_SUCC;
}

static int fstMasterInit(void)
{
    int  nRetVal   = 0;
    int  i         = 0;
    char szConfFile[128 +1]         =   {0x00,};
    char local_szCfgTmp[255 +1]           =   {0x00,};
    char local_szDefaultIp[15 +1]   = {0x00,};
    _DAP_COMN_INFO*  pstComnInfo    = NULL;
    _DAP_QUEUE_INFO* pstQueueInfo   = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    i = nRetVal = RET_SUCC;
    memset(szConfFile, 0x00, sizeof(szConfFile));

    pstComnInfo  = &g_stServerInfo.stDapComnInfo;
    pstQueueInfo = &g_stServerInfo.stDapQueueInfo;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */
    snprintf(pstComnInfo->szDapHome, sizeof(pstComnInfo->szDapHome  ), getenv("DAP_HOME"));

    // TEST
    //printf("WDKIM1 : %s\n", pstComnInfo->szDapHome);

    snprintf(pstComnInfo->szComConfigFile, sizeof(pstComnInfo->szComConfigFile), "%s%s", pstComnInfo->szDapHome, "/config/dap.cfg");

    // TEST
    //printf("WDKIM2 : %s\n", pstComnInfo->szComConfigFile);

    if(pstComnInfo->szComConfigFile[0] == 0x00)
    {
        nRetVal = (-1);
        goto __goto_end__;
    }

    fcom_SetIniName(pstComnInfo->szComConfigFile);

    pstComnInfo->nCfgMgwId            = fcom_GetProfileInt("COMMON","SERVER_ID",1);
    fsock_GetNic(local_szCfgTmp);
    fsock_GetIpAddress(local_szCfgTmp, local_szDefaultIp);
    fcom_GetProfile("COMMON","SERVER_IP",g_stServerInfo.stDapComnInfo.szServerIp, local_szDefaultIp );
    pstComnInfo->nCfgMaxOpenFileLimit = fcom_GetProfileInt("COMMON","MAX_OPEN_FILE_LIMIT", 65536);
    pstComnInfo->nArgSecInterval      = fcom_GetProfileInt("COMMON","PROCESS_CHECK_INTERVAL",15);


    /* Master Noti File Init */
    /* --------------------------------------------------------------------- */
    /* 0 : PROCESS_CHANGE */
    if(fcom_malloc((void**)&g_pstNotiMaster, sizeof(_CONFIG_NOTI) * MAX_MASTER_NOTI) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    snprintf(g_pstNotiMaster[PROCESS_CHANGE].szNotiFileName, sizeof(g_pstNotiMaster[PROCESS_CHANGE].szNotiFileName), "%s/config/proc_change",
            g_stServerInfo.stDapComnInfo.szDapHome);

    for(i = 0; i < MAX_MASTER_NOTI; i++)
    {
        g_pstNotiMaster[i].lastModify = 0;
        g_pstNotiMaster[i].reload     = TRUE;
    }

    snprintf(pstQueueInfo->szDAPQueueHome, sizeof(pstQueueInfo->szDAPQueueHome), "%s/.DAPQ", pstComnInfo->szDapHome);

    if(access(pstQueueInfo->szDAPQueueHome, R_OK) != 0 )
    {
        if (mkdir(pstQueueInfo->szDAPQueueHome, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
        {
            WRITE_CRITICAL(CATEGORY_INFO,"Fail in make queue directory(%s) ", pstQueueInfo->szDAPQueueHome);
            exit(0);
        }
    }

    snprintf(pstQueueInfo->szDblogQueueHome, sizeof(pstQueueInfo->szDblogQueueHome), "%s/DBLOGQ", pstQueueInfo->szDAPQueueHome);

    if(fipc_FQPutInit(DBLOG_QUEUE,pstQueueInfo->szDblogQueueHome) < 0)
    {
        printf("Fail in fqueue init, path(%s)", pstQueueInfo->szDblogQueueHome);
        exit(0);
    }

    /* Process Info Initialize */
    memset(g_stProcessInfo, 0x00, sizeof(g_stProcessInfo));

    printf("Success Init | %s\n", __func__ );

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */
__goto_end__:
    printf("%s RetValue = %d \n", __FUNCTION__, nRetVal);
    return nRetVal;

}

static void fst_FrdDumpExec(void)
{
    char buf[512 +1];

    memset(buf, 0x00, sizeof(buf));

    sprintf(buf, "%s/bin/dap_frd dump", getenv("DAP_HOME"));

    system(buf);

    return;
}

int fIBK_LogWrite_Router(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    _ROUTER_EVENT_DATA stData;

    int nVal = fIBK_EventInfo_Router(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime, &stData);

    if (0 >= nVal)
    {
        return nVal;
    }

    unsigned char szLogPath[1024] = {0x00,};
    unsigned char szTemp[1024] = {0x00,};
    unsigned char szCurrTime[24] = {0x00,};

    struct timeb ct;
    struct tm timeinfo;

    ftime(&ct);
    localtime_r(&ct.time, &timeinfo);

    // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
    sprintf(szCurrTime, "%04d%02d%02d_000000000",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday);

    // 현재시간 기준으로 폴더 경로 지정
    sYear = timeinfo.tm_year + 1900;
    sMonth = timeinfo.tm_mon + 1;
    sDay = timeinfo.tm_mday;

    sprintf(szLogPath, "%s/%u/%02u/%02u/%s_route.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    fIBK_MkPath(szLogPath, 0755);

    FILE* fp = NULL;

    if (-1 == access(szLogPath, F_OK))
    {
        fp = fopen(szLogPath, "w");

        snprintf(szTemp, sizeof(szTemp), "[IP],[SNO],[RT_INFO],[RT_IPADDR],[DETECT_TIME]\n");
        fprintf(fp, "%s", szTemp);
    }
    else
    {
        fp = fopen(szLogPath, "a");
    }

    snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
             ipAddr, szSNO, stData.rt_info, stData.rt_ipaddr, stData.rt_detect_time);
    fprintf(fp,"%s",szTemp);

    fclose(fp);

    return 0;
}

int fIBK_LogWrite_Disk_New(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_Disk_New\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    int nDataCount = fIBK_EventInfo_Disk(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime);

    // TEST
    //printf("Disk Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_disk.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE* fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DK_MODEL],[DK_VOLUME_SN],[DK_PHYSICAL_SN],[DK_INTERFACE_TYPE],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        // TEST
        //printf("summary = %s\n", g_stDiskList[0].dk_summary);

        char* pRtn = fIBK_GetAddInfo(g_stDiskList[0].dk_summary);

        if (pRtn)
        {
            char szValue2[1024] = {};
            strcpy(szValue2, pRtn);

            // TEST
            //printf("add = %s\n", szValue2);

            int nItemCount = fIBK_GetInnerDataCount(szValue2);

            // TEST
            //printf("item count = %d\n", nItemCount);

            int nItemIndex = 0;
            for (; nItemIndex < nItemCount; nItemIndex++)
            {
                pRtn = fIBK_GetInnerData(szValue2, nItemIndex);

                // TEST
                //printf("sub = %s\n", pRtn);

                int nDiskIndex = 0;
                for (; nDiskIndex < nDataCount; nDiskIndex++)
                {
                    if (strcmp(g_stDiskList[nDiskIndex].dk_model, pRtn) == 0 )
                    {
                        snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                                 ipAddr, szSNO, g_stDiskList[nDiskIndex].dk_model, g_stDiskList[nDiskIndex].dk_volume_sn,
                                 g_stDiskList[nDiskIndex].dk_physical_sn, g_stDiskList[nDiskIndex].dk_interface_type, g_stDiskList[nDiskIndex].dk_detect_time);
                        fprintf(fp,"%s",szTemp);

                        continue;
                    }

                    if (strcmp(g_stDiskList[nDiskIndex].dk_desc, pRtn) == 0)
                    {
                        snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                                 ipAddr, szSNO, g_stDiskList[nDiskIndex].dk_desc, g_stDiskList[nDiskIndex].dk_volume_sn,
                                 g_stDiskList[nDiskIndex].dk_physical_sn, g_stDiskList[nDiskIndex].dk_interface_type, g_stDiskList[nDiskIndex].dk_detect_time);
                        fprintf(fp,"%s",szTemp);
                    }
                }
            }
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_LogWrite_Disk_Removable(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_Disk_Removable\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    int nDataCount = fIBK_EventInfo_Disk(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime);

    // TEST
    //printf("Disk Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_disk.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE* fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DK_MODEL],[DK_VOLUME_SN],[DK_PHYSICAL_SN],[DK_INTERFACE_TYPE],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        // TEST
        //printf("summary = %s\n", g_stDiskList[0].dk_summary);

        char* pRtn = fIBK_GetAddInfo(g_stDiskList[0].dk_summary);

        if (NULL == pRtn)
        {
            pRtn = fIBK_GetModifyInfo(g_stDiskList[0].dk_summary);
        }

        if (pRtn)
        {
            char szValue2[1024] = {};
            strcpy(szValue2, pRtn);

            // TEST
            //printf("add = %s\n", szValue2);

            int nItemCount = fIBK_GetInnerDataCount(szValue2);

            // TEST
            //printf("item count = %d\n", nItemCount);

            int nItemIndex = 0;
            for (; nItemIndex < nItemCount; nItemIndex++)
            {
                pRtn = fIBK_GetInnerData(szValue2, nItemIndex);

                // TEST
                //printf("sub = %s\n", pRtn);

                int nDiskIndex = 0;
                for (; nDiskIndex < nDataCount; nDiskIndex++)
                {
                    if (g_stDiskList[nDiskIndex].dk_drive_type != 2)
                    {
                        continue;
                    }

                    if (strcmp(g_stDiskList[nDiskIndex].dk_model, pRtn) == 0 )
                    {
                        snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                                 ipAddr, szSNO, g_stDiskList[nDiskIndex].dk_model, g_stDiskList[nDiskIndex].dk_volume_sn,
                                 g_stDiskList[nDiskIndex].dk_physical_sn, g_stDiskList[nDiskIndex].dk_interface_type, g_stDiskList[nDiskIndex].dk_detect_time);
                        fprintf(fp,"%s",szTemp);

                        continue;
                    }

                    if (strcmp(g_stDiskList[nDiskIndex].dk_desc, pRtn) == 0)
                    {
                        snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                                 ipAddr, szSNO, g_stDiskList[nDiskIndex].dk_desc, g_stDiskList[nDiskIndex].dk_volume_sn,
                                 g_stDiskList[nDiskIndex].dk_physical_sn, g_stDiskList[nDiskIndex].dk_interface_type, g_stDiskList[nDiskIndex].dk_detect_time);
                        fprintf(fp,"%s",szTemp);
                    }
                }
            }
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_LogWrite_Wifi(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_Wifi\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    int nDataCount = fIBK_EventInfo_Wifi(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime);

    // TEST
    //printf("Wifi Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_netdev.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE* fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DEVICE_TYPE],[DEVICE_INFO],[DEVICE_IP],[DEVICE_MAC],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        int nWifiIndex = 0;
        for (; nWifiIndex < nDataCount; nWifiIndex++)
        {
            snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"WIFI\",\"%s\",\"\",\"%s\",\"%s\"\n",
                     ipAddr, szSNO, g_stWifiList[nWifiIndex].wf_interface_desc, g_stWifiList[nWifiIndex].wf_mac_addr,
                     g_stWifiList[nWifiIndex].wf_detect_time);
            fprintf(fp,"%s",szTemp);
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_LogWrite_Bluetooth(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_Bluetooth\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    int nDataCount = fIBK_EventInfo_Bluetooth(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime);

    // TEST
    //printf("Bluetooth Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_netdev.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE* fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DEVICE_TYPE],[DEVICE_INFO],[DEVICE_IP],[DEVICE_MAC],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        int nBlueIndex = 0;
        for (; nBlueIndex < nDataCount; nBlueIndex++)
        {
            snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"BLUETOOTH\",\"%s (%s)\",\"\",\"%s\",\"%s\"\n",
                     ipAddr, szSNO, g_stBluetoothList[nBlueIndex].bt_instance_name, g_stBluetoothList[nBlueIndex].bt_minor_device,
                     g_stBluetoothList[nBlueIndex].bt_mac_addr, g_stBluetoothList[nBlueIndex].bt_detect_time);
            fprintf(fp,"%s",szTemp);
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_LogWrite_Infrared(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_Infrared\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    _INFRARED_DEVICE_EVENT_DATA stData;
    int nDataCount = fIBK_EventInfo_InfraredDevice(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime, &stData);

    // TEST
    //printf("Infrared Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_netdev.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE *fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DEVICE_TYPE],[DEVICE_INFO],[DEVICE_IP],[DEVICE_MAC],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        int nAdapterIndex = 0;
        for (; nAdapterIndex < nDataCount; nAdapterIndex++)
        {
            snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",\"INFRARED\",\"%s\",\"\",\"\",\"%s\"\n",
                     ipAddr, szSNO, stData.id_name, stData.id_detect_time);
            fprintf(fp,"%s",szTemp);
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_LogWrite_NetAdapter(unsigned int nHB_SQ, unsigned int nUS_SQ, char* szSNO, char* ipAddr, char* szDetectTime)
{
    // TEST
    //printf("fIBK_LogWrite_NetAdapter\n");

    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    int nDataCount = fIBK_EventInfo_NetworkAdapter(sYear, sMonth, nHB_SQ, nUS_SQ, szDetectTime);

    // TEST
    //printf("NetAdapter Count = %d\n", nDataCount);

    if (nDataCount > 0)
    {
        unsigned char szLogPath[1024] = {0x00,};
        unsigned char szTemp[1024] = {0x00,};
        unsigned char szCurrTime[24] = {0x00,};

        struct timeb ct;
        struct tm timeinfo;

        ftime(&ct);
        localtime_r(&ct.time, &timeinfo);

        // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
        sprintf(szCurrTime, "%04d%02d%02d_000000000",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);

        // 현재시간 기준으로 폴더 경로 지정
        sYear = timeinfo.tm_year + 1900;
        sMonth = timeinfo.tm_mon + 1;
        sDay = timeinfo.tm_mday;

        sprintf(szLogPath, "%s/%u/%02u/%02u/%s_netdev.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

        fIBK_MkPath(szLogPath, 0755);

        FILE* fp = NULL;

        if (-1 == access(szLogPath, F_OK))
        {
            fp = fopen(szLogPath, "w");

            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DEVICE_TYPE],[DEVICE_INFO],[DEVICE_IP],[DEVICE_MAC],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
        else
        {
            fp = fopen(szLogPath, "a");
        }

        // TEST
        //printf("summary = %s\n", g_stNetAdapterList[0].na_summary);

        char* pRtn = fIBK_GetAddInfo(g_stNetAdapterList[0].na_summary);

        if (NULL == pRtn)
        {
            pRtn = fIBK_GetModifyInfo(g_stNetAdapterList[0].na_summary);
        }

        if (pRtn)
        {
            char szValue2[1024] = {};
            strcpy(szValue2, pRtn);

            // TEST
            //printf("add = %s\n", szValue2);

            int nItemCount = fIBK_GetInnerDataCount(szValue2);

            // TEST
            //printf("item count = %d\n", nItemCount);

            int nItemIndex = 0;
            for (; nItemIndex < nItemCount; nItemIndex++)
            {
                pRtn = fIBK_GetInnerData(szValue2, nItemIndex);

                int nAdapterIndex = 0;
                for (; nAdapterIndex < nDataCount; nAdapterIndex++)
                {
                    if (strcmp(g_stNetAdapterList[nAdapterIndex].na_name, pRtn) == 0 )
                    {
                        snprintf(szTemp, sizeof(szTemp),
                                 "\"%s\",\"%s\",\"NET_ADAPTER\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                                 ipAddr, szSNO, g_stNetAdapterList[nAdapterIndex].na_name,
                                 g_stNetAdapterList[nAdapterIndex].na_ip,
                                 g_stNetAdapterList[nAdapterIndex].na_mac,
                                 g_stNetAdapterList[nAdapterIndex].na_detect_time);

                        fprintf(fp,"%s",szTemp);
                    }
                }

            }
        }

        fclose(fp);
    }

    return 0;
}

int fIBK_Create_EventLogFile()
{
    unsigned char szLogPath[1024] = {0x00,};
    unsigned char szTemp[1024] = {0x00,};
    unsigned char szCurrTime[24] = {0x00,};

    struct timeb ct;
    struct tm timeinfo;

    ftime(&ct);
    localtime_r(&ct.time, &timeinfo);

    // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
    sprintf(szCurrTime, "%04d%02d%02d_000000000",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday);

    // 현재시간 기준으로 폴더 경로 지정
    unsigned short sYear = timeinfo.tm_year + 1900;
    unsigned short sMonth = timeinfo.tm_mon + 1;
    unsigned short sDay = timeinfo.tm_mday;

    sprintf(szLogPath, "%s/%d/%02d/%02d/%s_route.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    // 로그파일 기록 경로 생성
    fIBK_MkPath(szLogPath, 0755);

    FILE* fp = NULL;

    if (-1 == access(szLogPath, F_OK))
    {
        // 공유기 이벤트 로그 파일 생성
        fp = fopen(szLogPath, "w");

        if (fp != NULL)
        {
            snprintf(szTemp, sizeof(szTemp), "[IP],[SNO],[RT_INFO],[RT_IPADDR],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);

            fclose(fp);
        }
    }

    // 디스크 이벤트 로그 파일 생성
    sprintf(szLogPath, "%s/%d/%02d/%02d/%s_disk.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    if (-1 == access(szLogPath, F_OK))
    {
        fp = fopen(szLogPath, "w");

        if (fp != NULL)
        {
            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DK_MODEL],[DK_VOLUME_SN],[DK_PHYSICAL_SN],[DK_INTERFACE_TYPE],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);

            fclose(fp);
        }
    }

    // 네크워크 아답터 로그 파일 생성
    sprintf(szLogPath, "%s/%d/%02d/%02d/%s_netdev.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    if (-1 == access(szLogPath, F_OK))
    {
        fp = fopen(szLogPath,"w");

        if (fp != NULL)
        {
            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[SNO],[DEVICE_TYPE],[DEVICE_INFO],[DEVICE_IP],[DEVICE_MAC],[DETECT_TIME]\n");
            fprintf(fp, "%s", szTemp);

            fclose(fp);
        }
    }

    return 0;
}

int fIBK_Create_ManagerLogFile()
{
    unsigned char szLogPath[1024] = {0x00,};
    unsigned char szTemp[1024] = {0x00,};
    unsigned char szCurrTime[24] = {0x00,};

    struct timeb ct;
    struct tm timeinfo;

    ftime(&ct);
    localtime_r(&ct.time, &timeinfo);

    // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
    sprintf(szCurrTime, "%04d%02d%02d_000000000",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday);

    // 현재시간 기준으로 폴더 경로 지정
    unsigned short sYear = timeinfo.tm_year + 1900;
    unsigned short sMonth = timeinfo.tm_mon + 1;
    unsigned short sDay = timeinfo.tm_mday;

    sprintf(szLogPath, "%s/%d/%02d/%02d/%s_manager.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    // 로그파일 기록 경로 생성
    fIBK_MkPath(szLogPath, 0755);

    FILE* fp = NULL;

    if (-1 == access(szLogPath, F_OK))
    {
        // 공유기 이벤트 로그 파일 생성
        fp = fopen(szLogPath, "w");

        if (fp != NULL)
        {
            snprintf(szTemp, sizeof(szTemp), "[IP],[ID],[LOG_TYPE],[RECORD_TIME]\n");
            fprintf(fp, "%s", szTemp);

            fclose(fp);
        }
    }

    return 0;
}

int fIBK_LogWrite_Manager(char* ipAddr, char* szID, unsigned short nActionType, char* szDetectTime)
{
    time_t event_time =  fcom_str2time(szDetectTime, "YYYY-MM-DD hh:mm:ss");

    struct tm *aTime = localtime(&event_time);
    unsigned short sYear = aTime->tm_year + 1900;
    unsigned short sMonth = aTime->tm_mon + 1;
    unsigned short sDay = aTime->tm_mday;

    // TEST
    //printf("Infrared Count = %d\n", nDataCount);

    unsigned char szLogPath[1024] = {0x00,};
    unsigned char szTemp[1024] = {0x00,};
    unsigned char szCurrTime[24] = {0x00,};

    struct timeb ct;
    struct tm timeinfo;

    ftime(&ct);
    localtime_r(&ct.time, &timeinfo);

    // 구분(-, :)문자 제거된 현재시간 문자열 (일 정보까지만 사용. 나머지는 0 으로 채움.)
    sprintf(szCurrTime, "%04d%02d%02d_000000000",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday);

    // 현재시간 기준으로 폴더 경로 지정
    sYear = timeinfo.tm_year + 1900;
    sMonth = timeinfo.tm_mon + 1;
    sDay = timeinfo.tm_mday;

    sprintf(szLogPath, "%s/%u/%02u/%02u/%s_manager.log", szIBKLogHome, sYear, sMonth, sDay, szCurrTime);

    fIBK_MkPath(szLogPath, 0755);

    FILE *fp = NULL;

    if (-1 == access(szLogPath, F_OK))
    {
        fp = fopen(szLogPath, "w");

        if (fp) {
            snprintf(szTemp, sizeof(szTemp),
                     "[IP],[ID],[LOG_TYPE],[RECORD_TIME]\n");
            fprintf(fp, "%s", szTemp);
        }
    }
    else
    {
        fp = fopen(szLogPath, "a");
    }

    if (fp) {
        snprintf(szTemp, sizeof(szTemp), "\"%s\",\"%s\",%d,\"%s\"\n",
                 ipAddr, szID, nActionType, szDetectTime);
        fprintf(fp, "%s", szTemp);

        fclose(fp);
    }

    return 0;
}
