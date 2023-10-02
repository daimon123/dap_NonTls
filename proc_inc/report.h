//
// Created by KimByoungGook on 2020-06-24.
//

#ifndef REPORT_H
#define REPORT_H

#define		REPORT_CHANGE		0
#define		MAX_REPORT_NOTI		1
#define 	MAX_CUSTOM_ARRAY	90
#define     MAX_GROUP_CNT       1000
#define     MAX_THREAD           1

pthread_mutex_t db_mutex;

typedef struct
{
    int		nCfgPeriodicReport;
    int		nCfgReportStatsLimit;
    int		nCfgReportCheckNotiInterval;
    int		nCfgReportInterval;
    int		nCfgKeepSession;
    int		nCfgPrmonInterval;
    char	szCfgReportPath[128];

    int     nFlagDbConnected;
    time_t  nConfigLastModify;

    long	nCurTime;
    long	nLastJobTime;
    long	nLastSendTime;
    long    nIngJobTime;

    char    szConfReportRunTime[6+1];
    char	szConfMailUserView[4+1]; // ip,id,sno,name
    char	szConfMailLang[2+1];
    char    szConfMailClosedView[3+1];
    char    szConfMailFrom[64];
    char    szConfMailToDay[512+1];
    char    szConfMailToWeek[512+1];
    char    szConfMailToMonth[512+1];

}_DAP_PROC_REPORT_INFO;

_DAP_PROC_REPORT_INFO g_stProcReportInfo;


// 통계(Summary) 전 Total Row 데이터
struct _STD_EVENT_INFO
{
    char szRecordTime[13 +1];   // STD_EVENT_TB 시간
    char szDetectTime[20 +1];   // 장치별 History, 검출시간
    char szIpAddr[15 +1];       // 장치별 History, 장치 IP
    long nHbSq;                 // 장치별 History, 장치 PK값
    long nUsSq;                 // 이벤트 History, 유저 PK값
    long nUgSq;                 // 이벤트 History, 그룹 PK값
    int  nEventType;            // 이벤트 History, 이벤트 타입
    int  nEventLevel;           // 이벤트 History, 이벤트 레벨
    int  nEventExist;           // 이벤트 History, 0:해제,1:발생,2:중복,3:강제확인,4:강제삭제'
};
struct _EVENT_HISTORY_INFO
{
    char szRecordTime[13 +1];   // STD_EVENT_TB 시간
    char szDetectTime[20 +1];
//    char szDupDetectTime[20 +1];
    char szIpAddr[15 +1];       // 장치별 History, 장치 IP
    long nHbSq;
    long nUsSq;                 // 이벤트 History, 유저 PK값
    int  nEvType;
    int  nEvLevel;
    int  nEvExist;
};

struct _EVENT_TB_INFO
{
    char szRecordTime[13 +1];   // STD_EVENT_TB 시간
    char szDetectTime[20 +1];
//    char szDupDetectTime[20 +1];
    char szIpAddr[15 +1];       // 장치별 History, 장치 IP
    long nHbSq;
    long nUsSq;                 // 이벤트 History, 유저 PK값
    int  nEvType;
    int  nEvLevel;
    int  nEvExist;
};

struct _EVENT_INFO
{
    int nEventCnt;
    int nDuplCnt;

    struct _EVENT_TB_INFO* ptrEventTbDetect;
    struct _EVENT_TB_INFO* ptrEventDuplDetect;
};

struct _EVENT_HISTORY
{
    int    nDetectHistCnt;
    int    nDuplHistCnt;

    struct _EVENT_HISTORY_INFO* ptrEventHistoryDetect;
    struct _EVENT_HISTORY_INFO* ptrEventHistoryDuplDetect;
};

struct _STD_EVENT_RESULT
{
    int    EventCnt;
    struct _STD_EVENT_INFO* ptrStdEventInfo;
};


