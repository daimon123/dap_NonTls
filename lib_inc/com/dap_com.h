//
// Created by KimByoungGook on 2020-07-14.
//

#ifndef DAP_COM_H
#define DAP_COM_H

#include <stdio.h>
#include <time.h>
#include "com/dap_def.h"



#define     MAX_MASTER_NOTI  1
#define		MAX_BUF		     1024
#define     ONE_DAY          86400

typedef void * (*PTHREAD_START)(void *);

enum ENUM_TIMEFORMAT
{
    YYYYMMDDHHmmSS = 1,
    YYYYMMDDHHmm      ,
    YYYYMMDDHH        ,
    YYYYMMDD          ,
    YYMMDDHHmmSS      ,
    YYMMDDHHmm        ,
    YYMMDDHH          ,
    YYMMDD            ,
    YYMM              ,
    MMDDHHmmSS        ,
    MMDDHHmm          ,
    DDHHmm            ,
    HHmmss
} ;


/** Master Pid File용 Process 구별 ENUM **/
enum
{
    ENUM_DAP_MASTER = 0,    //1
    ENUM_DAP_ALARM,         //2
    ENUM_DAP_DBLOG,         //3
    ENUM_DAP_FRD,           //4
    ENUM_DAP_PCIF ,         //5
    ENUM_DAP_POLICY_FW,     //6
    ENUM_DAP_PRMON,         //7
    ENUM_DAP_PROXY,         //8
    ENUM_DAP_REPORT,        //9
    ENUM_DAP_SCHD,          //10
    ENUM_DAP_SYSMAN,        //11
    ENUM_DAP_VWLOG          //12
};


typedef struct
{
    int  nEnumIdx;
    char szProcName[31 +1];
}_DAP_PROC_PID_FILENAME_ST;

/** DAP서버 Process 관리 리스트 **/
_DAP_PROC_PID_FILENAME_ST g_stProcPidFileSt[12];

struct _ST_CFGENTRY
{
    char *name;
    char *value;
    struct _ST_CFGENTRY *next;
};

typedef struct
{
    int nLogLevel;
    char LevelMsg[16+1];
}_LOGGER_LEVEL;

typedef	struct
{
    char			szNotiFileName[100+1];
    time_t			lastModify;
    int				reload;
} _CONFIG_NOTI;


typedef struct
{
    int nCategoryIdx;
    char CategoryMsg[16+1];
}_LOGGER_CATEGORY;

typedef struct _FUNC_CONF_LIB_config_struct
{
    char	_cf_flag;										/* 서비스 플래그  LVL 1 : 0x01 , LVL 2 : 0x02 */
    int		_cf_group;										/* 그룹 번호   , 순서대로 그룹번호를 매겨둔다.*/
    char	_cf_item[31];			/* 아이템 이름 */
    char	_cf_opt[257];				/* 옵션 내용 */
    int		_cf_num;										/* 라인 넘버 */

} func_conf_lib_config_struct;

_DAP_SERVER_INFO g_stServerInfo ;
_CONFIG_NOTI*    g_pstNotiMaster;


/* ------------------------------------------------------------------- */
/* dap_config.c */
long fcom_Str2Dec(char *cpHex);
char* fcom_GetProfile(char* part, char* spart, char* rval, char* dval );
int fcom_GetProfileInt(char* part, char* spart, int dval);
int	fcom_ReadProfile(char* cpApp, char* cpKey,char* cpRxt);
char* fcom_makeToken(char* cpApp, char* cpRxt);
void fcom_SetIniName(char* name);
/* ------------------------------------------------------------------- */

int	func_conf_lib_Init(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct **Pre_Comm_Lib_Conf_Struct);
void func_conf_lib_free(struct _FUNC_CONF_LIB_config_struct **Pre_Comm_Lib_Conf_Struct);
int	func_conf_lib_Test_Parser(char *Pre_Configure_Path_File_Name);
int	func_conf_lib_Load_Configure(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct *Pre_Comm_Lib_Conf_Struct);
int	func_conf_lib_GetLine_Count(char *Pre_Configure_Path_File_Name);
void func_conf_lib_Cut_Comment(char *Pre_Strings);
int	func_conf_lib_Chk_Item(char *Pre_Strings);
int	func_conf_lib_Opt_Copy(char *Pre_ItemName, char *Pre_OptName, char *Pre_Strings);
int ConfigGetValue(char *pre_szFilePath, char *pre_szGroupName, char *pre_szItemName, char *pre_szOpt);
int	func_conf_lib_Load_Parser(char *Pre_Configure_Path_File_Name, struct _FUNC_CONF_LIB_config_struct	*Pre_Comm_Lib_Conf_Struct);
int func_conf_lib_Duplication_Check(struct _FUNC_CONF_LIB_config_struct *Pre_Comm_Lib_Conf_Struct, int Pre_Max_ItemCount);
int	func_conf_lib_GItem_Copy(char *Pre_GroupNmae, char *Pre_Strings);
int	func_conf_lib_Get_Comment(char *Pre_Strings);