enum REPORT_EV_TYPE
{
    ENUM_CPU = 0,
    ENUM_PROCESS,
    ENUM_DISK,
    ENUM_NET_ADAPTER,
    ENUM_NET_DRIVE,
    ENUM_WIFI,
    ENUM_NET_PRINTER,
    ENUM_NET_CONNECTION,
    ENUM_BLUETOOTH,
    ENUM_INFRARE,
    ENUM_ROUTER,
    ENUM_SHARE_FOLDER,
    ENUM_VIRTUAL_MACHINE,
    ENUM_CONNEXT,
    ENUM_RDP,
    ENUM_CPU_CONTROL,
};



//char g_pthread_stop_sign[MAX_THREAD];
char g_pthreadStopFlag;


void* fstThreadRecv(void *thrid);

/* ------------------------------------------------------------------- */
/* report_task.c */
int freport_SetDataLoop(char *jsbuff, char *jsonData);
int freport_MakeReportTemplate(
        char*				repPath,
        int					bDate,
        int					bCustom,
        char*				sDate,
        char*				eDate,
        unsigned long long 	groupSq,
        char*				mngId,
        char*				repHtmlPath,
        char                param_cRealTimeFlag);
int freport_MakeTableDay(char *tmpRes, int uDayVal[][3]);
int freport_MakeTableWeek(char *tmpRes, char day[][11], int uWeekVal[][3]);
int freport_MakeTableMonth(char *tmpRes, char day[][11], int uMonthVal[][3], int monthNum);
int freport_MakeTableCustom(char *tmpRes, char time[][14], int uCustVal[][3], int valueCnt, int bTime);

/* ------------------------------------------------------------------- */




/* ------------------------------------------------------------------- */
/* report_db.c*/
#define     MAX_GROUP_CNT       1000

int freport_DeployReportDailyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// char result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly,	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// char size
        char                param_cRealTimeFlag
);
int freport_DeployReportWeeklyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart,		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly,	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// chart size
        char                param_cRealTimeFlag
);
int freport_DeployReportCustomBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        int*				chartSize,		// chart size
        char*				beginDate, 		// begin date
        char*				endDate,		// end date
        unsigned long long	grpSq,			// group sequence
        int					isCust,         // custom status
        char                param_cRealTimeFlag);

int freport_DeployReportMonthlyBysq(
        char*				p_repPath, 		// path of report
        char*				resChart, 		// chart result
        char*				resTable, 		// table result
        char*				resRange, 		// range result
        char*				resPieAnaly, 	// pie comment
        char*				resTimeAnaly, 	// time comment
        char*				resRankAnaly, 	// rank comment
        char*				resTypeAnaly, 	// type comment
        char*				resGroupAnaly, 	// group comment
        unsigned long long	grpSq,			// group sequence
        int*				chartSize,		// chart size
        char                param_cRealTimeFlag
);

int freport_GetReport(void);
int freport_TaskReport_Old(char* DbPreFix, char* TablePreFix);
int freport_TaskReport(char* param_DbPreFix, char* param_TablePreFix);
int freport_TaskReportRealTime(char* param_YYYY, char* param_MM, char* param_DD);
void freport_CpuHistory(char* param_szDbName, char* param_szTableName, char* param_szWhereTime,
                        int* param_TotalCnt, int* param_PrevCnt, int* param_ResultIdx,
                        struct _STD_EVENT_RESULT* param_ResultEvent,
                        struct _EVENT_HISTORY* param_EventHistory);
void freport_ProcessHistory(char* param_szDbName,
                            char* param_szTableName,
                            char* param_szWhereTime,
                            int* param_TotalCnt,
                            int* param_PrevCnt,
                            struct _STD_EVENT_RESULT* param_ResultEvent,
                            struct _EVENT_HISTORY*    param_EventHistory);
void freport_DiskHistory(char* param_szDbName,
                         char* param_szTableName,
                         char* param_szWhereTime,
                         int* param_TotalCnt,
                         int* param_PrevCnt,
                         struct _STD_EVENT_RESULT* param_ResultEvent,
                         struct _EVENT_HISTORY*    param_EventHistory);