/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */
/* dap_log.c */
int  fcom_LogDirCheck   (void                                                        );
int  fcom_MkPath        (char* file_path, mode_t mode                                );
int  fcom_GetLogSize    (FILE* param_fp                                              );
int  fcom_SetLogInfo    (_DAP_LOG_INFO* param_stDapLogInfo, char* param_ProcName     );
void fcom_LogWrite      (int param_LogLevel, int param_CatLevel, const char *fmt, ...);
void fcom_DbLogWrite(int param_LogLevel, char* param_LogIp, int param_CatLevel, const char *fmt, ...);
int fcom_AgentDbLogWrite( char* param_HbSq,
                          char* param_UserKey,
                          char* param_UserIp,
                          char* param_Process,
                          char* param_LogDate,
                          char* param_LogLevel,
                          char* param_LogMsg);
FILE* fcom_OpenPcifLogFile(char* pcifLogPath, char* cpLName                          );
void fcom_IpLogWrite    (int param_LogLevel, char* cpip, const char *fmt, ...        );
void fcom_JsonWrite     (int param_LogLevel, char* cpip, const char *fmt, ...        );
int  fcom_LogInit       (char* param_ProcName                                        );
void fcom_SetEorFile    (char* param_EorFile                                         );
void fcom_LogTime       (FILE* Logfp                                                 );
int fcom_InitStdLog(const char *log_file);
void fcom_EorRet        (int etype, const char *fmt, ...                             );