void freport_NetadapterHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory);
void freport_NetdriveHistory(char* param_szDbName,
                             char* param_szTableName,
                             char* param_szWhereTime,
                             int* param_TotalCnt,
                             int* param_PrevCnt,
                             struct _STD_EVENT_RESULT* param_ResultEvent,
                             struct _EVENT_HISTORY*    param_EventHistory);
void freport_NetPrinterHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory);
void freport_NetConnectionHistory(char* param_szDbName,
                                  char* param_szTableName,
                                  char* param_szWhereTime,
                                  int* param_TotalCnt,
                                  int* param_PrevCnt,
                                  struct _STD_EVENT_RESULT* param_ResultEvent,
                                  struct _EVENT_HISTORY*    param_EventHistory);
void freport_BlueToothHistory(char* param_szDbName,
                              char* param_szTableName,
                              char* param_szWhereTime,
                              int* param_TotalCnt,
                              int* param_PrevCnt,
                              struct _STD_EVENT_RESULT* param_ResultEvent,
                              struct _EVENT_HISTORY*    param_EventHistory);
void freport_InfraredHistory(char* param_szDbName,
                             char* param_szTableName,
                             char* param_szWhereTime,
                             int* param_TotalCnt,
                             int* param_PrevCnt,
                             struct _STD_EVENT_RESULT* param_ResultEvent,
                             struct _EVENT_HISTORY*    param_EventHistory);
void freport_RouterHistory(char* param_szDbName,
                           char* param_szTableName,
                           char* param_szWhereTime,
                           int* param_TotalCnt,
                           int* param_PrevCnt,
                           struct _STD_EVENT_RESULT* param_ResultEvent,
                           struct _EVENT_HISTORY*    param_EventHistory);
void freport_WifiHistory(char* param_szDbName,
                         char* param_szTableName,
                         char* param_szWhereTime,
                         int* param_TotalCnt,
                         int* param_PrevCnt,
                         struct _STD_EVENT_RESULT* param_ResultEvent,
                         struct _EVENT_HISTORY*    param_EventHistory);
void freport_ShareFolderHistory(char* param_szDbName,
                                char* param_szTableName,
                                char* param_szWhereTime,
                                int* param_TotalCnt,
                                int* param_PrevCnt,
                                struct _STD_EVENT_RESULT* param_ResultEvent,
                                struct _EVENT_HISTORY*    param_EventHistory);
void freport_SystemHistory(char* param_szDbName,
                           char* param_szTableName,
                           char* param_szWhereTime,
                           int* param_TotalCnt,
                           int* param_PrevCnt,
                           struct _STD_EVENT_RESULT* param_ResultEvent,
                           struct _EVENT_HISTORY*    param_EventHistory);
void freport_ConnExtHistory(char* param_szDbName,
                            char* param_szTableName,
                            char* param_szWhereTime,
                            int* param_TotalCnt,
                            int* param_PrevCnt,
                            struct _STD_EVENT_RESULT* param_ResultEvent,
                            struct _EVENT_HISTORY*    param_EventHistory);
void freport_RdpSessionHistory(char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory);

void freport_CpuCtrlHistory(   char* param_szDbName,
                               char* param_szTableName,
                               char* param_szWhereTime,
                               int* param_TotalCnt,
                               int* param_PrevCnt,
                               struct _STD_EVENT_RESULT* param_ResultEvent,
                               struct _EVENT_HISTORY*    param_EventHistory);

int freport_CompEventHistory(int param_DetectCnt,
                             struct _STD_EVENT_INFO*   param_DetectInfo,
                             struct _EVENT_HISTORY*    param_HistInfo,
                             struct _STD_EVENT_RESULT* param_EventResult,
                             enum REPORT_EV_TYPE       param_EvValue);
int freport_MergeEvent(struct _EVENT_INFO* param_EventInfo,
                       struct _EVENT_HISTORY* param_EventHistInfo,
                       struct _STD_EVENT_RESULT* param_ResultInfo );
int freport_CopyEventResult(struct _EVENT_TB_INFO*      param_EventInfo,
                            struct _STD_EVENT_INFO*     param_EventResult,
                            int*                        param_KeyCnt,
                            char**                      param_KeyArray,
                            char*                       param_FindKey);
int freport_CopyEventHistResult(struct _EVENT_HISTORY_INFO*      param_EventHistInfo,
                                struct _STD_EVENT_INFO*              param_EventResult,
                                int*                                 param_KeyCnt,
                                char**                               param_KeyArray,
                                char*                                param_FindKey);
int freport_CopyEventHistory(struct _STD_EVENT_INFO*     param_StdEventResult,
                                 struct _STD_EVENT_INFO*     param_StdData,
                                 struct _EVENT_HISTORY_INFO* param_HistInfo,
                                 int*                        param_KeyCnt,
                                 char**                      param_KeyArray,
                                 char*                       param_FindKey);

int freport_SelectEventHistory(struct _EVENT_HISTORY*       param_EventHistInfo,
                               char* param_DbPreFix, char*  param_TablePreFix, char* param_WhereTime);
int freport_CpuUsageAlarm( char* param_DbPreFix, char* param_TablePreFix, char* param_WhereTime, struct _STD_EVENT_RESULT* param_ResultInfo );
//int freport_ExceptCpuEvent( char* param_DbPreFix,  char* param_TablePreFix, char* param_WhereTime, struct _STD_EVENT_RESULT* param_ResultInfo );
int freport_SelectEvent(struct _EVENT_INFO* param_EventInfo,
                        char* param_DbPreFix, char* param_TablePreFix, char* param_WhereTime);

int freport_CompareEventHistory(struct _STD_EVENT_INFO* param_StdEventInfo,
//                                struct _STD_EVENT_INFO** param_StdEventSummary,
                                int*  param_nSummaryCnt,
                                int   param_nEventCnt,
                                char* param_DbPreFix,
                                char* param_TablePreFix);
int freport_InsertStdEvent( struct _STD_EVENT_INFO* param_StdEventInfo, int param_Eventcnt, char* param_WhereTime, char param_cRealTimeFlag );
int freport_GetGroupInfo( struct _STD_EVENT_INFO* param_StdEventInfo, int param_EventCnt);
int freport_InsertKeyValue(char** param_KeyArray, int param_nKeyCnt, char* param_InsertKey);
int freport_CheckKeyValue( char** param_KeyArray, int param_nKeyCnt, char* param_FindKey );
int freport_GetReportMailInfo(
        char *p_confMailFrom,
        char *p_confMailToDay,
        char *p_confMailToWeek,
        char *p_confMailToMonth
);

int freport_GetRankTimeBysq(
        char *gubun,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp, char param_cRealTimeFlag
);
int freport_GetRankTypeBysq(
        char *gubun,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp,
        char *lang, char param_cRealTimeFlag);
int freport_GetRankIp(
        char *gubun,
        char *rankIp,
        char *sCloseVal,
        char *uWarnVal,
        char *uCritVal,
        char *uBlokVal,
        char *topVal,
        char *sDate,
        char *eDate,
        char *arrGrp,
        char *lang,
        char param_cRealTimeFlag
);
int freport_GetRankGroupBysq(
        char*	gubun,
        char*	rankGroup,
        char*	sCloseVal,
        char*	uWarnVal,
        char*	uCritVal,
        char*	uBlokVal,
        char*	topVal,
        char*	sDate,
        char*	eDate,
        char*	arrGrp,
        char*	lang, char param_cRealTimeFlag);

int freport_GetReportRuntime(char *p_confMailRunTime);
int freport_MergeReportDayBysq(char *p_histEventName, int mLimit, char *p_prefix, char *p_postfix);
int freport_MergeReportWeekBysq(int mLimit, char *p_prefix, char *p_postfix);
int freport_MergeReportMonthBysq(int mLimit, char *p_prefix, char *p_postfix);

/* ------------------------------------------------------------------- */
char g_szCustLang[2+1];

#endif //REPORT_H