#define WRITE_CRITICAL(CATEGORY,MSG,...)        { \
char buffer[1024 +1] = {0x00,}; \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_CRITICAL, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_MAJOR(CATEGORY,MSG,...)           { \
char buffer[1024 +1] = {0x00,}; \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_MAJOR   , CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_WARNING(CATEGORY,MSG,...)         { \
char buffer[1024 +1] = {0x00,}; \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_WARN    , CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_INFO(CATEGORY,MSG,...)            { \
char buffer[1024 +1] = {0x00,}; \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_INFO    , CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_DEBUG(CATEGORY,MSG,...)           { \
char buffer[1024 +1] = {0x00,}; \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_DEBUG   , CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_ALL(CATEGORY,MSG,...)             { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_LogWrite(DAP_ALL     , CATEGORY, buffer, ##__VA_ARGS__); \
}


#define WRITE_CRITICAL_DBLOG(CPIP,CATEGORY,MSG,...)   { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_DbLogWrite(DAP_CRITICAL, CPIP, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_MAJOR_DBLOG(CPIP,CATEGORY, MSG,...)      { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_DbLogWrite(DAP_MAJOR, CPIP, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_WARNING_DBLOG(CPIP,CATEGORY,MSG,...)    { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_DbLogWrite(DAP_WARN, CPIP, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_INFO_DBLOG(CPIP,CATEGORY,MSG,...)       { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_DbLogWrite(DAP_INFO, CPIP, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_DEBUG_DBLOG(CPIP,CATEGORY,MSG,...)      { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_DbLogWrite(DAP_DEBUG, CPIP, CATEGORY, buffer, ##__VA_ARGS__); \
}

#define WRITE_CRITICAL_IP(CPIP,MSG,...)     { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_IpLogWrite(DAP_CRITICAL,  CPIP,buffer, ##__VA_ARGS__); \
}

#define WRITE_MAJOR_IP(CPIP,MSG,...)        { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_IpLogWrite(DAP_MAJOR   ,  CPIP,buffer, ##__VA_ARGS__); \
}

#define WRITE_WARNING_IP(CPIP,MSG,...)      { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_IpLogWrite(DAP_WARN    ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_INFO_IP(CPIP,MSG,...)         { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_IpLogWrite(DAP_INFO    ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_DEBUG_IP(CPIP,MSG,...)        { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_IpLogWrite(DAP_DEBUG   ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_CRITICAL_JSON(CPIP,MSG,...)   { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_JsonWrite(DAP_CRITICAL,  CPIP,buffer, ##__VA_ARGS__); \
}

#define WRITE_MAJOR_JSON(CPIP,MSG,...)      { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_JsonWrite(DAP_MAJOR   ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_WARNING_JSON(CPIP,MSG,...)    { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_JsonWrite(DAP_WARN    ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_INFO_JSON(CPIP,MSG,...)       { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_JsonWrite(DAP_INFO    ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define WRITE_DEBUG_JSON(CPIP,MSG,...)      { \
char buffer[1024 +1] = {0x00,};  \
snprintf(buffer,sizeof(buffer)-1,"%s | %s@%s@%d",MSG,__FILE__,__func__,__LINE__); \
fcom_JsonWrite(DAP_DEBUG   ,  CPIP, buffer, ##__VA_ARGS__); \
}

#define DEBUG_LOG()   {WRITE_DEBUG(CATEGORY_DEBUG,"DBG %d |%s",__LINE__,__func__); }


/* ------------------------------------------------------------------- */


/* ------------------------------------------------------------------- */
/* dap_string.c */
void   fcom_Rtrim               (char *data, int len                            );
void   fcom_Ltrim               (char *str                                      );
void   fcom_Trim                (char *str, int len                             );
void   fcom_SpaceTrim           (char *str, int len                             );
char*  fcom_StrUpper            (char *str                                      );
void   fcom_ParseStringByToken  (char* szStr, char* szTok, char *res, int nOrder);
int    fcom_ChkStrHangle        (char* buffer                                   );
void   fcom_ReplaceAll          (char *inBuf, char *olds, char *news, char *res );
void   fcom_GetFQNum            (char *orgstr, char *mark, char *substr         );
int    fcom_StrToken            (char *str, char *mark, int cnt, char* res      );
int    fcom_StrTokenR           (char *str, char *mark, int cnt, char* res      );
int    fcom_GetReversePos       (char *s, char c                                );
int     fcom_GetTagValue(char*in, char* tag1, char* tag2, char* out, int outBufferSize);
//int    fcom_GetTagValue(char*in, char* tag1, char* tag2, char* out);
int    fcom_getBClassIp         (char *str, char* res                           );
void   fcom_SubStr              (int start, int len, char *orgstr, char *substr );
char** fcom_Jsplit              (char* tmp, char ch                             );
int    fcom_Str2ValToken        (char *str, char *mark, char* res1, char* res2  );
char*  fcom_StringAllocExing    (int len                                        );
void*  fcom_BufferAllocReturning(int len,int *result                            );
int    fcom_IsDigit             (char* Buffer                                   );
void   fcom_StrcatSafe          (char *dest, size_t dest_size, const char *src  );
int    fcom_TokenCnt            (char *orgstr, char *mark                       );
int    fcom_GetStringPos        (char *s, char c                                );
int    fcom_GetRoundTagValue    (char*in, char* out                             );
/* ------------------------------------------------------------------- */




/* ------------------------------------------------------------------- */
/* dap_time.c */
int fcom_GetFormatTime(enum ENUM_TIMEFORMAT param_format, char* paramBuffer, int BufferLen);
char*           fcom_time2str       (time_t nTime, char* cpTmp,char* format                 );
int             fcom_GetSysMinute   (                                                       );
void            fcom_getSysdate     (char* timebuf                                          );
time_t	        fcom_str2time       (char *cpSrc, char * format                             );
time_t          fcom_CalcTime       (int value                                              );
void            fcom_Msleep         (time_t msec                                            );
int fcom_GetAgoTime(int nAgoDay, char* param_TimeBuff);
int             fcom_GetTime        (char *timebuff, int sec                                );
int             fcom_GetDayOfWeek   (char *date                                             );
unsigned short  fcom_GetDayOfMonth  (struct tm *st                                          );
unsigned short  fcom_GetWeekOfMonth (struct tm *st                                          );
void            fcom_DayofWeek2Str  (int weekNum, char *res                                 );
void            fcom_GetDatemm2digit( char*   pre_date_time                                 );
void            fcom_GetDateyyyy4digit( char* pre_date_time                                 );
void            fcom_GetDateDD2digit( char *pre_date_time                                   );
void            fcom_SleepWait      ( int pre_int_wait                                      );
long fcom_GetDaysTo(char *yyyymmdd);
long fcom_GetRelativeDate(char* aDate, char* bDate);
int fcom_ToDay(int param_YYYY, int param_MM, int param_Day);
int fcom_DiffDay(int param_nYYYY1, int param_nMM1, int param_nDD1, int param_nYYYY2, int param_nMM2, int param_nDD2);
/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */
/* dap_util.c */

pid_t fcom_SetDeamon        (int param_CurLimit, int param_MaxLimit             );
void  fcom_ExitServer       (int param_nNum                                     );
void  fcom_BufferFree       (char *buf                                          );
int	  fcom_MallocFree       ( void **pre_point                                  );
int   fcom_ThreadCreate(void *threadp, PTHREAD_START start_func, void *arg, size_t nStackSize);
int   fcom_ArgParse         (char **argv                                        );
int   fcom_GetBiggerNumber  (int a, int b                                       );
int   fcom_IsNumber         (char *numStr                                       );
char* fcom_Ui2Ip            (unsigned int ipAsUInt                              );
void  fcom_GetUniqId        (char *p_userKey                                    );
int   fcom_FileCheckModify  (char* CheckFilePath, time_t* LastModify            );
short fcom_procFileCheckStatus(char* preFilepathFilename, char* processFileName );
short fcom_fileCheckStatus  (char* preFilepathFilename);
int   fcom_GetFileRows      (char *file                                         );
int   fcom_GetFileCols      (char *file                                         );
int   fcom_GetFileSize      (char *fullPath, char *fileName                     );
/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */
/* dap_report.c */
int   fcom_SetCustLang          (unsigned char *p_lang                                                                      );
char* fcom_GetCustLang          (                                                                                           );
void  fcom_GetStrType2          (int type, char* res, char* param_confLang                                                  );
char* fcom_getStrType(int type);
void  fcom_GetStrLevel          (int level, char* res                                                                       );
int   fcom_GetNumberLevel       (char* level                                                                                );
int   fcom_GetReportFile        (char *fPath, char *fName, char *res                                                        );
int   fcom_ReportPieFilter      (char *jsbuff, char *sVar, char *jsonData, char* szConfMailLang, char* szConfMailClosedView );
int   fcom_ReportHeaderFilter   (char *jsbuff, char *sChart, char *sUtil, int totSize                                       );
int   fcom_ReportBodyFilter     (
                                 char* jsbuff   ,
                                 char* strRange ,
                                 char* strTable ,
                                 int   dFlag    ,
                                 int   cFlag    ,
                                 int   totSize  ,
                                 char* grpName  ,
                                 char* szConfMailLang
                                );
int fcom_ReportFooterFilter     (char *jsbuff, int totSize, char* szConfMailLang                                            );
int fcom_ReportLineFilter       (
                                 char* jsbuff,
                                 char* sVar  ,
                                 char* sTp   ,
                                 char* sClose,
                                 char* sWarn ,
                                 char* sCrit ,
                                 char* sBlok                                                                                );
int fcom_ReportBarFilter        (
                                 char* jsbuff,
                                 char* sVar  ,
                                 char* sTp   ,
                                 char* sClose,
                                 char* sWarn ,
                                 char* sCrit ,
                                 char* sBlok                                                                                );


short fcom_FileCheckAndDelete(char* preFilepath, char* processName );
int   fcom_MakeFlagFile(char* FilePath);

int   fFile_GetMasterPid(char* PidFilePath, char* ProcessName);
int   fFile_CheckMasterPid(char* PidFilePath, char* ProcessName);
int   fcom_EucKrTOUtf8(char *pre_buff , int pre_buff_memory_size );
int	  fcom_malloc ( void **pre_point , size_t pre_size );
void fcom_DumpBinData( char* param_PreDataBuff, int param_PreDataBuffSize );
void  fcom_NobodyFunc();

int   fFile_MakeMasterPid(char* PidFilePath, char* ProcessName);
void  fFile_cleanupMasterPid(char* preFilePath, char* processName );
void  fFile_SetPidFileSt(void);
char* fFile_GetPidFileName(int param_EnumFileIdx);


#endif //DAP_COM_H
